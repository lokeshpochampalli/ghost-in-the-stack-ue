#include "GitsTrace.h"
#include "GitsDiagnostics.h"

namespace GitsTrace
{
	const FGitsStep& At(const FGitsTrace& Trace, int32 Index)
	{
		check(Trace.Steps.IsValidIndex(Index));
		return Trace.Steps[Index];
	}

	int32 Length(const FGitsTrace& Trace) { return Trace.Steps.Num(); }

	TArray<int32> StatementBoundaries(const FGitsTrace& Trace)
	{
		TArray<int32> Out;
		for (int32 i = 0; i < Trace.Steps.Num(); ++i) { if (Trace.Steps[i].bIsStatementBoundary) { Out.Add(i); } }
		return Out;
	}

	TArray<FString> OutputAt(const FGitsTrace& Trace, int32 Index)
	{
		TArray<FString> Out;
		for (int32 k = 0; k <= Index && k < Trace.Steps.Num(); ++k) { Out.Append(Trace.Steps[k].Output); }
		return Out;
	}

	FGitsWorldState WorldAt(const FGitsTrace& Trace, int32 Index)
	{
		FGitsWorldState World = Trace.InitialWorld;
		for (int32 k = 0; k <= Index && k < Trace.Steps.Num(); ++k)
		{
			for (const FGitsEffect& E : Trace.Steps[k].Effects) { World = GitsWorld::Reduce(World, E); }
		}
		return World;
	}

	TArray<FGitsEffect> AllEffects(const FGitsTrace& Trace)
	{
		TArray<FGitsEffect> Out;
		for (const FGitsStep& S : Trace.Steps) { Out.Append(S.Effects); }
		return Out;
	}

	TArray<TPair<FString, FGitsValue>> BindingsAt(const FGitsTrace& Trace, int32 Index)
	{
		TArray<TPair<FString, FGitsValue>> Out;
		if (!Trace.Steps.IsValidIndex(Index) || Trace.Steps[Index].Frames.Num() == 0) { return Out; }
		for (const auto& P : Trace.Steps[Index].Frames.Last().Bindings) { Out.Emplace(P.Key, GitsValue::Copy(P.Value)); }
		return Out;
	}

	FGitsWorldOracle ReplayOracle(const FGitsTrace& Trace)
	{
		TSharedPtr<TArray<FGitsValue>> Reads = MakeShared<TArray<FGitsValue>>();
		for (const FGitsStep& S : Trace.Steps) { if (S.bHasOracleRead) { Reads->Add(S.OracleRead.Value); } }
		TSharedPtr<int32> Cursor = MakeShared<int32>(0);
		return [Reads, Cursor](const FGitsWorldState&, const FGitsQuery&) -> FGitsValue
		{
			if (*Cursor >= Reads->Num()) { return FGitsValue::MakeNone(); }
			return (*Reads)[(*Cursor)++];
		};
	}

	// --- diff (ADR-002) --------------------------------------------------------------------

	static bool SameValue(const FGitsValue& A, const FGitsValue& B)
	{
		// Stricter than ==: 2 and 2.0 are a real behavioural difference between two runs.
		if (A.Kind != B.Kind) { return false; }
		return GitsValue::Equal(A, B);
	}

	static TArray<FString> CompareFrames(const TArray<FGitsFrameSnapshot>& A, const TArray<FGitsFrameSnapshot>& B)
	{
		TArray<FString> Lines;
		if (A.Num() != B.Num())
		{
			Lines.Add(FString::Printf(TEXT("the call stack was %d deep, now %d"), A.Num(), B.Num()));
			return Lines;
		}
		for (int32 i = 0; i < A.Num(); ++i)
		{
			TArray<FString> Names;
			for (const auto& P : A[i].Bindings) { Names.AddUnique(P.Key); }
			for (const auto& P : B[i].Bindings) { Names.AddUnique(P.Key); }
			Names.Sort();
			for (const FString& Name : Names)
			{
				const FGitsValue* VA = A[i].Find(Name);
				const FGitsValue* VB = B[i].Find(Name);
				if (!VA && VB) { Lines.Add(FString::Printf(TEXT("%s now exists, holding %s"), *Name, *GitsValue::Repr(*VB))); }
				else if (VA && !VB) { Lines.Add(FString::Printf(TEXT("%s no longer exists; it held %s"), *Name, *GitsValue::Repr(*VA))); }
				else if (VA && VB && !SameValue(*VA, *VB)) { Lines.Add(FString::Printf(TEXT("%s was %s, now %s"), *Name, *GitsValue::Repr(*VA), *GitsValue::Repr(*VB))); }
			}
		}
		return Lines;
	}

