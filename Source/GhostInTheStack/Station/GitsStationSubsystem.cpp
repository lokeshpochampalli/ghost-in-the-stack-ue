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

// --- running -----------------------------------------------------------------------------

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
	const FGitsWorldState Initial = BuildInitialWorld();
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

	// Start playback from the level's world, before the first step.
	CurrentWorld = Initial;
	PlayIndex = -1;
	PlayClock = 0.f;
	bPlaying = Trace.Steps.Num() > 0;
	PlaybackMinFps = 0.f; PlaybackAvgFps = 0.f; PlaybackFrames = 0; FpsAccum = 0.0;
	PlaybackWorstFrame = -1; bSkipNextFrameSample = true;
	for (AGitsSystemActor* S : Systems) { if (S) { S->PoseFromWorld(CurrentWorld, true); } }
	OnRunStarted.Broadcast();
	if (!bPlaying) { FinishPlayback(); }
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
	AdvancePlayHead(StepIndex);
	PoseSystems(bInstant);
	OnStepChanged.Broadcast(PlayIndex);
}

void UGitsStationSubsystem::FinishPlayback()
{
	bPlaying = false;
	if (HasTrace() && PlayIndex < Trace.Steps.Num() - 1) { SeekTo(Trace.Steps.Num() - 1, false); }
	if (PlaybackFrames > 0) { PlaybackAvgFps = (float)(PlaybackFrames / FpsAccum); }
	OnRunFinished.Broadcast();
	OnMessage.Broadcast(LastMessage);
}

void UGitsStationSubsystem::Tick(float DeltaTime)
{
	if (!bPlaying || !HasTrace()) { return; }
	// The first tick after Run() carries the delta of the frame that ran the interpreter (and,
	// under automation, the remote command that triggered it), so it is not a playback frame.
	if (bSkipNextFrameSample) { bSkipNextFrameSample = false; }
	else if (DeltaTime > 0.f)
	{
		const float Fps = 1.f / DeltaTime;
		if (PlaybackFrames == 0 || Fps < PlaybackMinFps) { PlaybackMinFps = Fps; PlaybackWorstFrame = PlaybackFrames; }
		FpsAccum += DeltaTime;
		++PlaybackFrames;
	}
	PlayClock += DeltaTime;
	const float Interval = 1.f / FMath::Max(0.1f, StatementsPerSecond);
	while (bPlaying && PlayClock >= Interval)
	{
		PlayClock -= Interval;
		// Advance to the next statement boundary, so the pace is one statement per beat.
		int32 Next = PlayIndex + 1;
		while (Next < Trace.Steps.Num() && !Trace.Steps[Next].bIsStatementBoundary) { ++Next; }
		if (Next >= Trace.Steps.Num())
		{
			FinishPlayback();
			return;
		}
		SeekTo(Next, false);
		if (PlayIndex >= Trace.Steps.Num() - 1) { FinishPlayback(); return; }
	}
}
