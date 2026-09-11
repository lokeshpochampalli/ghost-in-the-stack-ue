#include "GitsStationSubsystem.h"
#include "GitsScript.h"
#include "GitsStation.h"
#include "GitsSystemActor.h"
#include "Interpreter/GitsEvaluator.h"
#include "Interpreter/GitsTrace.h"
#include "Interpreter/GitsDiagnostics.h"
#include "EngineUtils.h"
#include "Engine/World.h"

bool UGitsStationSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (!Super::ShouldCreateSubsystem(Outer)) { return false; }
	const UWorld* World = Cast<UWorld>(Outer);
	return World && (World->IsGameWorld() || World->WorldType == EWorldType::PIE);
}

// --- systems -------------------------------------------------------------------------

void UGitsStationSubsystem::RegisterSystem(AGitsSystemActor* System)
{
	if (System) { Systems.AddUnique(System); }
}

void UGitsStationSubsystem::UnregisterSystem(AGitsSystemActor* System)
{
	Systems.Remove(System);
}

AGitsSystemActor* UGitsStationSubsystem::FindSystem(const FString& SystemId) const
{
	for (AGitsSystemActor* S : Systems) { if (S && S->SystemId == SystemId) { return S; } }
	return nullptr;
}

AGitsStation* UGitsStationSubsystem::FindStation() const
{
	for (TActorIterator<AGitsStation> It(GetWorld()); It; ++It) { return *It; }
	return nullptr;
}

FGitsWorldState UGitsStationSubsystem::BuildInitialWorld() const
{
	if (const AGitsStation* Station = FindStation()) { return Station->InitialWorld(); }
	return FGitsWorldState();
}

// --- power ---------------------------------------------------------------------------------

void UGitsStationSubsystem::InitialisePower(int32 Budget, int32 Reserve)
{
	PowerBudget = FMath::Max(0, Budget);
	ReserveRestore = FMath::Max(0, Reserve);
	Power = PowerBudget;
	ReserveDraws = 0;
	OnPowerChanged.Broadcast();
}

bool UGitsStationSubsystem::ChargePower(int32 Cost)
{
	if (Cost < 0 || Cost > Power) { return false; }
	if (Cost == 0) { return true; }
	Power -= Cost;
	OnPowerChanged.Broadcast();
	return true;
}

int32 UGitsStationSubsystem::DrawReserve()
{
	// Ilse's reserve cell: restores to a fixed level, never above it, and every draw counts.
	if (Power >= ReserveRestore) { return -1; }
	Power = ReserveRestore;
	++ReserveDraws;
	OnPowerChanged.Broadcast();
	return Power;
}

void UGitsStationSubsystem::SetPower(int32 NewPower)
{
	Power = FMath::Clamp(NewPower, 0, FMath::Max(PowerBudget, NewPower));
	OnPowerChanged.Broadcast();
}

// --- running -----------------------------------------------------------------------------

bool UGitsStationSubsystem::HasTraceFor(const UGitsScript* Script, const FString& Source) const
{
	if (!HasTrace() || !Script || CurrentScript.Get() != Script) { return false; }
	const FString* Last = LastRunSource.Find(Script->GetPathName());
	return Last && *Last == Source.Replace(TEXT("\r\n"), TEXT("\n"));
}

FGitsRunSummary UGitsStationSubsystem::RunSource(UGitsScript* Script, const FString& Source)
{
	FGitsRunSummary Summary;
	Run(Script, Source, Summary);
	return Summary;
}

