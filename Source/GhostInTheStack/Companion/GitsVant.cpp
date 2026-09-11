#include "GitsVant.h"
#include "GitsTags.h"
#include "Station/GitsScript.h"
#include "Station/GitsStationSubsystem.h"
#include "Station/GitsStation.h"
#include "Station/GitsTerminal.h"
#include "Interpreter/GitsRng.h"
#include "EngineUtils.h"
#include "Interpreter/GitsTrace.h"
#include "Engine/World.h"
#include "Misc/Guid.h"

// Telemetry precursor (Phase 8 writes the export): the reference event names, one line each.
DEFINE_LOG_CATEGORY_STATIC(LogGitsTelemetry, Display, All);
DEFINE_LOG_CATEGORY_STATIC(LogGitsVant, Display, All);

bool UGitsVantSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (!Super::ShouldCreateSubsystem(Outer)) { return false; }
	const UWorld* World = Cast<UWorld>(Outer);
	return World && (World->IsGameWorld() || World->WorldType == EWorldType::PIE);
}

void UGitsVantSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	SessionId = FGuid::NewGuid().ToString(EGuidFormats::DigitsLower);
	SessionSeed = GitsRng::HashString(SessionId);
	LogEvent(TEXT("session_start"), FString::Printf(TEXT("seed=%u"), SessionSeed));
	if (UGitsStationSubsystem* S = Station())
	{
		StartHandle = S->OnRunStarted.AddUObject(this, &UGitsVantSubsystem::HandleRunStarted);
		StepHandle = S->OnStepChanged.AddUObject(this, &UGitsVantSubsystem::HandleStep);
		FinishHandle = S->OnRunFinished.AddUObject(this, &UGitsVantSubsystem::HandleRunFinished);
	}
}

void UGitsVantSubsystem::Deinitialize()
{
	if (UGitsStationSubsystem* S = Station())
	{
		S->OnRunStarted.Remove(StartHandle);
		S->OnStepChanged.Remove(StepHandle);
		S->OnRunFinished.Remove(FinishHandle);
	}
	Super::Deinitialize();
}

UGitsStationSubsystem* UGitsVantSubsystem::Station() const
{
	return GetWorld() ? GetWorld()->GetSubsystem<UGitsStationSubsystem>() : nullptr;
}

FString UGitsVantSubsystem::KeyOf(const UGitsScript* Script) const
{
	return Script ? Script->GetPathName() : TEXT("<none>");
}

void UGitsVantSubsystem::LogEvent(const FString& Event, const FString& Fields)
{
	UE_LOG(LogGitsTelemetry, Display, TEXT("event=%s session=%s %s"), *Event, *SessionId, *Fields);
}

// --- voice ----------------------------------------------------------------------------------

void UGitsVantSubsystem::Speak(const FString& Line)
{
	LastLine = Line;
	UE_LOG(LogGitsVant, Display, TEXT("VANT: %s"), *Line);
	OnSpeak.Broadcast(Line);
}

// --- predictions ----------------------------------------------------------------------------

FGitsShift& UGitsVantSubsystem::ShiftFor(UGitsScript* Script)
{
	const FString Key = KeyOf(Script);
	if (FGitsShift* Existing = Shifts.Find(Key)) { return *Existing; }
	FGitsShift& Fresh = Shifts.Add(Key);
	if (Script)
	{
		Fresh.Reset(Script->Predictions);
		for (const FString& Problem : GitsTags::Validate(Script))
		{
			UE_LOG(LogGitsVant, Warning, TEXT("%s: %s"), *Script->GetName(), *Problem);
		}
	}
	return Fresh;
}

TArray<FString> UGitsVantSubsystem::Pending(UGitsScript* Script)
{
	if (!Script) { return TArray<FString>(); }
	return ShiftFor(Script).Pending(Script->Predictions);
}

