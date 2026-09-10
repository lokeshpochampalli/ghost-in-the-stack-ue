#include "GitsEvaluator.h"
#include "GitsDiagnostics.h"
#include <cmath>

namespace
{
	enum class EFlow : uint8 { Normal, Break, Continue, Return, Abort };

	struct FFrame
	{
		FString FunctionName;
		TArray<TPair<FString, FGitsValue>> Bindings;
		bool bHasCallSite = false;
		FGitsSpan CallSite;

		FGitsValue* Find(const FString& Name)
		{
			for (auto& P : Bindings) { if (P.Key == Name) { return &P.Value; } }
			return nullptr;
		}
		void Bind(const FString& Name, const FGitsValue& V)
		{
			if (FGitsValue* Existing = Find(Name)) { *Existing = V; return; }
			Bindings.Emplace(Name, V);
		}
	};

	FString Plural(int64 N, const TCHAR* Singular, const TCHAR* PluralForm)
	{
		return FString::Printf(TEXT("%lld %s"), N, N == 1 ? Singular : PluralForm);
	}

	class FEvaluator
	{
	public:
		FEvaluator(const FGitsProgram& InProgram, const FGitsWorldState& InitialWorld, FGitsWorldOracle InOracle, const FGitsRunOptions& InOptions)
			: Program(InProgram), Oracle(InOracle), Options(InOptions), World(InitialWorld)
		{
			Trace.InitialWorld = InitialWorld;
			Trace.Seed = InOptions.Seed;
			FFrame Module;
			Module.FunctionName = TEXT("<module>");
			Frames.Add(Module);
		}

		FGitsTrace Run()
		{
			if (Program.Root.IsValid())
			{
				for (const FGitsNodePtr& S : Program.Root->Body)
				{
					const EFlow F = ExecStmt(S);
					if (F == EFlow::Abort) { break; }
				}
			}
			Trace.StatementCount = StatementCount;
			return MoveTemp(Trace);
		}

	private:
		const FGitsProgram& Program;
		FGitsWorldOracle Oracle;
		FGitsRunOptions Options;
		FGitsWorldState World;
		FGitsTrace Trace;
		TArray<FFrame> Frames;
		TMap<FString, FGitsNodePtr> Definitions;
		int32 NextListId = 1;
		int32 StatementCount = 0;
		int32 NextStmtIndex = 0;
		int32 StmtIndex = 0;
		bool bStopped = false;
		FGitsValue ReturnValue;
		/** Set while the iterable of a for header is evaluated, so range() is legal there. */
		bool bInForHeader = false;
		TArray<FGitsValue> RangeValues;
		bool bRangeProduced = false;

		// --- errors and stopping ------------------------------------------------------------

		bool Error(EGitsDiagnosticCode Code, const FGitsSpan& Span, const FGitsDiagnosticParams& P = FGitsDiagnosticParams())
		{
			if (!bStopped)
			{
				Trace.Outcome.Kind = EGitsOutcomeKind::Error;
				Trace.Outcome.Diagnostic = GitsDiagnostics::Diagnose(Code, Span, P);
				bStopped = true;
			}
			return false;
		}

		void CapOutcome(EGitsOutcomeKind Kind, EGitsDiagnosticCode Code, int32 Cap)
		{
			FGitsSpan Span;
			bool bFound = false;
			for (int32 i = Trace.Steps.Num() - 1; i >= 0; --i)
			{
				if (Trace.Steps[i].Kind == EGitsStepKind::Iterate) { Span = Trace.Steps[i].Span; bFound = true; break; }
			}
			if (!bFound && Trace.Steps.Num() > 0) { Span = Trace.Steps.Last().Span; }
			Trace.Outcome.Kind = Kind;
			Trace.Outcome.Cap = Cap;
			Trace.Outcome.Diagnostic = GitsDiagnostics::Diagnose(Code, Span, FGitsDiagnosticParams(FString::FromInt(Cap), FString::FromInt(Span.Start.Line)));
			bStopped = true;
		}

		// --- steps --------------------------------------------------------------------------

		TArray<FGitsFrameSnapshot> Snapshot() const
		{
			TArray<FGitsFrameSnapshot> Out;
			Out.Reserve(Frames.Num());
			for (const FFrame& F : Frames)
			{
				FGitsFrameSnapshot S;
				S.FunctionName = F.FunctionName;
				S.bHasCallSite = F.bHasCallSite;
				S.CallSite = F.CallSite;
				S.Bindings.Reserve(F.Bindings.Num());
				for (const auto& P : F.Bindings) { S.Bindings.Emplace(P.Key, GitsValue::Copy(P.Value)); }
				Out.Add(MoveTemp(S));
			}
			return Out;
		}

		/** Appends a step. Returns false when a cap stopped the run. */
		bool Emit(const FGitsNodePtr& Node, EGitsStepKind Kind, int32 Depth, const FString& Label, bool bBoundary,
			const TArray<FString>* Output = nullptr, const TArray<FGitsEffect>* Effects = nullptr, const FGitsOracleRead* Read = nullptr)
		{
			if (bStopped) { return false; }
			FGitsStep S;
			S.Index = Trace.Steps.Num();
			S.NodeId = Node->NodeId;
			S.Span = Node->Span;
			S.Kind = Kind;
			S.Depth = Depth;
			S.StmtIndex = StmtIndex;
			S.bIsStatementBoundary = bBoundary;
			S.Frames = Snapshot();
			if (Output) { S.Output = *Output; for (const FString& L : *Output) { Trace.Output.Add(L); } }
			if (Effects) { S.Effects = *Effects; }
			if (Read) { S.bHasOracleRead = true; S.OracleRead = *Read; }
			S.Label = Label;
			Trace.Steps.Add(MoveTemp(S));
			if (bBoundary) { ++StatementCount; }

			if (Trace.Steps.Num() >= GitsLimits::SafetyStepCap)
			{
				CapOutcome(EGitsOutcomeKind::SafetyCapExceeded, EGitsDiagnosticCode::SafetyCapExceeded, GitsLimits::SafetyStepCap);
				return false;
			}
			if (bBoundary && StatementCount >= Options.StatementCap)
			{
				CapOutcome(EGitsOutcomeKind::StatementCapExceeded, EGitsDiagnosticCode::StatementCapExceeded, Options.StatementCap);
				return false;
			}
			return true;
		}