bool UGitsStationSubsystem::Run(UGitsScript* Script, const FString& Source, FGitsRunSummary& Summary)
{
	Summary = FGitsRunSummary();
	const FString Key = Script ? Script->GetPathName() : TEXT("<none>");
	const FString Normalised = Source.Replace(TEXT("\r\n"), TEXT("\n"));

	// ADR-020: running unchanged source is not a thing. The trace already exists.
	if (const FString* Last = LastRunSource.Find(Key))
	{
		if (*Last == Normalised && HasTrace())
		{
			const AGitsStation* Station = FindStation();
			Summary.bRefused = true;
			Summary.Message = Station ? Station->UnchangedSourceMessage : TEXT("Nothing has changed, so nothing new will happen.");
			LastMessage = Summary.Message;
			LastSummary = Summary;
			OnMessage.Broadcast(LastMessage);
			return false;
		}
	}

	FGitsParseOptions ParseOptions;
	ParseOptions.Tier = Script ? (EGitsTier)FMath::Clamp(Script->Tier, 1, 4) : EGitsTier::Four;
	FGitsParseResult Parsed = GitsParser::Parse(Normalised, ParseOptions);
	if (Parsed.Diagnostics.Num() > 0)
	{
		Summary.bParseFailed = true;
		Summary.Message = GitsDiagnostics::Format(Parsed.Diagnostics[0]);
		Summary.Outcome = TEXT("did not run: ") + GitsDiagnostics::CodeName(Parsed.Diagnostics[0].Code);
		LastMessage = Summary.Message;
		LastSummary = Summary;
		OnMessage.Broadcast(LastMessage);
		return false;
	}

	FGitsRunOptions RunOptions;
	if (Script)
	{
		RunOptions.StatementCap = FMath::Max(1, Script->StatementCap);
		if (Script->DeclaredBuiltins.Num() > 0)
		{
			RunOptions.bRestrictBuiltins = true;
			for (const FString& B : Script->DeclaredBuiltins) { RunOptions.Builtins.Add(B); }
		}
	}
	const AGitsStation* Station = FindStation();
	// The world carries over between runs: a door the last script opened is still open when
	// the next script starts. The station's initial values only fill in what no run has set.
	FGitsWorldState Initial = BuildInitialWorld();
	if (HasTrace())
	{
		const FGitsWorldState Carried = GitsTrace::WorldAt(Trace, Trace.Steps.Num() - 1);
		for (const auto& P : Carried) { Initial.Add(P.Key, P.Value); }
	}
	FGitsWorldOracle Oracle;
	if (Station)
	{
		TWeakObjectPtr<const AGitsStation> WeakStation = Station;
		Oracle = [WeakStation](const FGitsWorldState& World, const FGitsQuery& Query) -> FGitsValue
		{
			return WeakStation.IsValid() ? WeakStation->Read(World, Query) : FGitsValue::MakeInt(0);
		};
	}
	else
	{
		Oracle = FGitsWorldOracle(&GitsWorld::NullOracle);
	}

	Trace = GitsEvaluator::Run(Parsed.Program, Initial, Oracle, RunOptions);
	LastRunSource.Add(Key, Normalised);
	CurrentScript = Script;

	Summary.bRan = true;
	Summary.Steps = Trace.Steps.Num();
	Summary.Statements = Trace.StatementCount;
	Summary.Output = Trace.Output;
	Summary.Outcome = GitsTrace::DescribeOutcome(Trace);
	switch (Trace.Outcome.Kind)
	{
	case EGitsOutcomeKind::Completed: Summary.Message = TEXT("Run complete."); break;
	default: Summary.Message = GitsDiagnostics::Format(Trace.Outcome.Diagnostic); break;
	}
	LastMessage = Summary.Message;
	LastSummary = Summary;

	// Start playback from the world before the first step.
	Recorder.Build(Trace, StatementsPerSecond);
	CurrentWorld = Initial;
	PlayIndex = -1;
	PlayClock = 0.f;
	RewindHeldSeconds = 0.f;
	PlaybackMinFps = 0.f; PlaybackAvgFps = 0.f; PlaybackFrames = 0; FpsAccum = 0.0;
	PlaybackWorstFrame = -1; bSkipNextFrameSample = true;
	LastSeekMs = 0.f; MaxSeekMs = 0.f;
	for (AGitsSystemActor* S : Systems) { if (S) { S->PoseFromWorld(CurrentWorld, true); } }
	SetState(Trace.Steps.Num() > 0 ? EGitsPlayState::Playing : EGitsPlayState::Finished);
	OnRunStarted.Broadcast();
	if (Trace.Steps.Num() == 0) { FinishPlayback(); }
	return true;
}

// --- playback ---------------------------------------------------------------------------

TArray<FString> UGitsStationSubsystem::OutputAtPlayHead() const
{
	if (PlayIndex < 0 || !HasTrace()) { return TArray<FString>(); }
	return GitsTrace::OutputAt(Trace, PlayIndex);
}