TArray<FGitsPredictionOption> UGitsVantSubsystem::Show(UGitsScript* Script, const FString& PredictionId, uint32& OutSeed)
{
	TArray<FGitsPredictionOption> Out;
	const FGitsPrediction* P = Script ? Script->FindPrediction(PredictionId) : nullptr;
	if (!P) { OutSeed = 0; return Out; }
	// Shuffled, or position becomes a confound; seeded, or a session cannot be replayed (ADR-011).
	// The script name is in the seed because prediction ids repeat across scripts ("p1"), and two
	// scripts sharing a permutation would put the correct answer in the same place twice.
	OutSeed = GitsRng::HashString(SessionId + Script->GetName() + PredictionId);
	const FString Key = KeyOf(Script) + TEXT("|") + PredictionId;
	TArray<FString>* Order = OptionOrders.Find(Key);
	if (!Order)
	{
		TArray<FString> Ids;
		for (const FGitsPredictionOption& O : P->Options) { Ids.Add(O.Id); }
		GitsRng::FMulberry32 Rng(OutSeed);
		Order = &OptionOrders.Add(Key, Rng.Shuffle(Ids));
	}
	for (const FString& Id : *Order)
	{
		if (const FGitsPredictionOption* O = P->Options.FindByPredicate([&Id](const FGitsPredictionOption& C) { return C.Id == Id; })) { Out.Add(*O); }
	}
	LogEvent(TEXT("prediction_shown"), FString::Printf(TEXT("script=%s prediction=%s seed=%u order=%s"), *Script->GetName(), *PredictionId, OutSeed, *FString::Join(*Order, TEXT(","))));
	return Out;
}

bool UGitsVantSubsystem::Select(UGitsScript* Script, const FString& PredictionId, const FString& OptionId)
{
	if (!Script) { return false; }
	const bool bChanged = ShiftFor(Script).Select(PredictionId, OptionId);
	if (bChanged) { OnShiftChanged.Broadcast(); }
	return bChanged;
}

FString UGitsVantSubsystem::Commit(UGitsScript* Script, const FString& PredictionId, const FString& CurrentSource)
{
	const FGitsPrediction* P = Script ? Script->FindPrediction(PredictionId) : nullptr;
	if (!P) { return FString(); }
	FGitsShift& Shift = ShiftFor(Script);
	if (!Shift.Commit(PredictionId)) { return FString(); }
	LogEvent(TEXT("prediction_committed"), FString::Printf(TEXT("script=%s prediction=%s option=%s"), *Script->GetName(), *PredictionId, *Shift.StateOf(PredictionId).Committed));

	// A re-answer after the gate: the trace already exists and cannot change, so it settles the
	// commitment now, with no run and no power (ADR-006 meets ADR-020). Nothing is revealed early;
	// the execution already happened, and the gate made the player watch it.
	UGitsStationSubsystem* S = Station();
	if (S && S->HasTraceFor(Script, CurrentSource))
	{
		Settle(Script, *P, false);
	}
	else
	{
		Speak(TEXT("Noted. Run it, and we both find out."));
	}
	OnShiftChanged.Broadcast();
	return LastLine;
}

void UGitsVantSubsystem::Settle(UGitsScript* Script, const FGitsPrediction& Prediction, bool bAtTheRun)
{
	UGitsStationSubsystem* S = Station();
	if (!S || !S->HasTrace()) { return; }
	FGitsShift& Shift = ShiftFor(Script);
	int32 Found = 0;
	const int32 AnchorStep = FGitsShift::ResolveAnchor(S->GetTrace(), Prediction.AnchorLine, Prediction.AnchorOccurrence, Found);
	if (AnchorStep < 0)
	{
		// The line it was about never ran that often. Skipped, logged, never a blocker (ADR-005).
		Shift.MarkSkipped(Prediction.Id, true);
		LogEvent(TEXT("prediction_unresolvable"), FString::Printf(TEXT("script=%s prediction=%s line=%d occurrence=%d found=%d"), *Script->GetName(), *Prediction.Id, Prediction.AnchorLine, Prediction.AnchorOccurrence, Found));
		Speak(FString::Printf(TEXT("Line %d never ran, so that question is moot. It does not hold you up."), Prediction.AnchorLine));
		return;
	}
	FGitsResolvedCommitment R;
	if (!Shift.Resolve(Prediction, FPlatformTime::Seconds(), R)) { return; }
	LogEvent(TEXT("prediction_submitted"), FString::Printf(TEXT("script=%s prediction=%s option=%s correct=%d attempt=%d misconception=%s settledBy=%s"),
		*Script->GetName(), *R.PredictionId, *R.OptionId, R.bCorrect, R.Attempt, *R.Misconception, bAtTheRun ? TEXT("run") : TEXT("trace")));
	if (R.bCorrect)
	{
		Speak(bAtTheRun ? TEXT("That is what happened. You read it right.") : TEXT("Right. That is what it did. Reading confirmed."));
		if (!bAtTheRun) { TryComplete(Script, false); }
	}
	else
	{
		// The failure, named, when the content names it; the physical failure is on the panel.
		const FGitsPredictionOption* Chosen = Prediction.Options.FindByPredicate([&R](const FGitsPredictionOption& O) { return O.Id == R.OptionId; });
		const FString Named = Chosen && !Chosen->Reveal.IsEmpty() ? Chosen->Reveal + TEXT(" ") : FString();
		Speak(Named + (bAtTheRun
			? TEXT("Take it back to the start and watch it through before you answer again.")
			: TEXT("Still not it. Back to the start; watch the line, then tell me again.")));
	}
}