		// --- names --------------------------------------------------------------------------

		/** Local, then module. The whole scope rule. */
		FGitsValue* Lookup(const FString& Name)
		{
			if (FGitsValue* V = Frames.Last().Find(Name)) { return V; }
			if (Frames.Num() > 1) { if (FGitsValue* V = Frames[0].Find(Name)) { return V; } }
			return nullptr;
		}

		void Bind(const FString& Name, const FGitsValue& V) { Frames.Last().Bind(Name, V); }

		// --- statements ---------------------------------------------------------------------

		EFlow ExecBlock(const TArray<FGitsNodePtr>& Body)
		{
			for (const FGitsNodePtr& S : Body)
			{
				const EFlow F = ExecStmt(S);
				if (F != EFlow::Normal) { return F; }
			}
			return EFlow::Normal;
		}

		EFlow ExecStmt(const FGitsNodePtr& S)
		{
			if (bStopped) { return EFlow::Abort; }
			const int32 Saved = StmtIndex;
			StmtIndex = NextStmtIndex++;
			const EFlow F = ExecStmtInner(S);
			StmtIndex = Saved;
			return bStopped ? EFlow::Abort : F;
		}

		EFlow ExecStmtInner(const FGitsNodePtr& S)
		{
			switch (S->Kind)
			{
			case EGitsNodeKind::Assign:
			{
				FGitsValue V;
				if (!Eval(S->Value, 1, V)) { return EFlow::Abort; }
				Bind(S->Target->Text, V);
				Emit(S, EGitsStepKind::Assign, 0, S->Target->Text + TEXT("=") + GitsValue::Repr(V), true);
				return EFlow::Normal;
			}
			case EGitsNodeKind::AugAssign:
			{
				FGitsValue* Current = Lookup(S->Target->Text);
				if (!Current) { Error(EGitsDiagnosticCode::NameNotDefined, S->Target->Span, FGitsDiagnosticParams(S->Target->Text)); return EFlow::Abort; }
				const FGitsValue Left = *Current;
				FGitsValue Rhs;
				if (!Eval(S->Value, 1, Rhs)) { return EFlow::Abort; }
				FGitsValue Result;
				if (!BinaryOp(S->Text, Left, Rhs, S->Span, Result)) { return EFlow::Abort; }
				Bind(S->Target->Text, Result);
				Emit(S, EGitsStepKind::AugAssign, 0, S->Target->Text + TEXT("=") + GitsValue::Repr(Result), true);
				return EFlow::Normal;
			}
			case EGitsNodeKind::ExprStmt:
			{
				FGitsValue V;
				if (!Eval(S->Value, 1, V)) { return EFlow::Abort; }
				Emit(S, EGitsStepKind::Expr, 0, GitsAst::Unparse(S->Value) + TEXT(" -> ") + GitsValue::Repr(V), true);
				return EFlow::Normal;
			}
			case EGitsNodeKind::If:
			{
				FGitsValue T;
				if (!Eval(S->Test, 1, T)) { return EFlow::Abort; }
				const bool bTaken = GitsValue::Truthy(T);
				const FString Label = FString(S->bIsElif ? TEXT("elif ") : TEXT("if ")) + GitsAst::Unparse(S->Test) + (bTaken ? TEXT(" -> True") : TEXT(" -> False"));
				if (!Emit(S, EGitsStepKind::Branch, 0, Label, true)) { return EFlow::Abort; }
				return ExecBlock(bTaken ? S->Body : S->OrElse);
			}
			case EGitsNodeKind::While:
			{
				int32 Iteration = 0;
				while (true)
				{
					FGitsValue T;
					if (!Eval(S->Test, 1, T)) { return EFlow::Abort; }
					const FString Test = GitsAst::Unparse(S->Test);
					if (!GitsValue::Truthy(T))
					{
						Emit(S, EGitsStepKind::Iterate, 0, FString::Printf(TEXT("while %s -> False, loop ends after %d"), *Test, Iteration), true);
						return bStopped ? EFlow::Abort : EFlow::Normal;
					}
					++Iteration;
					if (!Emit(S, EGitsStepKind::Iterate, 0, FString::Printf(TEXT("while %s -> True (iteration %d)"), *Test, Iteration), true)) { return EFlow::Abort; }
					const EFlow F = ExecBlock(S->Body);
					if (F == EFlow::Abort || F == EFlow::Return) { return F; }
					if (F == EFlow::Break) { return EFlow::Normal; }
				}
			}
			case EGitsNodeKind::For:
			{
				TArray<FGitsValue> Sequence;
				TSharedPtr<FGitsList> LiveList;
				const bool bRangeHeader = S->Iter->Kind == EGitsNodeKind::Call && S->Iter->Func->Kind == EGitsNodeKind::Name
					&& S->Iter->Func->Text == TEXT("range") && Lookup(TEXT("range")) == nullptr;
				FGitsValue IterValue;
				bInForHeader = bRangeHeader;
				bRangeProduced = false;
				const bool bOk = Eval(S->Iter, 1, IterValue);
				bInForHeader = false;
				if (!bOk) { return EFlow::Abort; }
				if (bRangeProduced) { Sequence = RangeValues; }
				else if (IterValue.Kind == EGitsValueKind::List) { LiveList = IterValue.List; }
				else if (IterValue.Kind == EGitsValueKind::Str)
				{
					for (TCHAR C : IterValue.Str) { Sequence.Add(FGitsValue::MakeStr(FString::Chr(C))); }
				}
				else
				{
					Error(EGitsDiagnosticCode::NotAList, S->Iter->Span, FGitsDiagnosticParams(GitsAst::Unparse(S->Iter)));
					return EFlow::Abort;
				}
				const FString Var = S->Target->Text;
				int32 Iteration = 0;
				for (int32 i = 0; ; ++i)
				{
					FGitsValue Item;
					if (LiveList.IsValid()) { if (i >= LiveList->Items.Num()) { break; } Item = LiveList->Items[i]; }
					else { if (i >= Sequence.Num()) { break; } Item = Sequence[i]; }
					++Iteration;
					Bind(Var, Item);
					if (!Emit(S, EGitsStepKind::Iterate, 0, FString::Printf(TEXT("for %s=%s (iteration %d)"), *Var, *GitsValue::Repr(Item), Iteration), true)) { return EFlow::Abort; }
					const EFlow F = ExecBlock(S->Body);
					if (F == EFlow::Abort || F == EFlow::Return) { return F; }
					if (F == EFlow::Break) { return EFlow::Normal; }
				}
				Emit(S, EGitsStepKind::Iterate, 0, FString::Printf(TEXT("for %s: loop ends after %d"), *Var, Iteration), true);
				return bStopped ? EFlow::Abort : EFlow::Normal;
			}
			case EGitsNodeKind::FunctionDef:
			{
				Definitions.Add(S->NodeId, S);
				Bind(S->Text, FGitsValue::MakeFunction(S->Text, S->Params, S->NodeId));
				Emit(S, EGitsStepKind::Def, 0, FString::Printf(TEXT("def %s(%s)"), *S->Text, *GitsAst::ParamList(*S)), true);
				return EFlow::Normal;
			}
			case EGitsNodeKind::Return:
			{
				FGitsValue V;
				if (S->Value.IsValid()) { if (!Eval(S->Value, 1, V)) { return EFlow::Abort; } }
				ReturnValue = V;
				if (!Emit(S, EGitsStepKind::Return, 0, TEXT("return ") + GitsValue::Repr(V), true)) { return EFlow::Abort; }
				return EFlow::Return;
			}
			case EGitsNodeKind::Break:
				if (!Emit(S, EGitsStepKind::Break, 0, TEXT("break"), true)) { return EFlow::Abort; }
				return EFlow::Break;
			case EGitsNodeKind::Continue:
				if (!Emit(S, EGitsStepKind::Continue, 0, TEXT("continue"), true)) { return EFlow::Abort; }
				return EFlow::Continue;
			default:
				return EFlow::Normal;
			}
		}

