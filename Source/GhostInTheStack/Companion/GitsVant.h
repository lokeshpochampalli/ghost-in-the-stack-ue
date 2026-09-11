// Ghost in the Stack — VANT, the station's management system.
//
// Dry, tired, alone for eleven months. VANT asks the prediction before a first run, says what
// happened when the anchored line runs, locks a wrong reading until the player has watched the
// run from the start, hands out Ilse's notes a tier at a time, and speaks the outro when a
// system works. One per world; the terminals, the overlay and the caption all listen here.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GitsShift.h"
#include "GitsVant.generated.h"

class UGitsScript;
class UGitsStationSubsystem;

DECLARE_MULTICAST_DELEGATE_OneParam(FGitsOnVantSpeak, const FString& /*Line*/);
DECLARE_MULTICAST_DELEGATE(FGitsOnShiftChanged);

UCLASS()
class GHOSTINTHESTACK_API UGitsVantSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// --- voice
	FGitsOnVantSpeak OnSpeak;
	/** Something about a prediction, a lock or a hint changed; screens should redraw. */
	FGitsOnShiftChanged OnShiftChanged;
	void Speak(const FString& Line);
	const FString& GetLastLine() const { return LastLine; }

	// --- session (ADR-011: the seed is recorded so a session's shuffles can be reproduced)
	UFUNCTION(BlueprintPure, Category = "VANT")
	const FString& GetSessionId() const { return SessionId; }
	uint32 GetSessionSeed() const { return SessionSeed; }

	// --- predictions
	FGitsShift& ShiftFor(UGitsScript* Script);
	/** Ids that still need a commitment before the next run, in script order. */
	TArray<FString> Pending(UGitsScript* Script);
	/** Options in the order they are shown: shuffled from hash(sessionId + predictionId). Logs prediction_shown. */
	TArray<FGitsPredictionOption> Show(UGitsScript* Script, const FString& PredictionId, uint32& OutSeed);
	bool Select(UGitsScript* Script, const FString& PredictionId, const FString& OptionId);
	/**
	 * Commits, free. If a trace already exists for this exact source (a re-answer after the gate),
	 * it is settled against that trace at once; otherwise the run settles it. Returns VANT's line.
	 */
	FString Commit(UGitsScript* Script, const FString& PredictionId, const FString& CurrentSource);
	/** False, with VANT's reason, when a prediction is still pending. */
	bool CanRun(UGitsScript* Script, FString& Reason);
	/** Reveals the next unrevealed hint tier and speaks it, if the bus can pay for it; empty otherwise. */
	FString RevealNextHint(UGitsScript* Script);
	/** A committed or confirmed reading buys the discounted run (reference runCost). */
	bool IsDiscounted(UGitsScript* Script);
	/** Telemetry for a run that was charged, and for the reserve cell. */
	void NoteRun(UGitsScript* Script, int32 Cost, bool bDiscounted, int32 PowerAfter);
	void NoteReserveDrawn(int32 Before, int32 After, int32 Draws);
	/** The player changed a line; a prediction anchored on it needs a fresh commitment. */
	void NoteEdit(UGitsScript* Script, int32 Line);
	/** The intro, the first time a terminal is used. */
	void TerminalUsed(UGitsScript* Script);
	/** One line per prediction, for the console. */
	FString DescribeState(UGitsScript* Script);
	/** The script's goal has been met at least once this session. */
	bool IsComplete(const UGitsScript* Script) const;
	/** Every counted terminal in the level is complete. */
	bool IsSectorComplete() const { return bSectorComplete; }
	/** For a Make script: which test cases the last run of it passed, by index. */
	const TSet<int32>& PassedTests(const UGitsScript* Script);

	// UWorldSubsystem
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

private:
	void HandleRunStarted();
	void HandleStep(int32 StepIndex);
	void HandleRunFinished();
	UGitsStationSubsystem* Station() const;
	FString KeyOf(const UGitsScript* Script) const;
	/** Settles a committed prediction against the current trace at its anchor, and speaks. */
	void Settle(UGitsScript* Script, const FGitsPrediction& Prediction, bool bAtTheRun);
	void LogEvent(const FString& Event, const FString& Fields);

	FString SessionId;
	uint32 SessionSeed = 0;
	FString LastLine;
	TMap<FString, FGitsShift> Shifts;
	/** Option order per session and prediction, keyed script|prediction. */
	TMap<FString, TArray<FString>> OptionOrders;
	TSet<FString> Introduced;
	TSet<FString> OutroSpoken;
	TMap<FString, TSet<int32>> Passed;
	bool bSectorComplete = false;
	/** True with the failure in the station's voice as Reason when not. */
	bool EvaluateGoal(UGitsScript* Script, UGitsStationSubsystem* S, FString& Reason);
	/** Completes the script if its goal holds now: unlock, outro, sector check. */
	void TryComplete(UGitsScript* Script, bool bAfterRun);
	void CheckSector();

	/** The run in progress: the script and where each prediction's anchor landed in its trace. */
	TWeakObjectPtr<UGitsScript> RunScript;
	TMap<FString, int32> RunAnchorSteps;
	int32 RunFirstBoundary = 0;

	FDelegateHandle StartHandle, StepHandle, FinishHandle;
};
