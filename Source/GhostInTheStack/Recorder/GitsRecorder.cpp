#include "GitsRecorder.h"
#include "Interpreter/GitsTrace.h"

FString FGitsLoopContext::Describe() const
{
	if (bEnded) { return FString::Printf(TEXT("%s: loop ends after %d"), *Header, Total); }
	if (Total == 0) { return FString::Printf(TEXT("%s: 0 iterations"), *Header); }
	return FString::Printf(TEXT("%s: iteration %d of %d"), *Header, Iteration, Total);
}

void FGitsRecorder::Clear()
{
	Bounds.Reset();
	StepToBeat.Reset();
	BeatLoops.Reset();
	LastStep = -1;
}

namespace
{
	bool SpanInside(const FGitsSpan& Inner, const FGitsSpan& Outer)
	{
		return Inner.Start.Offset >= Outer.Start.Offset && Inner.End.Offset <= Outer.End.Offset;
	}

	/** First beat at or after From whose boundary is outside LoopSpan (exclusive end of the loop's extent). */
	int32 ExtentEnd(const FGitsTrace& Trace, const TArray<int32>& Bounds, int32 From, const FGitsSpan& LoopSpan)
	{
		int32 Scan = From;
		while (Scan < Bounds.Num() && SpanInside(Trace.Steps[Bounds[Scan]].Span, LoopSpan)) { ++Scan; }
		return Scan;
	}

	/** "for i=3 (iteration 3)" -> "for i"; "while x < 3 -> True (iteration 2)" -> "while x < 3". */
	FString HeaderOf(const FString& Label)
	{
		int32 Cut = Label.Len();
		for (const TCHAR* Marker : { TEXT(" (iteration"), TEXT(" -> "), TEXT(": loop ends") })
		{
			const int32 P = Label.Find(Marker);
			if (P != INDEX_NONE && P < Cut) { Cut = P; }
		}
		FString Header = Label.Left(Cut);
		if (Header.StartsWith(TEXT("for ")))
		{
			int32 Eq;
			if (Header.FindChar(TEXT('='), Eq)) { Header = Header.Left(Eq); }
		}
		return Header.TrimEnd();
	}
}

void FGitsRecorder::Build(const FGitsTrace& Trace, float StatementsPerSecond)
{
	Clear();
	Interval = 1.f / FMath::Max(0.1f, StatementsPerSecond);
	LastStep = Trace.Steps.Num() - 1;
	Bounds = GitsTrace::StatementBoundaries(Trace);

	// Every step belongs to the beat of the first boundary at or after it: a statement's
	// expression steps come before the boundary that closes the statement. Steps after the
	// last boundary (a statement cut short by an error) ride on the last beat.
	StepToBeat.SetNum(Trace.Steps.Num());
	int32 Beat = 0;
	for (int32 i = 0; i < Trace.Steps.Num(); ++i)
	{
		while (Beat < Bounds.Num() && Bounds[Beat] < i) { ++Beat; }
		StepToBeat[i] = Beat < Bounds.Num() ? Beat : Bounds.Num() - 1;
	}

	// Loop stack over the beats. A loop opens at its first iterate boundary and owns every
	// boundary inside its span; nested loops open inside it and close first.
	struct FOpen { FGitsLoopContext Ctx; int32 EndBeat = 0; };
	TArray<FOpen> Stack;
	BeatLoops.SetNum(Bounds.Num());
	for (int32 b = 0; b < Bounds.Num(); ++b)
	{
		const FGitsStep& Step = Trace.Steps[Bounds[b]];
		while (Stack.Num() > 0 && b >= Stack.Last().EndBeat) { Stack.Pop(); }
		if (Step.Kind == EGitsStepKind::Iterate)
		{
			const bool bEnds = Step.Label.Contains(TEXT("loop ends"));
			if (Stack.Num() > 0 && Stack.Last().Ctx.NodeId == Step.NodeId)
			{
				FGitsLoopContext& C = Stack.Last().Ctx;
				if (bEnds) { C.bEnded = true; C.Iteration = C.Total; }
				else { ++C.Iteration; }
			}
			else
			{
				FOpen Open;
				Open.EndBeat = ExtentEnd(Trace, Bounds, b, Step.Span);
				int32 Iterates = 0; bool bSawEnd = false;
				for (int32 k = b; k < Open.EndBeat; ++k)
				{
					const FGitsStep& Inner = Trace.Steps[Bounds[k]];
					if (Inner.Kind == EGitsStepKind::Iterate && Inner.NodeId == Step.NodeId)
					{
						++Iterates;
						if (Inner.Label.Contains(TEXT("loop ends"))) { bSawEnd = true; }
					}
				}
				Open.Ctx.NodeId = Step.NodeId;
				Open.Ctx.Header = HeaderOf(Step.Label);
				Open.Ctx.Line = Step.Span.Start.Line;
				Open.Ctx.Total = FMath::Max(0, bSawEnd ? Iterates - 1 : Iterates);
				Open.Ctx.Iteration = bEnds ? 0 : 1;
				Open.Ctx.bEnded = bEnds;
				Stack.Add(Open);
			}
		}
		for (const FOpen& O : Stack) { BeatLoops[b].Add(O.Ctx); }
	}
}

int32 FGitsRecorder::StepAtTime(float T) const
{
	if (Bounds.Num() == 0) { return -1; }
	const int32 Beat = FMath::FloorToInt(T / Interval) - 1;
	if (Beat < 0) { return -1; }
	if (Beat >= Bounds.Num()) { return LastStep; }
	return Bounds[Beat];
}

int32 FGitsRecorder::BeatOfStep(int32 StepIndex) const
{
	return StepToBeat.IsValidIndex(StepIndex) ? StepToBeat[StepIndex] : -1;
}

float FGitsRecorder::TimeOfStep(int32 StepIndex) const
{
	const int32 Beat = BeatOfStep(StepIndex);
	return Beat < 0 ? 0.f : (Beat + 1) * Interval;
}

TArray<FGitsLoopContext> FGitsRecorder::LoopsAt(int32 StepIndex) const
{
	const int32 Beat = BeatOfStep(StepIndex);
	return BeatLoops.IsValidIndex(Beat) ? BeatLoops[Beat] : TArray<FGitsLoopContext>();
}