		// --- expressions --------------------------------------------------------------------

		bool Eval(const FGitsNodePtr& N, int32 Depth, FGitsValue& Out)
		{
			if (bStopped) { return false; }
			switch (N->Kind)
			{
			case EGitsNodeKind::Num:
				Out = N->bIsFloat ? FGitsValue::MakeFloat(N->FloatValue) : FGitsValue::MakeInt(N->IntValue);
				return Emit(N, EGitsStepKind::Literal, Depth, GitsValue::Repr(Out), false);
			case EGitsNodeKind::Str:
				Out = FGitsValue::MakeStr(N->StrValue);
				return Emit(N, EGitsStepKind::Literal, Depth, GitsValue::Repr(Out), false);
			case EGitsNodeKind::Bool:
				Out = FGitsValue::MakeBool(N->bBoolValue);
				return Emit(N, EGitsStepKind::Literal, Depth, GitsValue::Repr(Out), false);
			case EGitsNodeKind::NoneLit:
				Out = FGitsValue::MakeNone();
				return Emit(N, EGitsStepKind::Literal, Depth, TEXT("None"), false);
			case EGitsNodeKind::Name:
			{
				FGitsValue* V = Lookup(N->Text);
				if (!V) { return Error(EGitsDiagnosticCode::NameNotDefined, N->Span, FGitsDiagnosticParams(N->Text)); }
				Out = *V;
				return Emit(N, EGitsStepKind::Name, Depth, N->Text + TEXT(" -> ") + GitsValue::Repr(Out), false);
			}
			case EGitsNodeKind::List:
			{
				TSharedPtr<FGitsList> L = MakeShared<FGitsList>();
				L->Id = NextListId++;
				for (const FGitsNodePtr& E : N->Elts)
				{
					FGitsValue Item;
					if (!Eval(E, Depth + 1, Item)) { return false; }
					L->Items.Add(Item);
				}
				Out = FGitsValue::MakeList(L);
				return Emit(N, EGitsStepKind::List, Depth, GitsAst::Unparse(N) + TEXT(" -> ") + GitsValue::Repr(Out), false);
			}
			case EGitsNodeKind::BinOp:
			{
				FGitsValue L, R;
				if (!Eval(N->Left, Depth + 1, L)) { return false; }
				if (!Eval(N->Right, Depth + 1, R)) { return false; }
				if (!BinaryOp(N->Text, L, R, N->Span, Out)) { return false; }
				return Emit(N, EGitsStepKind::BinOp, Depth, GitsAst::Unparse(N) + TEXT(" -> ") + GitsValue::Repr(Out), false);
			}
			case EGitsNodeKind::UnaryOp:
			{
				FGitsValue V;
				if (!Eval(N->Operand, Depth + 1, V)) { return false; }
				if (N->Text == TEXT("not")) { Out = FGitsValue::MakeBool(!GitsValue::Truthy(V)); }
				else
				{
					if (V.Kind == EGitsValueKind::Float) { Out = FGitsValue::MakeFloat(-V.Float); }
					else if (V.IsNumeric()) { Out = FGitsValue::MakeInt(-V.AsInt()); }
					else { return Error(EGitsDiagnosticCode::TypeMismatch, N->Span, FGitsDiagnosticParams(TEXT("negate ") + GitsValue::TypeName(V))); }
				}
				return Emit(N, EGitsStepKind::UnaryOp, Depth, GitsAst::Unparse(N) + TEXT(" -> ") + GitsValue::Repr(Out), false);
			}
			case EGitsNodeKind::BoolOp:
			{
				FGitsValue L;
				if (!Eval(N->Left, Depth + 1, L)) { return false; }
				const bool bAnd = N->Text == TEXT("and");
				const bool bShort = bAnd ? !GitsValue::Truthy(L) : GitsValue::Truthy(L);
				if (bShort)
				{
					Out = L;
					return Emit(N, EGitsStepKind::BoolOp, Depth, GitsAst::Unparse(N) + TEXT(" -> ") + GitsValue::Repr(Out) + TEXT(" (right side skipped)"), false);
				}
				FGitsValue R;
				if (!Eval(N->Right, Depth + 1, R)) { return false; }
				Out = R;
				return Emit(N, EGitsStepKind::BoolOp, Depth, GitsAst::Unparse(N) + TEXT(" -> ") + GitsValue::Repr(Out), false);
			}
			case EGitsNodeKind::Compare:
			{
				FGitsValue L, R;
				if (!Eval(N->Left, Depth + 1, L)) { return false; }
				if (!Eval(N->Right, Depth + 1, R)) { return false; }
				bool Result = false;
				if (!Compare(N, L, R, Result)) { return false; }
				Out = FGitsValue::MakeBool(Result);
				return Emit(N, EGitsStepKind::Compare, Depth, GitsAst::Unparse(N) + TEXT(" -> ") + GitsValue::Repr(Out), false);
			}
			case EGitsNodeKind::Subscript:
			{
				FGitsValue V, I;
				if (!Eval(N->Value, Depth + 1, V)) { return false; }
				if (!Eval(N->Index, Depth + 1, I)) { return false; }
				if (V.Kind != EGitsValueKind::List && V.Kind != EGitsValueKind::Str)
				{
					return Error(EGitsDiagnosticCode::NotAList, N->Span, FGitsDiagnosticParams(GitsAst::Unparse(N->Value)));
				}
				if (I.Kind != EGitsValueKind::Int && I.Kind != EGitsValueKind::Bool)
				{
					return Error(EGitsDiagnosticCode::BadIndexType, N->Span, FGitsDiagnosticParams(GitsValue::TypeName(I)));
				}
				const int64 Len = V.Kind == EGitsValueKind::List ? (int64)V.List->Items.Num() : (int64)V.Str.Len();
				int64 Idx = I.AsInt();
				const int64 Asked = Idx;
				if (Idx < 0) { Idx += Len; }
				if (Idx < 0 || Idx >= Len)
				{
					return Error(EGitsDiagnosticCode::IndexOutOfRange, N->Span,
						FGitsDiagnosticParams(FString::Printf(TEXT("%lld"), Asked), Plural(Len, TEXT("item"), TEXT("items"))));
				}
				Out = V.Kind == EGitsValueKind::List ? V.List->Items[(int32)Idx] : FGitsValue::MakeStr(FString::Chr(V.Str[(int32)Idx]));
				return Emit(N, EGitsStepKind::Subscript, Depth, GitsAst::Unparse(N) + TEXT(" -> ") + GitsValue::Repr(Out), false);
			}
			case EGitsNodeKind::Call:
				return EvalCall(N, Depth, Out);
			case EGitsNodeKind::Method:
				return EvalMethod(N, Depth, Out);
			default:
				Out = FGitsValue::MakeNone();
				return true;
			}
		}