bool UGitsVantSubsystem::CanRun(UGitsScript* Script, FString& Reason)
{
	const TArray<FString> Pend = Pending(Script);
	if (Pend.Num() == 0) { return true; }
	Reason = Pend.Num() == 1
		? TEXT("Commit to a reading before you spend the power. It costs nothing to say what you expect.")
		: FString::Printf(TEXT("Commit to all %d readings before you spend the power. It costs nothing to say what you expect."), Pend.Num());
	return false;
}

FString UGitsVantSubsystem::RevealNextHint(UGitsScript* Script)
{
	if (!Script) { return FString(); }
	FGitsShift& Shift = ShiftFor(Script);
	TArray<FGitsHint> Sorted = Script->Hints;
	Sorted.Sort([](const FGitsHint& A, const FGitsHint& B) { return A.Tier < B.Tier; });
	UGitsStationSubsystem* S = Station();
	for (const FGitsHint& H : Sorted)
	{
		if (Shift.HintsRevealed.Contains(H.Tier)) { continue; }
		// Ilse's later notes cost power (ADR-006 in spirit: the note is a cheaper run, not a free one).
		if (S && H.CostsPower > S->GetPower())
		{
			LogEvent(TEXT("hint_requested"), FString::Printf(TEXT("script=%s tier=%d costsPower=%d affordable=0"), *Script->GetName(), H.Tier, H.CostsPower));
			Speak(FString::Printf(TEXT("That note draws %d and the bus is holding %d."), H.CostsPower, S->GetPower()));
			return FString();
		}
		if (S) { S->ChargePower(H.CostsPower); }
		Shift.HintsRevealed.Add(H.Tier);
		LogEvent(TEXT("hint_requested"), FString::Printf(TEXT("script=%s tier=%d costsPower=%d affordable=1"), *Script->GetName(), H.Tier, H.CostsPower));
		Speak(FString::Printf(TEXT("Ilse's note: %s"), *H.Text));
		OnShiftChanged.Broadcast();
		return H.Text;
	}
	Speak(TEXT("That is everything she wrote down. The rest is in the code."));
	return FString();
}

bool UGitsVantSubsystem::IsDiscounted(UGitsScript* Script)
{
	if (!Script) { return false; }
	FGitsShift& Shift = ShiftFor(Script);
	for (const FGitsPrediction& P : Script->Predictions)
	{
		const FGitsPredictionState& S = Shift.StateOf(P.Id);
		if (!S.Committed.IsEmpty() || S.bSatisfied) { return true; }
	}
	return false;
}

void UGitsVantSubsystem::NoteRun(UGitsScript* Script, int32 Cost, bool bDiscounted, int32 PowerAfter)
{
	LogEvent(TEXT("run_executed"), FString::Printf(TEXT("script=%s cost=%d discounted=%d powerAfter=%d"), Script ? *Script->GetName() : TEXT("none"), Cost, bDiscounted, PowerAfter));
	OnShiftChanged.Broadcast();
}

void UGitsVantSubsystem::NoteReserveDrawn(int32 Before, int32 After, int32 Draws)
{
	LogEvent(TEXT("reserve_drawn"), FString::Printf(TEXT("before=%d after=%d draws=%d"), Before, After, Draws));
}

void UGitsVantSubsystem::NoteEdit(UGitsScript* Script, int32 Line)
{
	if (!Script) { return; }
	ShiftFor(Script).InvalidateLine(Line, Script->Predictions);
	OnShiftChanged.Broadcast();
}

void UGitsVantSubsystem::TerminalUsed(UGitsScript* Script)
{
	if (!Script) { return; }
	const FString Key = KeyOf(Script);
	if (Introduced.Contains(Key)) { return; }
	Introduced.Add(Key);
	ShiftFor(Script);
	if (!Script->Intro.IsEmpty()) { Speak(Script->Intro); }
}

FString UGitsVantSubsystem::DescribeState(UGitsScript* Script)
{
	if (!Script) { return TEXT("no script"); }
	FGitsShift& Shift = ShiftFor(Script);
	FString Out;
	for (const FGitsPrediction& P : Script->Predictions)
	{
		const FGitsPredictionState& S = Shift.StateOf(P.Id);
		Out += FString::Printf(TEXT("%s: selected=%s committed=%s attempts=%d satisfied=%d locked=%d reachedStart=%d skipped=%d; "),
			*P.Id, *S.Selected, *S.Committed, S.Attempts, S.bSatisfied, S.bLocked, S.bReachedStartSinceLock, S.bSkipped);
	}
	Out += FString::Printf(TEXT("hints=%d"), Shift.HintsRevealed.Num());
	return Out;
}

