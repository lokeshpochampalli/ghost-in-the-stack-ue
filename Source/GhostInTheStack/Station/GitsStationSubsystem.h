// Ghost in the Stack — the station: runs scripts, records traces, plays them back.
//
// One per world. Systems register here by id and pose themselves from the folded world
// at the play head (CLAUDE.md: systems animate, the interpreter decides; rewind is index
// stepping). Playback runs forward one statement per beat; holding rewind runs the same
// clock backwards through the recorder, and every system poses itself from the step the
// clock is on.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "Interpreter/GitsTypes.h"
#include "Recorder/GitsRecorder.h"
#include "GitsStationSubsystem.generated.h"

class UGitsScript;
class AGitsSystemActor;
class AGitsStation;

DECLARE_MULTICAST_DELEGATE_OneParam(FGitsOnStepChanged, int32 /*StepIndex*/);
DECLARE_MULTICAST_DELEGATE(FGitsOnRunStarted);
DECLARE_MULTICAST_DELEGATE(FGitsOnRunFinished);
DECLARE_MULTICAST_DELEGATE_OneParam(FGitsOnMessage, const FString& /*Message*/);

UENUM(BlueprintType)
enum class EGitsPlayState : uint8
{
	/** No trace yet. */
	Idle,
	/** The clock runs forward, one statement per beat. */
	Playing,
	/** Rewind is held: the clock runs backward. */
	Rewinding,
	/** The clock reached the end; the world is as the run left it. */
	Finished
};

DECLARE_MULTICAST_DELEGATE_OneParam(FGitsOnPlayStateChanged, EGitsPlayState);

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
	const FGitsRecorder& GetRecorder() const { return Recorder; }
	/** The script asset the current trace came from, so a terminal knows whether the run is its own. */
	UGitsScript* GetCurrentScript() const { return CurrentScript.Get(); }
	/** True when the current trace is the run of exactly this source of this script (ADR-020: it cannot change). */
	bool HasTraceFor(const UGitsScript* Script, const FString& Source) const;
	int32 GetPlayIndex() const { return PlayIndex; }
	UFUNCTION(BlueprintPure, Category = "Station")
	EGitsPlayState GetPlayState() const { return State; }
	bool IsPlaying() const { return State == EGitsPlayState::Playing; }
	bool IsRewinding() const { return State == EGitsPlayState::Rewinding; }
	/** World time of the play head, in seconds of playback. */
	float GetPlayClock() const { return PlayClock; }

	// --- rewind (Phase 3): hold to scrub the clock backwards, release to resume forward
	UFUNCTION(BlueprintCallable, Category = "Station")
	bool BeginRewind();
	UFUNCTION(BlueprintCallable, Category = "Station")
	void EndRewind();
	/**
	 * Seeks through every statement boundary backwards, checking that the folded world and the
	 * current line match the trace at each, and timing each step change. For the acceptance
	 * report; restores the play head afterwards.
	 */
	bool VerifyRewind(FString& Report);
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

	/** Rewind pace as a multiple of playback pace when the key is first held... */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Station")
	float RewindSpeedStart = 1.f;
	/** ...ramping to this after RewindRampSeconds, so a long loop is not a long wait. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Station")
	float RewindSpeedMax = 4.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Station")
	float RewindRampSeconds = 3.f;

	FGitsOnStepChanged OnStepChanged;
	FGitsOnRunStarted OnRunStarted;
	FGitsOnRunFinished OnRunFinished;
	FGitsOnMessage OnMessage;
	FGitsOnPlayStateChanged OnPlayStateChanged;

	/** Wall time of the last and the slowest step change since the run started, in ms. */
	float LastSeekMs = 0.f;
	float MaxSeekMs = 0.f;

	/** Frame time sampled over the current playing or rewinding stretch, for the acceptance report. */
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
	void SetState(EGitsPlayState NewState);
	void SampleFrame(float DeltaTime);

	UPROPERTY() TArray<TObjectPtr<AGitsSystemActor>> Systems;
	FGitsTrace Trace;
	FGitsRecorder Recorder;
	TWeakObjectPtr<UGitsScript> CurrentScript;
	FGitsWorldState CurrentWorld;
	FGitsRunSummary LastSummary;
	FString LastMessage;
	int32 PlayIndex = -1;
	EGitsPlayState State = EGitsPlayState::Idle;
	float PlayClock = 0.f;
	float RewindHeldSeconds = 0.f;
	/** Source of the last run per script asset path, for "nothing has changed". */
	TMap<FString, FString> LastRunSource;
	double FpsAccum = 0.0;
	bool bSkipNextFrameSample = false;
};