		// --- operators ------------------------------------------------------------------------

		static FGitsValue NumericResult(const FGitsValue& A, const FGitsValue& B, int64 IntResult, double FloatResult)
		{
			const bool bFloat = A.Kind == EGitsValueKind::Float || B.Kind == EGitsValueKind::Float;
			return bFloat ? FGitsValue::MakeFloat(FloatResult) : FGitsValue::MakeInt(IntResult);
		}

		static FString Verb(const FString& Op)
		{
			if (Op == TEXT("+")) { return TEXT("add"); }
			if (Op == TEXT("-")) { return TEXT("subtract"); }
			if (Op == TEXT("*")) { return TEXT("multiply"); }
			if (Op == TEXT("/") || Op == TEXT("//")) { return TEXT("divide"); }
			if (Op == TEXT("%")) { return TEXT("take the remainder of"); }
			return TEXT("raise to a power");
		}

		bool Mismatch(const FString& Op, const FGitsValue& A, const FGitsValue& B, const FGitsSpan& Span)
		{
			const FString Joiner = Op == TEXT("+") ? TEXT(" to ") : Op == TEXT("-") ? TEXT(" from ") : Op == TEXT("*") ? TEXT(" by ") : Op == TEXT("**") ? TEXT(" by ") : TEXT(" by ");
			FString Subject;
			if (Op == TEXT("-")) { Subject = FString::Printf(TEXT("subtract %s from %s"), *GitsValue::TypeName(B), *GitsValue::TypeName(A)); }
			else { Subject = Verb(Op) + TEXT(" ") + GitsValue::TypeName(A) + Joiner + GitsValue::TypeName(B); }
			return Error(EGitsDiagnosticCode::TypeMismatch, Span, FGitsDiagnosticParams(Subject));
		}