	static FString DescribeLines(const TArray<FString>& Lines)
	{
		if (Lines.Num() == 0) { return TEXT("nothing"); }
		TArray<FString> Quoted;
		for (const FString& L : Lines) { Quoted.Add(QuoteJson(L)); }
		return FString::Join(Quoted, TEXT(", "));
	}

	static const TCHAR* OutcomeName(EGitsOutcomeKind K)
	{
		switch (K)
		{
		case EGitsOutcomeKind::Completed: return TEXT("completed");
		case EGitsOutcomeKind::Error: return TEXT("error");
		case EGitsOutcomeKind::StatementCapExceeded: return TEXT("statementCapExceeded");
		case EGitsOutcomeKind::SafetyCapExceeded: return TEXT("safetyCapExceeded");
		}
		return TEXT("?");
	}

	FGitsDiffResult Diff(const FGitsTrace& A, const FGitsTrace& B)
	{
		FGitsDiffResult R;
		const TArray<int32> BA = StatementBoundaries(A);
		const TArray<int32> BB = StatementBoundaries(B);
		const int32 Shared = FMath::Min(BA.Num(), BB.Num());
		for (int32 n = 0; n < Shared; ++n)
		{
			const int32 IA = BA[n], IB = BB[n];
			const FGitsStep& SA = A.Steps[IA];
			const FGitsStep& SB = B.Steps[IB];
			TArray<EGitsDivergenceKind> Kinds;
			TArray<FString> Summary;
			TArray<FString> Bindings = CompareFrames(SA.Frames, SB.Frames);
			if (Bindings.Num() > 0) { Kinds.Add(EGitsDivergenceKind::Bindings); Summary.Append(Bindings); }
			const TArray<FString> OutA = OutputAt(A, IA), OutB = OutputAt(B, IB);
			if (OutA != OutB)
			{
				Kinds.Add(EGitsDivergenceKind::Output);
				Summary.Add(FString::Printf(TEXT("output was %s, now %s"), *DescribeLines(OutA), *DescribeLines(OutB)));
			}
			TArray<FGitsEffect> FxA, FxB;
			for (int32 k = 0; k <= IA; ++k) { FxA.Append(A.Steps[k].Effects); }
			for (int32 k = 0; k <= IB; ++k) { FxB.Append(B.Steps[k].Effects); }
			bool bSameFx = FxA.Num() == FxB.Num();
			for (int32 k = 0; bSameFx && k < FxA.Num(); ++k) { bSameFx = FxA[k] == FxB[k]; }
			if (!bSameFx)
			{
				Kinds.Add(EGitsDivergenceKind::Effects);
				Summary.Add(FString::Printf(TEXT("the facility saw %d change%s, now %d"), FxA.Num(), FxA.Num() == 1 ? TEXT("") : TEXT("s"), FxB.Num()));
			}
			if (Kinds.Num() > 0)
			{
				R.bDiverged = true; R.AtBoundary = n; R.AStepIndex = IA; R.BStepIndex = IB; R.Kinds = Kinds; R.Summary = Summary;
				return R;
			}
		}
		if (BA.Num() != BB.Num())
		{
			R.bDiverged = true; R.AtBoundary = Shared;
			R.AStepIndex = BA.IsValidIndex(Shared) ? BA[Shared] : -1;
			R.BStepIndex = BB.IsValidIndex(Shared) ? BB[Shared] : -1;
			R.Kinds.Add(EGitsDivergenceKind::Length);
			R.Summary.Add(FString::Printf(TEXT("the program ran %d steps before, and %d now"), BA.Num(), BB.Num()));
			return R;
		}
		if (A.Outcome.Kind != B.Outcome.Kind)
		{
			R.bDiverged = true; R.AtBoundary = Shared == 0 ? 0 : Shared - 1;
			R.AStepIndex = BA.IsValidIndex(Shared - 1) ? BA[Shared - 1] : -1;
			R.BStepIndex = BB.IsValidIndex(Shared - 1) ? BB[Shared - 1] : -1;
			R.Kinds.Add(EGitsDivergenceKind::Outcome);
			R.Summary.Add(FString::Printf(TEXT("the run ended as %s before, and as %s now"), OutcomeName(A.Outcome.Kind), OutcomeName(B.Outcome.Kind)));
			return R;
		}
		return R;
	}