// --- listening to the station -------------------------------------------------------------

void UGitsVantSubsystem::HandleRunStarted()
{
	UGitsStationSubsystem* S = Station();
	RunScript = S ? S->GetCurrentScript() : nullptr;
	RunAnchorSteps.Reset();
	RunFirstBoundary = 0;
	UGitsScript* Script = RunScript.Get();
	if (!S || !Script) { return; }
	const TArray<int32> Bounds = GitsTrace::StatementBoundaries(S->GetTrace());
	RunFirstBoundary = Bounds.Num() > 0 ? Bounds[0] : 0;
	FGitsShift& Shift = ShiftFor(Script);
	for (const FGitsPrediction& P : Script->Predictions)
	{
		int32 Found = 0;
		const int32 Step = FGitsShift::ResolveAnchor(S->GetTrace(), P.AnchorLine, P.AnchorOccurrence, Found);
		if (Step >= 0)
		{
			RunAnchorSteps.Add(P.Id, Step);
			Shift.MarkSkipped(P.Id, false);
		}
		else if (!Shift.StateOf(P.Id).bSatisfied)
		{
			// Anchors resolve only once there is a trace; a line that never ran is skipped, not a blocker.
			Shift.MarkSkipped(P.Id, true);
			LogEvent(TEXT("prediction_unresolvable"), FString::Printf(TEXT("script=%s prediction=%s line=%d occurrence=%d found=%d"), *Script->GetName(), *P.Id, P.AnchorLine, P.AnchorOccurrence, Found));
			if (!Shift.StateOf(P.Id).Committed.IsEmpty()) { Speak(FString::Printf(TEXT("Line %d never ran this time, so that reading is moot."), P.AnchorLine)); }
		}
	}
	OnShiftChanged.Broadcast();
}

void UGitsVantSubsystem::HandleStep(int32 StepIndex)
{
	UGitsScript* Script = RunScript.Get();
	UGitsStationSubsystem* S = Station();
	if (!S || !Script) { return; }
	FGitsShift& Shift = ShiftFor(Script);
	const bool bForward = S->IsPlaying();
	bool bChanged = false;
	for (const FGitsPrediction& P : Script->Predictions)
	{
		const int32* Anchor = RunAnchorSteps.Find(P.Id);
		if (!Anchor) { continue; }
		const FGitsPredictionState& State = Shift.StateOf(P.Id);
		if (!State.Committed.IsEmpty() && bForward && StepIndex >= *Anchor)
		{
			// The anchored line just ran: this is the only moment correctness is revealed.
			Settle(Script, P, true);
			bChanged = true;
		}
		else if (State.bLocked)
		{
			if (Shift.RecordHead(P.Id, StepIndex, RunFirstBoundary, *Anchor, bForward))
			{
				LogEvent(TEXT("scrub_gate_satisfied"), FString::Printf(TEXT("script=%s prediction=%s"), *Script->GetName(), *P.Id));
				Speak(TEXT("You watched it. Now tell me again what that line does."));
				bChanged = true;
			}
		}
	}
	if (bChanged) { OnShiftChanged.Broadcast(); }
}

bool UGitsVantSubsystem::IsComplete(const UGitsScript* Script) const
{
	return Script && OutroSpoken.Contains(KeyOf(Script));
}

const TSet<int32>& UGitsVantSubsystem::PassedTests(const UGitsScript* Script)
{
	return Passed.FindOrAdd(KeyOf(Script));
}