		bool BinaryOp(const FString& Op, const FGitsValue& A, const FGitsValue& B, const FGitsSpan& Span, FGitsValue& Out)
		{
			const bool bBothNumeric = A.IsNumeric() && B.IsNumeric();
			if (Op == TEXT("+"))
			{
				if (bBothNumeric) { Out = NumericResult(A, B, A.AsInt() + B.AsInt(), A.AsDouble() + B.AsDouble()); return true; }
				if (A.Kind == EGitsValueKind::Str && B.Kind == EGitsValueKind::Str) { Out = FGitsValue::MakeStr(A.Str + B.Str); return true; }
				if (A.Kind == EGitsValueKind::List && B.Kind == EGitsValueKind::List)
				{
					TSharedPtr<FGitsList> L = MakeShared<FGitsList>(); L->Id = NextListId++;
					L->Items = A.List->Items; L->Items.Append(B.List->Items);
					Out = FGitsValue::MakeList(L); return true;
				}
				return Mismatch(Op, A, B, Span);
			}
			if (Op == TEXT("-"))
			{
				if (bBothNumeric) { Out = NumericResult(A, B, A.AsInt() - B.AsInt(), A.AsDouble() - B.AsDouble()); return true; }
				return Mismatch(Op, A, B, Span);
			}
			if (Op == TEXT("*"))
			{
				if (bBothNumeric) { Out = NumericResult(A, B, A.AsInt() * B.AsInt(), A.AsDouble() * B.AsDouble()); return true; }
				const FGitsValue* S = nullptr; const FGitsValue* Nn = nullptr;
				if (A.Kind == EGitsValueKind::Str && (B.Kind == EGitsValueKind::Int || B.Kind == EGitsValueKind::Bool)) { S = &A; Nn = &B; }
				if (B.Kind == EGitsValueKind::Str && (A.Kind == EGitsValueKind::Int || A.Kind == EGitsValueKind::Bool)) { S = &B; Nn = &A; }
				if (S)
				{
					FString R; const int64 Times = Nn->AsInt();
					for (int64 i = 0; i < Times; ++i) { R += S->Str; }
					Out = FGitsValue::MakeStr(R); return true;
				}
				const FGitsValue* Li = nullptr;
				if (A.Kind == EGitsValueKind::List && (B.Kind == EGitsValueKind::Int || B.Kind == EGitsValueKind::Bool)) { Li = &A; Nn = &B; }
				if (B.Kind == EGitsValueKind::List && (A.Kind == EGitsValueKind::Int || A.Kind == EGitsValueKind::Bool)) { Li = &B; Nn = &A; }
				if (Li)
				{
					TSharedPtr<FGitsList> L = MakeShared<FGitsList>(); L->Id = NextListId++;
					for (int64 i = 0; i < Nn->AsInt(); ++i) { L->Items.Append(Li->List->Items); }
					Out = FGitsValue::MakeList(L); return true;
				}
				return Mismatch(Op, A, B, Span);
			}
			if (Op == TEXT("/"))
			{
				if (!bBothNumeric) { return Mismatch(Op, A, B, Span); }
				if (B.AsDouble() == 0.0) { return Error(EGitsDiagnosticCode::DivisionByZero, Span); }
				Out = FGitsValue::MakeFloat(A.AsDouble() / B.AsDouble()); return true;
			}
			if (Op == TEXT("//"))
			{
				if (!bBothNumeric) { return Mismatch(Op, A, B, Span); }
				if (B.AsDouble() == 0.0) { return Error(EGitsDiagnosticCode::DivisionByZero, Span); }
				if (A.Kind != EGitsValueKind::Float && B.Kind != EGitsValueKind::Float)
				{
					const int64 X = A.AsInt(), Y = B.AsInt();
					int64 Q = X / Y;
					if ((X % Y != 0) && ((X < 0) != (Y < 0))) { --Q; }
					Out = FGitsValue::MakeInt(Q);
				}
				else { Out = FGitsValue::MakeFloat(std::floor(A.AsDouble() / B.AsDouble())); }
				return true;
			}
			if (Op == TEXT("%"))
			{
				if (!bBothNumeric) { return Mismatch(Op, A, B, Span); }
				if (B.AsDouble() == 0.0) { return Error(EGitsDiagnosticCode::DivisionByZero, Span); }
				if (A.Kind != EGitsValueKind::Float && B.Kind != EGitsValueKind::Float)
				{
					const int64 X = A.AsInt(), Y = B.AsInt();
					int64 R = X % Y;
					if (R != 0 && ((R < 0) != (Y < 0))) { R += Y; }
					Out = FGitsValue::MakeInt(R);
				}
				else
				{
					const double X = A.AsDouble(), Y = B.AsDouble();
					double R = std::fmod(X, Y);
					if (R != 0.0 && ((R < 0) != (Y < 0))) { R += Y; }
					Out = FGitsValue::MakeFloat(R);
				}
				return true;
			}
			if (Op == TEXT("**"))
			{
				if (!bBothNumeric) { return Mismatch(Op, A, B, Span); }
				if (A.Kind != EGitsValueKind::Float && B.Kind != EGitsValueKind::Float && B.AsInt() >= 0)
				{
					int64 R = 1; const int64 Base = A.AsInt();
					for (int64 i = 0; i < B.AsInt(); ++i) { R *= Base; }
					Out = FGitsValue::MakeInt(R);
				}
				else { Out = FGitsValue::MakeFloat(std::pow(A.AsDouble(), B.AsDouble())); }
				return true;
			}
			return Mismatch(Op, A, B, Span);
		}

		bool Compare(const FGitsNodePtr& N, const FGitsValue& L, const FGitsValue& R, bool& Result)
		{
			const FString& Op = N->Text;
			if (Op == TEXT("==")) { Result = GitsValue::Equal(L, R); return true; }
			if (Op == TEXT("!=")) { Result = !GitsValue::Equal(L, R); return true; }
			if (Op == TEXT("in"))
			{
				if (R.Kind == EGitsValueKind::List)
				{
					Result = false;
					for (const FGitsValue& Item : R.List->Items) { if (GitsValue::Equal(Item, L)) { Result = true; break; } }
					return true;
				}
				if (R.Kind == EGitsValueKind::Str && L.Kind == EGitsValueKind::Str) { Result = R.Str.Contains(L.Str) || L.Str.IsEmpty(); return true; }
				return Error(EGitsDiagnosticCode::NotAList, N->Span, FGitsDiagnosticParams(GitsAst::Unparse(N->Right)));
			}
			int32 Cmp = 0;
			if (L.IsNumeric() && R.IsNumeric())
			{
				if (L.Kind != EGitsValueKind::Float && R.Kind != EGitsValueKind::Float) { Cmp = L.AsInt() < R.AsInt() ? -1 : L.AsInt() > R.AsInt() ? 1 : 0; }
				else { Cmp = L.AsDouble() < R.AsDouble() ? -1 : L.AsDouble() > R.AsDouble() ? 1 : 0; }
			}
			else if (L.Kind == EGitsValueKind::Str && R.Kind == EGitsValueKind::Str) { Cmp = L.Str.Compare(R.Str, ESearchCase::CaseSensitive); Cmp = Cmp < 0 ? -1 : Cmp > 0 ? 1 : 0; }
			else
			{
				return Error(EGitsDiagnosticCode::TypeMismatch, N->Span,
					FGitsDiagnosticParams(FString::Printf(TEXT("compare %s with %s"), *GitsValue::TypeName(L), *GitsValue::TypeName(R))));
			}
			if (Op == TEXT("<")) { Result = Cmp < 0; }
			else if (Op == TEXT(">")) { Result = Cmp > 0; }
			else if (Op == TEXT("<=")) { Result = Cmp <= 0; }
			else { Result = Cmp >= 0; }
			return true;
		}

