// Recorder tests: the clock <-> step mapping and the loop contexts the display shows.
#include "Interpreter/Tests/GitsTestUtil.h"
#include "Interpreter/GitsTrace.h"
#include "Recorder/GitsRecorder.h"

#if WITH_DEV_AUTOMATION_TESTS

using namespace GitsTest;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGitsRecorderLoops, "GhostInTheStack.Recorder.Loops", GitsTest::TestFlags)
bool FGitsRecorderLoops::RunTest(const FString&)
{
	// The corridor lights script in miniature: a loop inside a loop, then a statement after.
	const FString Source = TEXT("level = 0\nfor notch in range(3):\n    level = level + 1\n    for tick in range(2):\n        wait(1)\nlog(\"done\")\n");
	FGitsTrace T = Exec(Source);
	TestEqual(TEXT("completes"), (int32)T.Outcome.Kind, (int32)EGitsOutcomeKind::Completed);

	FGitsRecorder R;
	R.Build(T, 2.5f);
	TestTrue(TEXT("built"), R.IsBuilt());
	const TArray<int32> Bounds = GitsTrace::StatementBoundaries(T);
	TestEqual(TEXT("one beat per boundary"), R.NumBeats(), Bounds.Num());
	TestTrue(TEXT("beat interval"), FMath::IsNearlyEqual(R.BeatInterval(), 0.4f));

	// The clock: nothing before the first beat, boundaries as beats land, the last step at the end.
	TestEqual(TEXT("before the first beat"), R.StepAtTime(0.1f), -1);
	TestEqual(TEXT("first beat"), R.StepAtTime(0.4f), Bounds[0]);
	TestEqual(TEXT("second beat"), R.StepAtTime(0.85f), Bounds[1]);
	TestEqual(TEXT("last beat"), R.StepAtTime(R.TotalTime() - 0.01f), Bounds.Last());
	TestEqual(TEXT("run over"), R.StepAtTime(R.TotalTime()), T.Steps.Num() - 1);
	// Every step belongs to a beat, and beats never go backwards along the trace.
	int32 PrevBeat = -1;
	for (int32 i = 0; i < T.Steps.Num(); ++i)
	{
		const int32 Beat = R.BeatOfStep(i);
		TestTrue(TEXT("step has a beat"), Beat >= 0 || i < Bounds[0]);
		TestTrue(TEXT("beats are monotone"), Beat >= PrevBeat);
		PrevBeat = FMath::Max(PrevBeat, Beat);
	}
	TestTrue(TEXT("time of a boundary is its beat"), FMath::IsNearlyEqual(R.TimeOfStep(Bounds[2]), 3 * 0.4f));

	// Loop contexts: the outer loop counts 1..3, the inner 1..2 inside it, nothing after.
	int32 OuterSeen = 0, InnerBodySeen = 0, InnerEndsSeen = 0;
	for (int32 b : Bounds)
	{
		const FGitsStep& S = T.Steps[b];
		const TArray<FGitsLoopContext> L = R.LoopsAt(b);
		if (S.Kind == EGitsStepKind::Iterate && S.Label.StartsWith(TEXT("for notch=")))
		{
			++OuterSeen;
			TestEqual(TEXT("outer: one loop deep"), L.Num(), 1);
			if (L.Num() == 1)
			{
				TestEqual(TEXT("outer header"), L[0].Header, FString(TEXT("for notch")));
				TestEqual(TEXT("outer iteration"), L[0].Iteration, OuterSeen);
				TestEqual(TEXT("outer total"), L[0].Total, 3);
				TestEqual(TEXT("outer line"), L[0].Line, 2);
				TestEqual(TEXT("outer describes"), L[0].Describe(), FString::Printf(TEXT("for notch: iteration %d of 3"), OuterSeen));
			}
		}
		if (S.Label.StartsWith(TEXT("wait(")))
		{
			++InnerBodySeen;
			TestEqual(TEXT("inner body: two loops deep"), L.Num(), 2);
			if (L.Num() == 2)
			{
				TestEqual(TEXT("outer first"), L[0].Header, FString(TEXT("for notch")));
				TestEqual(TEXT("inner second"), L[1].Header, FString(TEXT("for tick")));
				TestEqual(TEXT("inner total"), L[1].Total, 2);
				TestEqual(TEXT("inner iteration"), L[1].Iteration, ((InnerBodySeen - 1) % 2) + 1);
				TestEqual(TEXT("outer iteration from inside"), L[0].Iteration, ((InnerBodySeen - 1) / 2) + 1);
			}
		}
		if (S.Label.Contains(TEXT("for tick: loop ends")))
		{
			++InnerEndsSeen;
			TestTrue(TEXT("inner ended"), L.Num() == 2 && L[1].bEnded);
			if (L.Num() == 2) { TestEqual(TEXT("inner ends describes"), L[1].Describe(), FString(TEXT("for tick: loop ends after 2"))); }
		}
		if (S.Label.StartsWith(TEXT("log("))) { TestEqual(TEXT("after the loops"), L.Num(), 0); }
	}
	TestEqual(TEXT("three outer iterations"), OuterSeen, 3);
	TestEqual(TEXT("six inner bodies"), InnerBodySeen, 6);
	TestEqual(TEXT("three inner endings"), InnerEndsSeen, 3);

	// A loop that never runs reads as 0 iterations; a while loop keeps its test as the header.
	{
		FGitsTrace T2 = Exec(TEXT("for i in range(0):\n    wait(1)\nn = 2\nwhile n > 0:\n    n = n - 1\n"));
		FGitsRecorder R2; R2.Build(T2, 2.5f);
		const TArray<int32> B2 = GitsTrace::StatementBoundaries(T2);
		bool bSawEmpty = false, bSawWhile = false;
		for (int32 b : B2)
		{
			const TArray<FGitsLoopContext> L = R2.LoopsAt(b);
			if (T2.Steps[b].Label.StartsWith(TEXT("for i: loop ends"))) { bSawEmpty = L.Num() == 1 && L[0].Total == 0 && L[0].bEnded; }
			if (T2.Steps[b].Label.StartsWith(TEXT("while n > 0 -> True (iteration 2)"))) { bSawWhile = L.Num() == 1 && L[0].Header == TEXT("while n > 0") && L[0].Iteration == 2 && L[0].Total == 2; }
		}
		TestTrue(TEXT("empty loop"), bSawEmpty);
		TestTrue(TEXT("while header and count"), bSawWhile);
	}
	return true;
}

#endif