	// --- serialisation (ADR-015) ---------------------------------------------------------

	FString QuoteJson(const FString& S)
	{
		FString Out = TEXT("\"");
		for (TCHAR C : S)
		{
			switch (C)
			{
			case '"': Out += TEXT("\\\""); break;
			case '\\': Out += TEXT("\\\\"); break;
			case '\n': Out += TEXT("\\n"); break;
			case '\t': Out += TEXT("\\t"); break;
			case '\r': Out += TEXT("\\r"); break;
			default:
				if (C < 0x20) { Out += FString::Printf(TEXT("\\u%04x"), (int32)C); }
				else { Out.AppendChar(C); }
				break;
			}
		}
		return Out + TEXT("\"");
	}

	FString DescribeEffect(const FGitsEffect& E)
	{
		switch (E.Kind)
		{
		case FGitsEffect::EKind::Set: return FString::Printf(TEXT("set %s=%s"), *E.Key, *E.Value.ToText());
		case FGitsEffect::EKind::Log: return TEXT("log ") + QuoteJson(E.Message);
		case FGitsEffect::EKind::Wait: return FString::Printf(TEXT("wait %lld"), E.Ticks);
		}
		return TEXT("");
	}

	FString DescribeOutcome(const FGitsTrace& Trace)
	{
		const FGitsOutcome& O = Trace.Outcome;
		switch (O.Kind)
		{
		case EGitsOutcomeKind::Completed: return TEXT("completed");
		case EGitsOutcomeKind::Error:
			return FString::Printf(TEXT("error %s at line %d"), *GitsDiagnostics::CodeName(O.Diagnostic.Code), O.Diagnostic.Span.Start.Line);
		case EGitsOutcomeKind::StatementCapExceeded:
			return FString::Printf(TEXT("statement cap %d exceeded at line %d"), O.Cap, O.Diagnostic.Span.Start.Line);
		case EGitsOutcomeKind::SafetyCapExceeded:
			return FString::Printf(TEXT("safety cap %d exceeded at line %d"), O.Cap, O.Diagnostic.Span.Start.Line);
		}
		return TEXT("");
	}

	static FString SpanLabel(const FGitsSpan& S)
	{
		if (S.Start.Line == S.End.Line) { return FString::Printf(TEXT("L%d:%d-%d"), S.Start.Line, S.Start.Column, S.End.Column); }
		return FString::Printf(TEXT("L%d-%d"), S.Start.Line, S.End.Line);
	}

	static FString PadRight(const FString& S, int32 Width)
	{
		return S.Len() >= Width ? S : S + FString::ChrN(Width - S.Len(), TEXT(' '));
	}

	FString Serialise(const FGitsTrace& Trace)
	{
		TArray<FString> Lines;
		for (const FGitsStep& S : Trace.Steps)
		{
			TArray<FString> Extras;
			for (const FString& L : S.Output) { Extras.Add(TEXT("out ") + QuoteJson(L)); }
			for (const FGitsEffect& E : S.Effects) { Extras.Add(TEXT("fx ") + DescribeEffect(E)); }
			if (S.bHasOracleRead)
			{
				Extras.Add(FString::Printf(TEXT("read %s:%s -> %s"), *S.OracleRead.Query.Kind, *S.OracleRead.Query.Id, *GitsValue::Repr(S.OracleRead.Value)));
			}
			const FString Tail = Extras.Num() > 0 ? TEXT("  | ") + FString::Join(Extras, TEXT(" ")) : TEXT("");
			Lines.Add(FString::Printf(TEXT("%03d%s %s %s d%d  %s%s"), S.Index, S.bIsStatementBoundary ? TEXT("*") : TEXT(" "),
				*PadRight(GitsStepKindName(S.Kind), 10), *PadRight(SpanLabel(S.Span), 12), S.Depth, *S.Label, *Tail));
		}
		Lines.Add(TEXT(""));
		Lines.Add(TEXT("-- outcome: ") + DescribeOutcome(Trace));
		Lines.Add(FString::Printf(TEXT("-- statements: %d  steps: %d"), Trace.StatementCount, Trace.Steps.Num()));
		if (Trace.Output.Num() > 0)
		{
			Lines.Add(TEXT("-- printed:"));
			for (const FString& L : Trace.Output) { Lines.Add(TEXT("   ") + L); }
		}
		return FString::Join(Lines, TEXT("\n"));
	}
}