int32 UGitsStationSubsystem::CurrentLine() const
{
	if (PlayIndex < 0 || !Trace.Steps.IsValidIndex(PlayIndex)) { return 0; }
	// The line of the statement this step belongs to: the nearest boundary at or after it
	// within the same statement, else the step's own line.
	const int32 Stmt = Trace.Steps[PlayIndex].StmtIndex;
	for (int32 i = PlayIndex; i < Trace.Steps.Num(); ++i)
	{
		if (Trace.Steps[i].StmtIndex != Stmt) { break; }
		if (Trace.Steps[i].bIsStatementBoundary) { return Trace.Steps[i].Span.Start.Line; }
	}
	return Trace.Steps[PlayIndex].Span.Start.Line;
}

void UGitsStationSubsystem::PoseSystems(bool bInstant)
{
	for (AGitsSystemActor* S : Systems) { if (S) { S->PoseFromWorld(CurrentWorld, bInstant); } }
}

void UGitsStationSubsystem::AdvancePlayHead(int32 NewIndex)
{
	NewIndex = FMath::Clamp(NewIndex, -1, Trace.Steps.Num() - 1);
	if (NewIndex < PlayIndex)
	{
		// Backwards: refold from the start. Rewind is index stepping, never reverse simulation.
		CurrentWorld = Trace.InitialWorld;
		PlayIndex = -1;
	}
	for (int32 i = PlayIndex + 1; i <= NewIndex; ++i)
	{
		for (const FGitsEffect& E : Trace.Steps[i].Effects) { CurrentWorld = GitsWorld::Reduce(CurrentWorld, E); }
	}
	PlayIndex = NewIndex;
}

void UGitsStationSubsystem::SeekTo(int32 StepIndex, bool bInstant)
{
	if (!HasTrace()) { return; }
	const double Started = FPlatformTime::Seconds();
	AdvancePlayHead(StepIndex);
	PoseSystems(bInstant);
	OnStepChanged.Broadcast(PlayIndex);
	LastSeekMs = (float)((FPlatformTime::Seconds() - Started) * 1000.0);
	MaxSeekMs = FMath::Max(MaxSeekMs, LastSeekMs);
}

void UGitsStationSubsystem::SetState(EGitsPlayState NewState)
{
	if (State == NewState) { return; }
	State = NewState;
	// Frame samples describe one stretch of playing or rewinding, so the report can tell
	// them apart; the frame that changed state (a key, a console command) is not sampled.
	if (State == EGitsPlayState::Playing || State == EGitsPlayState::Rewinding)
	{
		PlaybackMinFps = 0.f; PlaybackAvgFps = 0.f; PlaybackFrames = 0; FpsAccum = 0.0; PlaybackWorstFrame = -1;
		bSkipNextFrameSample = true;
	}
	else if (PlaybackFrames > 0) { PlaybackAvgFps = (float)(PlaybackFrames / FpsAccum); }
	OnPlayStateChanged.Broadcast(State);
}

void UGitsStationSubsystem::FinishPlayback()
{
	PlayClock = Recorder.IsBuilt() ? Recorder.TotalTime() : 0.f;
	if (HasTrace() && PlayIndex < Trace.Steps.Num() - 1) { SeekTo(Trace.Steps.Num() - 1, false); }
	SetState(EGitsPlayState::Finished);
	OnRunFinished.Broadcast();
	OnMessage.Broadcast(LastMessage);
}

// --- rewind ---------------------------------------------------------------------------------

bool UGitsStationSubsystem::BeginRewind()
{
	if (!HasTrace() || !Recorder.IsBuilt()) { return false; }
	if (State == EGitsPlayState::Rewinding) { return true; }
	if (State == EGitsPlayState::Finished) { PlayClock = Recorder.TotalTime(); }
	RewindHeldSeconds = 0.f;
	SetState(EGitsPlayState::Rewinding);
	OnStepChanged.Broadcast(PlayIndex);
	return true;
}

void UGitsStationSubsystem::EndRewind()
{
	if (State != EGitsPlayState::Rewinding) { return; }
	// Release to resume: the clock runs forward again from wherever it got to.
	SetState(EGitsPlayState::Playing);
	OnStepChanged.Broadcast(PlayIndex);
}