		// --- calls ----------------------------------------------------------------------------

		static FString ArgList(const TArray<FGitsValue>& Args)
		{
			FString Out;
			for (int32 i = 0; i < Args.Num(); ++i) { if (i > 0) { Out += TEXT(", "); } Out += GitsValue::Repr(Args[i]); }
			return Out;
		}

		bool CheckArity(const FString& Name, int32 Given, int32 Min, int32 Max, const FGitsSpan& Span)
		{
			if (Given >= Min && Given <= Max) { return true; }
			const FString Takes = Min == Max ? Plural(Min, TEXT("value"), TEXT("values")) : FString::Printf(TEXT("%d to %d values"), Min, Max);
			return Error(EGitsDiagnosticCode::WrongArgumentCount, Span,
				FGitsDiagnosticParams(Name + TEXT("()"), FString::Printf(TEXT("%s, and it takes %s"), *Plural(Given, TEXT("value"), TEXT("values")), *Takes)));
		}

		bool StationAllowed(const FString& Name) const
		{
			return !Options.bRestrictBuiltins || Options.Builtins.Contains(Name);
		}

		bool EvalCall(const FGitsNodePtr& N, int32 Depth, FGitsValue& Out)
		{
			const bool bWasForHeader = bInForHeader;
			bInForHeader = false;

			// Resolve the callee. A user definition shadows a builtin, as Python does.
			FGitsValue Callee;
			bool bUserFunction = false;
			FString Name;
			if (N->Func->Kind == EGitsNodeKind::Name)
			{
				Name = N->Func->Text;
				if (FGitsValue* V = Lookup(Name))
				{
					if (V->Kind != EGitsValueKind::Function) { return Error(EGitsDiagnosticCode::UnknownFunction, N->Span, FGitsDiagnosticParams(Name + TEXT("()"))); }
					Callee = *V; bUserFunction = true;
				}
			}
			else
			{
				if (!Eval(N->Func, Depth + 1, Callee)) { return false; }
				if (Callee.Kind != EGitsValueKind::Function) { return Error(EGitsDiagnosticCode::UnknownFunction, N->Span, FGitsDiagnosticParams(GitsAst::Unparse(N->Func) + TEXT("()"))); }
				bUserFunction = true;
			}

			TArray<FGitsValue> Args;
			for (const FGitsNodePtr& A : N->Args)
			{
				FGitsValue V;
				if (!Eval(A, Depth + 1, V)) { return false; }
				Args.Add(V);
			}

			const FString Callable = GitsAst::Unparse(N->Func);
			auto Label = [&](const FGitsValue& Result) { return Callable + TEXT("(") + ArgList(Args) + TEXT(") -> ") + GitsValue::Repr(Result); };

			if (bUserFunction)
			{
				if (Frames.Num() + 1 > GitsLimits::MaxCallDepth)
				{
					return Error(EGitsDiagnosticCode::CallDepthExceeded, N->Span, FGitsDiagnosticParams(FString::FromInt(GitsLimits::MaxCallDepth)));
				}
				if (!CheckArity(Callee.FunctionName, Args.Num(), Callee.Params.Num(), Callee.Params.Num(), N->Span)) { return false; }
				const FGitsNodePtr* Def = Definitions.Find(Callee.FunctionNodeId);
				if (!Def) { return Error(EGitsDiagnosticCode::UnknownFunction, N->Span, FGitsDiagnosticParams(Callee.FunctionName + TEXT("()"))); }
				FFrame Frame;
				Frame.FunctionName = Callee.FunctionName;
				Frame.bHasCallSite = true;
				Frame.CallSite = N->Span;
				for (int32 i = 0; i < Callee.Params.Num(); ++i) { Frame.Bindings.Emplace(Callee.Params[i], Args[i]); }
				Frames.Add(MoveTemp(Frame));
				ReturnValue = FGitsValue::MakeNone();
				const EFlow F = ExecBlock((*Def)->Body);
				Frames.Pop();
				if (F == EFlow::Abort || bStopped) { return false; }
				Out = F == EFlow::Return ? ReturnValue : FGitsValue::MakeNone();
				ReturnValue = FGitsValue::MakeNone();
				return Emit(N, EGitsStepKind::Call, Depth, Label(Out), false);
			}

			// --- builtins
			if (Name == TEXT("print"))
			{
				FString Line;
				for (int32 i = 0; i < Args.Num(); ++i) { if (i > 0) { Line += TEXT(" "); } Line += GitsValue::Display(Args[i]); }
				Out = FGitsValue::MakeNone();
				TArray<FString> Output; Output.Add(Line);
				return Emit(N, EGitsStepKind::Call, Depth, Label(Out), false, &Output);
			}
			if (Name == TEXT("len"))
			{
				if (!CheckArity(Name, Args.Num(), 1, 1, N->Span)) { return false; }
				const FGitsValue& A = Args[0];
				if (A.Kind == EGitsValueKind::List) { Out = FGitsValue::MakeInt(A.List->Items.Num()); }
				else if (A.Kind == EGitsValueKind::Str) { Out = FGitsValue::MakeInt(A.Str.Len()); }
				else { return Error(EGitsDiagnosticCode::TypeMismatch, N->Span, FGitsDiagnosticParams(TEXT("find the length of ") + GitsValue::TypeName(A))); }
				return Emit(N, EGitsStepKind::Call, Depth, Label(Out), false);
			}
			if (Name == TEXT("int") || Name == TEXT("float"))
			{
				if (!CheckArity(Name, Args.Num(), 1, 1, N->Span)) { return false; }
				const FGitsValue& A = Args[0];
				const bool bInt = Name == TEXT("int");
				const FString Target = bInt ? TEXT("a whole number") : TEXT("a decimal number");
				if (A.IsNumeric()) { Out = bInt ? FGitsValue::MakeInt(A.Kind == EGitsValueKind::Float ? (int64)A.Float : A.AsInt()) : FGitsValue::MakeFloat(A.AsDouble()); }
				else if (A.Kind == EGitsValueKind::Str)
				{
					const FString T = A.Str.TrimStartAndEnd();
					if (bInt && T.IsNumeric() && !T.Contains(TEXT(".")) && !T.Contains(TEXT("e"))) { Out = FGitsValue::MakeInt(FCString::Atoi64(*T)); }
					else if (!bInt && T.IsNumeric()) { Out = FGitsValue::MakeFloat(FCString::Atod(*T)); }
					else
					{
						return Error(EGitsDiagnosticCode::TypeMismatch, N->Span,
							FGitsDiagnosticParams(FString::Printf(TEXT("turn the text '%s' into %s"), *A.Str, *Target)));
					}
				}
				else { return Error(EGitsDiagnosticCode::TypeMismatch, N->Span, FGitsDiagnosticParams(FString::Printf(TEXT("turn %s into %s"), *GitsValue::TypeName(A), *Target))); }
				return Emit(N, EGitsStepKind::Call, Depth, Label(Out), false);
			}
			if (Name == TEXT("str"))
			{
				if (!CheckArity(Name, Args.Num(), 1, 1, N->Span)) { return false; }
				Out = FGitsValue::MakeStr(GitsValue::Display(Args[0]));
				return Emit(N, EGitsStepKind::Call, Depth, Label(Out), false);
			}
			if (Name == TEXT("bool"))
			{
				if (!CheckArity(Name, Args.Num(), 1, 1, N->Span)) { return false; }
				Out = FGitsValue::MakeBool(GitsValue::Truthy(Args[0]));
				return Emit(N, EGitsStepKind::Call, Depth, Label(Out), false);
			}
			if (Name == TEXT("range"))
			{
				if (!bWasForHeader) { return Error(EGitsDiagnosticCode::BadRange, N->Span, FGitsDiagnosticParams(TEXT("outside a for loop"))); }
				if (!CheckArity(Name, Args.Num(), 1, 3, N->Span)) { return false; }
				for (const FGitsValue& A : Args)
				{
					if (A.Kind != EGitsValueKind::Int && A.Kind != EGitsValueKind::Bool)
					{
						return Error(EGitsDiagnosticCode::BadRange, N->Span, FGitsDiagnosticParams(TEXT("with ") + GitsValue::TypeName(A)));
					}
				}
				int64 Start = 0, Stop = 0, Step = 1;
				if (Args.Num() == 1) { Stop = Args[0].AsInt(); }
				else { Start = Args[0].AsInt(); Stop = Args[1].AsInt(); if (Args.Num() == 3) { Step = Args[2].AsInt(); } }
				if (Step == 0) { return Error(EGitsDiagnosticCode::BadRange, N->Span, FGitsDiagnosticParams(TEXT("in steps of 0"))); }
				RangeValues.Empty();
				for (int64 i = Start; Step > 0 ? i < Stop : i > Stop; i += Step) { RangeValues.Add(FGitsValue::MakeInt(i)); }
				bRangeProduced = true;
				Out = FGitsValue::MakeNone();
				return Emit(N, EGitsStepKind::Call, Depth, FString::Printf(TEXT("%s(%s) -> %d values"), *Callable, *ArgList(Args), RangeValues.Num()), false);
			}

			// --- station builtins
			const bool bStation = Name == TEXT("open_valve") || Name == TEXT("close_valve") || Name == TEXT("set_heater")
				|| Name == TEXT("log") || Name == TEXT("wait") || Name == TEXT("read_sensor")
				|| Name == TEXT("open_door") || Name == TEXT("close_door") || Name == TEXT("set_light");
			if (bStation && StationAllowed(Name))
			{
				if (Name == TEXT("set_light"))
				{
					if (!CheckArity(Name, Args.Num(), 2, 2, N->Span)) { return false; }
					if (Args[0].Kind != EGitsValueKind::Str) { return Error(EGitsDiagnosticCode::TypeMismatch, N->Span, FGitsDiagnosticParams(TEXT("name a light with ") + GitsValue::TypeName(Args[0]))); }
					if (!Args[1].IsNumeric()) { return Error(EGitsDiagnosticCode::TypeMismatch, N->Span, FGitsDiagnosticParams(TEXT("set a light to ") + GitsValue::TypeName(Args[1]))); }
					TArray<FGitsEffect> LightFx;
					LightFx.Add(FGitsEffect::MakeSet(TEXT("light.") + Args[0].Str, FGitsWorldValue::MakeNumber(Args[1].AsDouble())));
					for (const FGitsEffect& E : LightFx) { World = GitsWorld::Reduce(World, E); }
					Out = FGitsValue::MakeNone();
					return Emit(N, EGitsStepKind::Call, Depth, Label(Out), false, nullptr, &LightFx);
				}
				if (!CheckArity(Name, Args.Num(), 1, 1, N->Span)) { return false; }
				const FGitsValue& A = Args[0];
				Out = FGitsValue::MakeNone();
				if (Name == TEXT("read_sensor"))
				{
					if (A.Kind != EGitsValueKind::Str) { return Error(EGitsDiagnosticCode::TypeMismatch, N->Span, FGitsDiagnosticParams(TEXT("read a sensor named by ") + GitsValue::TypeName(A))); }
					FGitsOracleRead Read;
					Read.Query.Kind = TEXT("sensor");
					Read.Query.Id = A.Str;
					Read.Value = Oracle ? Oracle(World, Read.Query) : FGitsValue::MakeNone();
					Out = Read.Value;
					return Emit(N, EGitsStepKind::Call, Depth, Label(Out), false, nullptr, nullptr, &Read);
				}
				TArray<FGitsEffect> Effects;
				if (Name == TEXT("open_valve") || Name == TEXT("close_valve"))
				{
					if (A.Kind != EGitsValueKind::Str) { return Error(EGitsDiagnosticCode::TypeMismatch, N->Span, FGitsDiagnosticParams(TEXT("name a valve with ") + GitsValue::TypeName(A))); }
					Effects.Add(FGitsEffect::MakeSet(TEXT("valve.") + A.Str, FGitsWorldValue::MakeBool(Name == TEXT("open_valve"))));
				}
				else if (Name == TEXT("open_door") || Name == TEXT("close_door"))
				{
					if (A.Kind != EGitsValueKind::Str) { return Error(EGitsDiagnosticCode::TypeMismatch, N->Span, FGitsDiagnosticParams(TEXT("name a door with ") + GitsValue::TypeName(A))); }
					Effects.Add(FGitsEffect::MakeSet(TEXT("door.") + A.Str, FGitsWorldValue::MakeBool(Name == TEXT("open_door"))));
				}
				else if (Name == TEXT("set_light"))
				{
					// set_light(id, level): two arguments, checked below after the one-argument gate.
				}
				else if (Name == TEXT("set_heater"))
				{
					if (!A.IsNumeric()) { return Error(EGitsDiagnosticCode::TypeMismatch, N->Span, FGitsDiagnosticParams(TEXT("set the heater to ") + GitsValue::TypeName(A))); }
					Effects.Add(FGitsEffect::MakeSet(TEXT("heater"), FGitsWorldValue::MakeNumber(A.AsDouble())));
				}
				else if (Name == TEXT("log"))
				{
					Effects.Add(FGitsEffect::MakeLog(GitsValue::Display(A)));
				}
				else if (Name == TEXT("wait"))
				{
					if (A.Kind != EGitsValueKind::Int && A.Kind != EGitsValueKind::Bool) { return Error(EGitsDiagnosticCode::TypeMismatch, N->Span, FGitsDiagnosticParams(TEXT("wait for ") + GitsValue::TypeName(A))); }
					Effects.Add(FGitsEffect::MakeWait(A.AsInt()));
				}
				for (const FGitsEffect& E : Effects) { World = GitsWorld::Reduce(World, E); }
				return Emit(N, EGitsStepKind::Call, Depth, Label(Out), false, nullptr, &Effects);
			}

			return Error(EGitsDiagnosticCode::UnknownFunction, N->Span, FGitsDiagnosticParams(Callable + TEXT("()")));
		}

