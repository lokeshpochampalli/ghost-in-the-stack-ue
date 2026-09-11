#include "GitsShift.h"
#include "Interpreter/GitsTrace.h"

void FGitsShift::Reset(const TArray<FGitsPrediction>& Predictions)
{
	States.Reset();
	for (const FGitsPrediction& P : Predictions) { States.Add(P.Id, FGitsPredictionState()); }
	HintsRevealed.Reset();
}

const FGitsPredictionState& FGitsShift::StateOf(const FString& PredictionId) const
{
	static const FGitsPredictionState Fresh;
	const FGitsPredictionState* S = States.Find(PredictionId);
	return S ? *S : Fresh;
}

FGitsPredictionState& FGitsShift::Mutable(const FString& PredictionId)
{
	return States.FindOrAdd(PredictionId);
}

TArray<FString> FGitsShift::Pending(const TArray<FGitsPrediction>& Predictions) const
{
	TArray<FString> Out;
	for (const FGitsPrediction& P : Predictions)
	{
		const FGitsPredictionState& S = StateOf(P.Id);
		if (S.bSatisfied || !S.Committed.IsEmpty() || S.bLocked || S.bSkipped) { continue; }
		Out.Add(P.Id);
	}
	return Out;
}

bool FGitsShift::AllSatisfied(const TArray<FGitsPrediction>& Predictions) const
{
	for (const FGitsPrediction& P : Predictions)
	{
		const FGitsPredictionState& S = StateOf(P.Id);
		if (!S.bSatisfied && !S.bSkipped) { return false; }
	}
	return true;
}

bool FGitsShift::Select(const FString& PredictionId, const FString& OptionId)
{
	FGitsPredictionState& S = Mutable(PredictionId);
	if (S.bSatisfied || S.bLocked || !S.Committed.IsEmpty()) { return false; }
	S.Selected = OptionId;
	return true;
}

bool FGitsShift::Commit(const FString& PredictionId)
{
	FGitsPredictionState& S = Mutable(PredictionId);
	if (S.bSatisfied || S.bLocked || S.Selected.IsEmpty() || !S.Committed.IsEmpty()) { return false; }
	S.Committed = S.Selected;
	return true;
}

bool FGitsShift::Resolve(const FGitsPrediction& Prediction, double Now, FGitsResolvedCommitment& Out)
{
	FGitsPredictionState& S = Mutable(Prediction.Id);
	if (S.Committed.IsEmpty()) { return false; }
	const bool bCorrect = S.Committed == Prediction.CorrectId;
	const FGitsPredictionOption* Chosen = Prediction.Options.FindByPredicate([&S](const FGitsPredictionOption& O) { return O.Id == S.Committed; });
	Out.PredictionId = Prediction.Id;
	Out.OptionId = S.Committed;
	Out.bCorrect = bCorrect;
	Out.Attempt = S.Attempts + 1;
	// Which wrong belief they hold, not merely that they were wrong.
	Out.Misconception = Chosen ? Chosen->Misconception : FString();
	S.Attempts = Out.Attempt;
	S.Committed.Reset();
	S.Selected.Reset();
	S.bSatisfied = bCorrect;
	S.bLocked = !bCorrect;
	S.bReachedStartSinceLock = false;
	S.LockedAt = bCorrect ? 0.0 : Now;
	return true;
}

bool FGitsShift::RecordHead(const FString& PredictionId, int32 Head, int32 FirstBoundary, int32 AnchorStep, bool bForward)
{
	FGitsPredictionState& S = Mutable(PredictionId);
	if (!S.bLocked) { return false; }
	if (Head <= FirstBoundary) { S.bReachedStartSinceLock = true; }
	if (S.bReachedStartSinceLock && bForward && Head >= AnchorStep)
	{
		S.bLocked = false;
		S.bReachedStartSinceLock = false;
		S.LockedAt = 0.0;
		return true;
	}
	return false;
}

void FGitsShift::MarkSkipped(const FString& PredictionId, bool bSkipped)
{
	Mutable(PredictionId).bSkipped = bSkipped;
}

void FGitsShift::InvalidateLine(int32 Line, const TArray<FGitsPrediction>& Predictions)
{
	for (const FGitsPrediction& P : Predictions)
	{
		if (P.AnchorLine != Line) { continue; }
		FGitsPredictionState& S = Mutable(P.Id);
		if (S.bSatisfied) { continue; }
		S.Committed.Reset();
		S.Selected.Reset();
		S.bLocked = false;
		S.bReachedStartSinceLock = false;
		S.bSkipped = false;
		S.LockedAt = 0.0;
	}
}

int32 FGitsShift::ResolveAnchor(const FGitsTrace& Trace, int32 Line, int32 Occurrence, int32& OccurrencesFound)
{
	OccurrencesFound = 0;
	for (int32 StepIndex : GitsTrace::StatementBoundaries(Trace))
	{
		if (Trace.Steps[StepIndex].Span.Start.Line != Line) { continue; }
		++OccurrencesFound;
		if (OccurrencesFound == Occurrence) { return StepIndex; }
	}
	return -1;
}
