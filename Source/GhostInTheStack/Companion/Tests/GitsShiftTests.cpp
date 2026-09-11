// The prediction gate, as rules: free commit, reveal at the run, lock, watch, re-answer.
#include "Interpreter/Tests/GitsTestUtil.h"
#include "Interpreter/GitsTrace.h"
#include "Companion/GitsShift.h"
#include "Companion/GitsTags.h"

#if WITH_DEV_AUTOMATION_TESTS

using namespace GitsTest;

namespace
{
	FGitsPrediction DoorPrediction()
	{
		FGitsPrediction P;
		P.Id = TEXT("p1");
		P.Prompt = TEXT("Which way does line 8 go?");
		P.AnchorLine = 8;
		P.AnchorOccurrence = 1;
		P.Kind = TEXT("branch");
		FGitsPredictionOption A; A.Id = TEXT("a"); A.Label = TEXT("holds");
		FGitsPredictionOption B; B.Id = TEXT("b"); B.Label = TEXT("opens"); B.Misconception = TEXT("assignment-as-equality");
		FGitsPredictionOption C; C.Id = TEXT("c"); C.Label = TEXT("both"); C.Misconception = TEXT("branch-both");
		P.Options = { A, B, C };
		P.CorrectId = TEXT("a");
		return P;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGitsShiftGate, "GhostInTheStack.Companion.Shift.Gate", GitsTest::TestFlags)
bool FGitsShiftGate::RunTest(const FString&)
{
	const TArray<FGitsPrediction> Predictions = { DoorPrediction() };
	FGitsShift Shift;
	Shift.Reset(Predictions);

	// Pending until committed; selecting is not committing.
	TestEqual(TEXT("pending at first"), Shift.Pending(Predictions).Num(), 1);
	TestFalse(TEXT("commit needs a selection"), Shift.Commit(TEXT("p1")));
	TestTrue(TEXT("select"), Shift.Select(TEXT("p1"), TEXT("b")));
	TestEqual(TEXT("still pending"), Shift.Pending(Predictions).Num(), 1);
	TestTrue(TEXT("commit"), Shift.Commit(TEXT("p1")));
	TestEqual(TEXT("committed is not pending"), Shift.Pending(Predictions).Num(), 0);
	TestFalse(TEXT("cannot reselect once committed"), Shift.Select(TEXT("p1"), TEXT("a")));

	// Wrong at the run: attempt counted, misconception named, locked.
	FGitsResolvedCommitment R;
	TestTrue(TEXT("resolves"), Shift.Resolve(Predictions[0], 100.0, R));
	TestFalse(TEXT("wrong"), R.bCorrect);
	TestEqual(TEXT("attempt 1"), R.Attempt, 1);
	TestEqual(TEXT("misconception"), R.Misconception, FString(TEXT("assignment-as-equality")));
	TestTrue(TEXT("locked"), Shift.StateOf(TEXT("p1")).bLocked);
	TestEqual(TEXT("locked is not pending"), Shift.Pending(Predictions).Num(), 0);
	TestFalse(TEXT("cannot select while locked"), Shift.Select(TEXT("p1"), TEXT("a")));

	// The gate: forward past the anchor alone does not release; the start first, then the anchor.
	const int32 First = 2, Anchor = 9;
	TestFalse(TEXT("past the anchor without the start"), Shift.RecordHead(TEXT("p1"), 12, First, Anchor, true));
	TestFalse(TEXT("rewinding to the anchor"), Shift.RecordHead(TEXT("p1"), 9, First, Anchor, false));
	TestFalse(TEXT("rewound to the start"), Shift.RecordHead(TEXT("p1"), -1, First, Anchor, false));
	TestTrue(TEXT("reached start recorded"), Shift.StateOf(TEXT("p1")).bReachedStartSinceLock);
	TestFalse(TEXT("forward, before the anchor"), Shift.RecordHead(TEXT("p1"), 5, First, Anchor, true));
	TestTrue(TEXT("forward through the anchor releases"), Shift.RecordHead(TEXT("p1"), 9, First, Anchor, true));
	TestFalse(TEXT("unlocked"), Shift.StateOf(TEXT("p1")).bLocked);
	TestEqual(TEXT("pending again"), Shift.Pending(Predictions).Num(), 1);
	TestFalse(TEXT("no double release"), Shift.RecordHead(TEXT("p1"), 12, First, Anchor, true));

	// Re-answer, right this time: satisfied, attempt 2, never pending again.
	TestTrue(TEXT("select again"), Shift.Select(TEXT("p1"), TEXT("a")));
	TestTrue(TEXT("commit again"), Shift.Commit(TEXT("p1")));
	TestTrue(TEXT("resolves again"), Shift.Resolve(Predictions[0], 200.0, R));
	TestTrue(TEXT("correct"), R.bCorrect);
	TestEqual(TEXT("attempt 2"), R.Attempt, 2);
	TestTrue(TEXT("satisfied"), Shift.StateOf(TEXT("p1")).bSatisfied);
	TestTrue(TEXT("all satisfied"), Shift.AllSatisfied(Predictions));
	TestFalse(TEXT("satisfied cannot select"), Shift.Select(TEXT("p1"), TEXT("b")));

	// An edit on the anchor line drops a commitment; elsewhere it does not.
	{
		FGitsShift Fresh; Fresh.Reset(Predictions);
		Fresh.Select(TEXT("p1"), TEXT("c")); Fresh.Commit(TEXT("p1"));
		Fresh.InvalidateLine(6, Predictions);
		TestEqual(TEXT("edit elsewhere keeps the commitment"), Fresh.StateOf(TEXT("p1")).Committed, FString(TEXT("c")));
		Fresh.InvalidateLine(8, Predictions);
		TestTrue(TEXT("edit on the anchor drops it"), Fresh.StateOf(TEXT("p1")).Committed.IsEmpty());
		TestEqual(TEXT("pending after the edit"), Fresh.Pending(Predictions).Num(), 1);
	}

	// A skipped prediction neither blocks nor counts against completion.
	{
		FGitsShift Fresh; Fresh.Reset(Predictions);
		Fresh.MarkSkipped(TEXT("p1"), true);
		TestEqual(TEXT("skipped is not pending"), Fresh.Pending(Predictions).Num(), 0);
		TestTrue(TEXT("skipped counts as done"), Fresh.AllSatisfied(Predictions));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGitsShiftAnchors, "GhostInTheStack.Companion.Shift.Anchors", GitsTest::TestFlags)
bool FGitsShiftAnchors::RunTest(const FString&)
{
	// Line 2 runs three times; the third occurrence is a specific step, the fourth never happens.
	FGitsTrace T = Exec(TEXT("for i in range(3):\n    wait(1)\nlog(\"done\")\n"));
	int32 Found = 0;
	const int32 Third = FGitsShift::ResolveAnchor(T, 2, 3, Found);
	TestTrue(TEXT("third occurrence resolves"), Third >= 0);
	TestEqual(TEXT("three found"), Found, 3);
	if (Third >= 0)
	{
		TestTrue(TEXT("it is a boundary on line 2"), T.Steps[Third].bIsStatementBoundary && T.Steps[Third].Span.Start.Line == 2);
	}
	TestEqual(TEXT("fourth never ran"), FGitsShift::ResolveAnchor(T, 2, 4, Found), -1);
	TestEqual(TEXT("line 9 never ran"), FGitsShift::ResolveAnchor(T, 9, 1, Found), -1);
	TestEqual(TEXT("none found"), Found, 0);
	const int32 Log = FGitsShift::ResolveAnchor(T, 3, 1, Found);
	TestTrue(TEXT("the line after the loop"), Log > Third);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGitsTagsValidate, "GhostInTheStack.Companion.Tags.Validate", GitsTest::TestFlags)
bool FGitsTagsValidate::RunTest(const FString&)
{
	TestTrue(TEXT("known misconception"), GitsTags::IsMisconception(TEXT("fencepost")));
	TestFalse(TEXT("unknown misconception"), GitsTags::IsMisconception(TEXT("being-wrong")));
	TestTrue(TEXT("known concept"), GitsTags::IsConcept(TEXT("conditional")));
	TestEqual(TEXT("fifteen misconceptions"), GitsTags::Misconceptions().Num(), 15);
	TestEqual(TEXT("twenty-seven concepts"), GitsTags::Concepts().Num(), 27);

	UGitsScript* Script = NewObject<UGitsScript>();
	Script->Source = TEXT("a = 1\nb = 2\nc = 3\nd = 4\ne = 5\nf = 6\ng = 7\nh = 8\n");
	Script->Concepts = { TEXT("conditional") };
	Script->Predictions = { DoorPrediction() };
	FGitsHint H1; H1.Tier = 1; H1.Text = TEXT("look at line 6");
	Script->Hints = { H1 };
	TestEqual(TEXT("valid script"), GitsTags::Validate(Script).Num(), 0);

	Script->Predictions[0].Options[1].Misconception = TEXT("being-wrong");
	Script->Predictions[0].Options[2].Misconception.Reset();
	Script->Predictions[0].AnchorLine = 40;
	Script->Concepts.Add(TEXT("vibes"));
	const TArray<FString> Problems = GitsTags::Validate(Script);
	TestEqual(TEXT("four problems"), Problems.Num(), 4);
	return true;
}

#endif