bool UGitsStationSubsystem::VerifyRewind(FString& Report)
{
	if (!HasTrace() || !Recorder.IsBuilt()) { Report = TEXT("no trace"); return false; }
	const int32 SavedIndex = PlayIndex;
	const EGitsPlayState SavedState = State;
	const TArray<int32>& Bounds = Recorder.Boundaries();
	int32 Checked = 0, Mismatches = 0;
	float Worst = 0.f;
	for (int32 b = Bounds.Num() - 1; b >= 0; --b)
	{
		SeekTo(Bounds[b], true);
		Worst = FMath::Max(Worst, LastSeekMs);
		++Checked;
		const FGitsWorldState Expected = GitsTrace::WorldAt(Trace, Bounds[b]);
		bool bSame = Expected.Num() == CurrentWorld.Num();
		for (const auto& P : Expected) { const FGitsWorldValue* V = CurrentWorld.Find(P.Key); if (!V || !(*V == P.Value)) { bSame = false; } }
		const int32 ExpectedLine = Trace.Steps[Bounds[b]].Span.Start.Line;
		if (!bSame || CurrentLine() != ExpectedLine) { ++Mismatches; }
	}
	SeekTo(SavedIndex, true);
	SetState(SavedState);
	Report = FString::Printf(TEXT("verified %d boundaries backwards, mismatches=%d, slowest step change %.2f ms"), Checked, Mismatches, Worst);
	return Mismatches == 0;
}

// --- the clock ------------------------------------------------------------------------------

void UGitsStationSubsystem::SampleFrame(float DeltaTime)
{
	// The first tick after Run() carries the delta of the frame that ran the interpreter (and,
	// under automation, the remote command that triggered it), so it is not a playback frame.
	if (bSkipNextFrameSample) { bSkipNextFrameSample = false; return; }
	if (DeltaTime <= 0.f) { return; }
	const float Fps = 1.f / DeltaTime;
	if (PlaybackFrames == 0 || Fps < PlaybackMinFps) { PlaybackMinFps = Fps; PlaybackWorstFrame = PlaybackFrames; }
	FpsAccum += DeltaTime;
	++PlaybackFrames;
}

void UGitsStationSubsystem::StartFrameSample(float Seconds)
{
	SampleRemaining = FMath::Max(0.1f, Seconds);
	SampleMinFps = 0.f; SampleAvgFps = 0.f; SampleFrames = 0; SampleFramesUnder60 = 0; SampleAccum = 0.0;
	bSampleSkipFirst = true;
}

void UGitsStationSubsystem::Tick(float DeltaTime)
{
	if (SampleRemaining > 0.f)
	{
		// The frame that started the sample ran the console command; it is not a walking frame.
		if (bSampleSkipFirst) { bSampleSkipFirst = false; }
		else if (DeltaTime > 0.f)
		{
			const float Fps = 1.f / DeltaTime;
			SampleMinFps = SampleFrames == 0 ? Fps : FMath::Min(SampleMinFps, Fps);
			if (Fps < 59.f) { ++SampleFramesUnder60; }
			SampleAccum += DeltaTime;
			++SampleFrames;
			SampleRemaining -= DeltaTime;
			if (SampleRemaining <= 0.f)
			{
				SampleAvgFps = SampleFrames > 0 ? (float)(SampleFrames / SampleAccum) : 0.f;
				UE_LOG(LogTemp, Display, TEXT("GitsFrameSample: frames=%d avgFps=%.1f minFps=%.1f under60=%d"), SampleFrames, SampleAvgFps, SampleMinFps, SampleFramesUnder60);
			}
		}
	}
	if (!HasTrace() || !Recorder.IsBuilt()) { return; }
	if (State == EGitsPlayState::Playing)
	{
		SampleFrame(DeltaTime);
		PlayClock += DeltaTime;
		if (PlayClock >= Recorder.TotalTime()) { FinishPlayback(); return; }
		const int32 Step = Recorder.StepAtTime(PlayClock);
		if (Step != PlayIndex) { SeekTo(Step, false); }
	}
	else if (State == EGitsPlayState::Rewinding)
	{
		SampleFrame(DeltaTime);
		RewindHeldSeconds += DeltaTime;
		const float Ramp = RewindRampSeconds > 0.f ? FMath::Clamp(RewindHeldSeconds / RewindRampSeconds, 0.f, 1.f) : 1.f;
		const float Speed = FMath::Lerp(RewindSpeedStart, RewindSpeedMax, Ramp);
		PlayClock = FMath::Max(0.f, PlayClock - DeltaTime * Speed);
		const int32 Step = Recorder.StepAtTime(PlayClock);
		if (Step != PlayIndex) { SeekTo(Step, false); }
	}
}