bool UGitsVantSubsystem::EvaluateGoal(UGitsScript* Script, UGitsStationSubsystem* S, FString& Reason)
{
	const FGitsRunSummary& Sum = S->GetLastSummary();
	const FGitsWorldState& World = S->GetCurrentWorld();
	// A reading script's system waits for the reading, not just the output: the panel can show
	// the right number while the player still holds the wrong belief about how it got there.
	if (Script->Predictions.Num() > 0 && !ShiftFor(Script).AllSatisfied(Script->Predictions))
	{
		Reason = TEXT("The reading is not yours yet.");
		return false;
	}
	if (!Script->GoalKey.IsEmpty())
	{
		const FGitsWorldValue* V = World.Find(Script->GoalKey);
		if (!V || V->ToText() != Script->GoalValue)
		{
			Reason = FString::Printf(TEXT("%s reads %s; the station wanted %s."), *Script->GoalKey, V ? *V->ToText() : TEXT("nothing"), *Script->GoalValue);
			return false;
		}
	}
	if (Script->GoalOutput.Num() > 0 && Sum.Output != Script->GoalOutput)
	{
		Reason = FString::Printf(TEXT("The panel read %s. It wanted %s."),
			Sum.Output.Num() ? *FString::Join(Sum.Output, TEXT(" / ")) : TEXT("nothing"), *FString::Join(Script->GoalOutput, TEXT(" / ")));
		return false;
	}
	TSet<int32>& PassedHere = Passed.FindOrAdd(KeyOf(Script));
	PassedHere.Reset();
	bool bAll = true;
	for (int32 i = 0; i < Script->TestCases.Num(); ++i)
	{
		const FGitsTestCase& T = Script->TestCases[i];
		bool bOk = true;
		FString Why;
		if (T.ExpectedOutput.Num() > 0 && Sum.Output != T.ExpectedOutput)
		{
			bOk = false;
			Why = FString::Printf(TEXT("got %s"), Sum.Output.Num() ? *FString::Join(Sum.Output, TEXT(" then ")) : TEXT("nothing at all"));
		}
		for (const auto& P : T.ExpectedWorld)
		{
			const FGitsWorldValue* V = World.Find(P.Key);
			if (!V || V->ToText() != P.Value)
			{
				bOk = false;
				Why = FString::Printf(TEXT("%s reads %s"), *P.Key, V ? *V->ToText() : TEXT("nothing"));
				break;
			}
		}
		if (bOk) { PassedHere.Add(i); }
		else if (bAll)
		{
			// Failures in the station's voice, never as assertion output (ADR-012).
			bAll = false;
			Reason = FString::Printf(TEXT("%s: %s."), *T.Label, *Why);
		}
	}
	return bAll;
}

void UGitsVantSubsystem::CheckSector()
{
	if (bSectorComplete) { return; }
	UGitsStationSubsystem* S = Station();
	if (!S) { return; }
	int32 Counted = 0;
	for (TActorIterator<AGitsTerminal> It(GetWorld()); It; ++It)
	{
		if (!It->Script || !It->Script->bCountsForSector || !It->Script->HasGoal()) { continue; }
		++Counted;
		if (!IsComplete(It->Script)) { return; }
	}
	if (Counted == 0) { return; }
	bSectorComplete = true;
	LogEvent(TEXT("sector_complete"), FString::Printf(TEXT("systems=%d"), Counted));
	for (TActorIterator<AGitsStation> It(GetWorld()); It; ++It)
	{
		FString K, V;
		if (It->SectorUnlocks.Split(TEXT("="), &K, &V)) { S->SetWorldValue(K, V); }
		if (!It->SectorCompleteLine.IsEmpty()) { Speak(It->SectorCompleteLine); }
		break;
	}
}

void UGitsVantSubsystem::TryComplete(UGitsScript* Script, bool bAfterRun)
{
	UGitsStationSubsystem* S = Station();
	if (!S || !Script || !Script->HasGoal() || !S->HasTrace() || S->GetCurrentScript() != Script) { return; }
	const FString Key = KeyOf(Script);
	if (OutroSpoken.Contains(Key)) { return; }
	FString Reason;
	const bool bMet = EvaluateGoal(Script, S, Reason);
	OnShiftChanged.Broadcast();
	if (!bMet)
	{
		LogEvent(TEXT("goal_missed"), FString::Printf(TEXT("script=%s reason=\"%s\""), *Script->GetName(), *Reason));
		if (!bAfterRun) { return; }
		// A Make script gets the station's verdict. A reading script whose output was right but
		// whose reading was wrong is told the system is waiting on the reading.
		if (Script->TestCases.Num() > 0) { Speak(Reason); }
		else if (Reason == TEXT("The reading is not yours yet."))
		{
			const FGitsRunSummary& Sum = S->GetLastSummary();
			Speak(FString::Printf(TEXT("The panel reads %s. The system holds until the reading is yours."), Sum.Output.Num() ? *FString::Join(Sum.Output, TEXT(" / ")) : TEXT("nothing")));
		}
		return;
	}
	OutroSpoken.Add(Key);
	LogEvent(TEXT("level_complete"), FString::Printf(TEXT("script=%s"), *Script->GetName()));
	FString K, V;
	if (Script->GoalUnlocks.Split(TEXT("="), &K, &V)) { S->SetWorldValue(K, V); }
	if (!Script->Outro.IsEmpty()) { Speak(Script->Outro); }
	CheckSector();
}

void UGitsVantSubsystem::HandleRunFinished()
{
	TryComplete(RunScript.Get(), true);
}