		bool EvalMethod(const FGitsNodePtr& N, int32 Depth, FGitsValue& Out)
		{
			FGitsValue Receiver;
			if (!Eval(N->Receiver, Depth + 1, Receiver)) { return false; }
			TArray<FGitsValue> Args;
			for (const FGitsNodePtr& A : N->Args)
			{
				FGitsValue V;
				if (!Eval(A, Depth + 1, V)) { return false; }
				Args.Add(V);
			}
			if (N->Text == TEXT("append"))
			{
				if (Receiver.Kind != EGitsValueKind::List) { return Error(EGitsDiagnosticCode::NotAList, N->Span, FGitsDiagnosticParams(GitsAst::Unparse(N->Receiver))); }
				if (!CheckArity(TEXT("append"), Args.Num(), 1, 1, N->Span)) { return false; }
				Receiver.List->Items.Add(Args[0]);
				Out = FGitsValue::MakeNone();
				const FString Label = GitsAst::Unparse(N->Receiver) + TEXT(".append(") + ArgList(Args) + TEXT(") -> None");
				return Emit(N, EGitsStepKind::Method, Depth, Label, false);
			}
			return Error(EGitsDiagnosticCode::MethodNotAvailable, N->Span, FGitsDiagnosticParams(N->Text + TEXT("()")));
		}
	};
}

