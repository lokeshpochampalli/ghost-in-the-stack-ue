// Ghost in the Stack — the station: runs scripts, records traces, plays them back.
//
// One per world. Systems register here by id and pose themselves from the folded world
// at the play head (CLAUDE.md: systems animate, the interpreter decides; rewind is index
// stepping). Phase 2 plays forward at a readable pace; Phase 3 adds scrubbing backwards
// on the same play head.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "Interpreter/GitsTypes.h"
#include "GitsStationSubsystem.generated.h"

class UGitsScript;
class AGitsSystemActor;
class AGitsStation;

DECLARE_MULTICAST_DELEGATE_OneParam(FGitsOnStepChanged, int32 /*StepIndex*/);
DECLARE_MULTICAST_DELEGATE(FGitsOnRunStarted);
DECLARE_MULTICAST_DELEGATE(FGitsOnRunFinished);
DECLARE_MULTICAST_DELEGATE_OneParam(FGitsOnMessage, const FString& /*Message*/);

/** What a run produced, for displays and tests. */
USTRUCT(BlueprintType)
struct FGitsRunSummary
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Station") bool bRan = false;
	UPROPERTY(BlueprintReadOnly, Category = "Station") bool bRefused = false;
	UPROPERTY(BlueprintReadOnly, Category = "Station") bool bParseFailed = false;
	UPROPERTY(BlueprintReadOnly, Category = "Station") FString Outcome;
	UPROPERTY(BlueprintReadOnly, Category = "Station") FString Message;
	UPROPERTY(BlueprintReadOnly, Category = "Station") TArray<FString> Output;
	UPROPERTY(BlueprintReadOnly, Category = "Station") int32 Steps = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Station") int32 Statements = 0;
};

UCLASS()
class GHOSTINTHESTACK_API UGitsStationSubsystem : public UWorldSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	// --- systems
	void RegisterSystem(AGitsSystemActor* System);
	void UnregisterSystem(AGitsSystemActor* System);
	AGitsSystemActor* FindSystem(const FString& SystemId) const;

	// --- running
	/**
	 * Parses and runs Source for Script. Returns false when refused (unchanged source) or the
	 * parse failed; Summary and LastMessage say why. A successful run starts playback.
	 */
	bool Run(UGitsScript* Script, const FString& Source, FGitsRunSummary& Summary);
	UFUNCTION(BlueprintCallable, Category = "Station")
	FGitsRunSummary RunSource(UGitsScript* Script, const FString& Source);

	/** VANT's last line: the refusal, the outcome, or the diagnostic. */
	UFUNCTION(BlueprintPure, Category = "Station")
	const FString& GetLastMessage() const { return LastMessage; }
	const FGitsRunSummary& GetLastSummary() const { return LastSummary; }

	// --- playback
	bool HasTrace() const { return Trace.Steps.Num() > 0; }
	const FGitsTrace& GetTrace() const { return Trace; }
	int32 GetPlayIndex() const { return PlayIndex; }
	bool IsPlaying() const { return bPlaying; }
	/** The world folded up to the play head. */
	const FGitsWorldState& GetCurrentWorld() const { return CurrentWorld; }
	/** Everything printed up to the play head. */
	TArray<FString> OutputAtPlayHead() const;
	/** 1-based source line of the statement at the play head, or 0. */
	int32 CurrentLine() const;
	/** Moves the play head to a step and poses every system from the world there. */
	void SeekTo(int32 StepIndex, bool bInstant);
	void FinishPlayback();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Station")
	float StatementsPerSecond = 2.5f;

	FGitsOnStepChanged OnStepChanged;
	FGitsOnRunStarted OnRunStarted;
	FGitsOnRunFinished OnRunFinished;
	FGitsOnMessage OnMessage;

	/** Frame time sampled while playback runs, for the acceptance report. */
	float PlaybackMinFps = 0.f;
	float PlaybackAvgFps = 0.f;
	int32 PlaybackFrames = 0;
	/** Index (within the playback) of the slowest frame, for finding hitches. */
	int32 PlaybackWorstFrame = -1;

	// FTickableGameObject
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UGitsStationSubsystem, STATGROUP_Tickables); }
	virtual bool IsTickable() const override { return !IsTemplate() && GetWorld() != nullptr; }
	virtual bool IsTickableInEditor() const override { return false; }

	// UWorldSubsystem
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

private:
	AGitsStation* FindStation() const;
	FGitsWorldState BuildInitialWorld() const;
	void PoseSystems(bool bInstant);
	void AdvancePlayHead(int32 NewIndex);

	UPROPERTY() TArray<TObjectPtr<AGitsSystemActor>> Systems;
	FGitsTrace Trace;
	FGitsWorldState CurrentWorld;
	FGitsRunSummary LastSummary;
	FString LastMessage;
	int32 PlayIndex = -1;
	bool bPlaying = false;
	float PlayClock = 0.f;
	/** Source of the last run per script asset path, for "nothing has changed". */
	TMap<FString, FString> LastRunSource;
	double FpsAccum = 0.0;
	bool bSkipNextFrameSample = false;
};
