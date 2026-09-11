// Ghost in the Stack — the prediction gate (port of the rules in src/ui/shift.ts).
//
// Committing is free and reveals nothing; correctness arrives with the run (ADR-006). A wrong
// answer locks the prediction until the player has watched the run from the start through the
// anchor (the scrub gate), after which it is re-answered against the trace that already exists
// (ADR-020: there is nothing to re-run). Plain C++ so the rules are testable without a world.
#pragma once

#include "CoreMinimal.h"
#include "Station/GitsScript.h"

struct FGitsTrace;

/** Where one prediction stands. */
struct FGitsPredictionState
{
	/** Browsing, not committing. */
	FString Selected;
	/** The option committed, until the run settles it. */
	FString Committed;
	int32 Attempts = 0;
	bool bSatisfied = false;
	/** Wrong, and waiting on the gate before it can be answered again. */
	bool bLocked = false;
	/** Since the lock, the head has been at or before the first statement. */
	bool bReachedStartSinceLock = false;
	/** The anchor never ran in the trace: skipped, not blocking. */
	bool bSkipped = false;
	double LockedAt = 0.0;
};

struct FGitsResolvedCommitment
{
	FString PredictionId;
	FString OptionId;
	FString Misconception;
	bool bCorrect = false;
	int32 Attempt = 0;
};

class GHOSTINTHESTACK_API FGitsShift
{
public:
	void Reset(const TArray<FGitsPrediction>& Predictions);
	const FGitsPredictionState& StateOf(const FString& PredictionId) const;

	/** Predictions that need a commitment before the next run: not satisfied, committed, locked or skipped. */
	TArray<FString> Pending(const TArray<FGitsPrediction>& Predictions) const;
	bool AllSatisfied(const TArray<FGitsPrediction>& Predictions) const;

	/** Selecting is browsing: free, reversible, reveals nothing. */
	bool Select(const FString& PredictionId, const FString& OptionId);
	/** Commits the selection. Free. Refused when satisfied, locked, or nothing is selected. */
	bool Commit(const FString& PredictionId);

	/** Settles the committed option: right satisfies, wrong locks. False when nothing was committed. */
	bool Resolve(const FGitsPrediction& Prediction, double Now, FGitsResolvedCommitment& Out);

	/**
	 * The gate. Records where the head is for a locked prediction and releases the lock once the
	 * head has been at or before the first statement and then, moving forward, at or past the
	 * anchor. Returns true on the tick that releases it.
	 */
	bool RecordHead(const FString& PredictionId, int32 Head, int32 FirstBoundary, int32 AnchorStep, bool bForward);

	void MarkSkipped(const FString& PredictionId, bool bSkipped);

	/** An edit to the line a prediction is anchored on needs a fresh commitment (ADR-020 rule 2). */
	void InvalidateLine(int32 Line, const TArray<FGitsPrediction>& Predictions);

	/** Which hint tiers have been revealed, in order. */
	TArray<int32> HintsRevealed;

	/**
	 * The step an anchor names: the Nth statement boundary whose statement starts on the line.
	 * -1 when it never ran that often; OccurrencesFound says how often it did (ADR-005).
	 */
	static int32 ResolveAnchor(const FGitsTrace& Trace, int32 Line, int32 Occurrence, int32& OccurrencesFound);

private:
	FGitsPredictionState& Mutable(const FString& PredictionId);
	TMap<FString, FGitsPredictionState> States;
};