namespace GitsEvaluator
{
	FGitsTrace Run(const FGitsProgram& Program, const FGitsWorldState& InitialWorld, FGitsWorldOracle Oracle, const FGitsRunOptions& Options)
	{
		FEvaluator E(Program, InitialWorld, Oracle, Options);
		return E.Run();
	}

	FGitsTrace RunSource(const FString& Source, const FGitsWorldState& InitialWorld, FGitsWorldOracle Oracle, const FGitsRunOptions& Options, EGitsTier Tier)
	{
		FGitsParseOptions PO; PO.Tier = Tier;
		FGitsParseResult Parsed = GitsParser::Parse(Source, PO);
		if (Parsed.Diagnostics.Num() > 0)
		{
			FGitsTrace T;
			T.InitialWorld = InitialWorld;
			T.Outcome.Kind = EGitsOutcomeKind::Error;
			T.Outcome.Diagnostic = Parsed.Diagnostics[0];
			return T;
		}
		return Run(Parsed.Program, InitialWorld, Oracle, Options);
	}

	const TArray<FString>& StationBuiltinNames()
	{
		static const TArray<FString> Names = { TEXT("open_valve"), TEXT("close_valve"), TEXT("set_heater"), TEXT("log"), TEXT("wait"), TEXT("read_sensor"),
			TEXT("open_door"), TEXT("close_door"), TEXT("set_light") };
		return Names;
	}
}
