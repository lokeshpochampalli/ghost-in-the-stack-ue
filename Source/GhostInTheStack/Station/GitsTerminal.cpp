#include "GitsTerminal.h"
#include "GitsScript.h"
#include "GitsStationSubsystem.h"
#include "Companion/GitsVant.h"
#include "Telemetry/GitsTelemetry.h"
#include "Engine/GameInstance.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GhostInTheStackPlayerController.h"

AGitsTerminal::AGitsTerminal()
{
	PrimaryActorTick.bCanEverTick = false;
	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	SetRootComponent(Body);
	Body->SetCollisionProfileName(TEXT("BlockAll"));

	Screen = CreateDefaultSubobject<UWidgetComponent>(TEXT("Screen"));
	Screen->SetupAttachment(Body);
	// The terminal head's front face is at x=-20, z 95..140 (see Tools/export_kit.py). The
	// widget quad faces +X, so turn it to face the player standing at -X.
	Screen->SetRelativeLocation(FVector(-20.6f, 0.f, 117.5f));
	Screen->SetRelativeRotation(FRotator(0.f, 180.f, 0.f));
	Screen->SetRelativeScale3D(FVector(0.1f));
	Screen->SetDrawSize(FVector2D(660.f, 450.f));
	Screen->SetWidgetSpace(EWidgetSpace::World);
	Screen->SetWidgetClass(UGitsScreenWidget::StaticClass());
	Screen->SetCollisionProfileName(TEXT("NoCollision"));
	Screen->SetTwoSided(false);
	Screen->SetDrawAtDesiredSize(false);
	Screen->SetPivot(FVector2D(0.5f, 0.5f));
}

void AGitsTerminal::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (Lines.Num() == 0) { ResetToScript(); }
}

void AGitsTerminal::BeginPlay()
{
	Super::BeginPlay();
	ResetToScript();
	if (Screen)
	{
		Screen->InitWidget();
		if (UGitsScreenWidget* W = Cast<UGitsScreenWidget>(Screen->GetUserWidgetObject())) { W->FontSize = 18; W->MaxCodeRows = 12; W->MaxMessageRows = 2; }
	}
	Status = IdlePrompt;
	if (UGitsStationSubsystem* S = Station())
	{
		StepHandle = S->OnStepChanged.AddUObject(this, &AGitsTerminal::HandleStep);
		StartHandle = S->OnRunStarted.AddUObject(this, &AGitsTerminal::HandleRunStarted);
		FinishHandle = S->OnRunFinished.AddUObject(this, &AGitsTerminal::HandleRunFinished);
		MessageHandle = S->OnMessage.AddUObject(this, &AGitsTerminal::HandleMessage);
	}
	RefreshScreen();
}

void AGitsTerminal::EndPlay(const EEndPlayReason::Type Reason)
{
	if (UGitsStationSubsystem* S = Station())
	{
		S->OnStepChanged.Remove(StepHandle);
		S->OnRunStarted.Remove(StartHandle);
		S->OnRunFinished.Remove(FinishHandle);
		S->OnMessage.Remove(MessageHandle);
	}
	Super::EndPlay(Reason);
}

UGitsStationSubsystem* AGitsTerminal::Station() const
{
	return GetWorld() ? GetWorld()->GetSubsystem<UGitsStationSubsystem>() : nullptr;
}

// --- source -------------------------------------------------------------------------------

void AGitsTerminal::ResetToScript()
{
	Lines = Script ? Script->Lines() : TArray<FString>();
}

FString AGitsTerminal::GetSourceText() const
{
	return FString::Join(Lines, TEXT("\n")) + TEXT("\n");
}

bool AGitsTerminal::IsLineEditable(int32 LineNumber) const
{
	if (LineNumber < 1 || LineNumber > Lines.Num()) { return false; }
	if (!Script || Script->EditableLines.Num() == 0) { return true; }
	return Script->EditableLines.Contains(LineNumber);
}

bool AGitsTerminal::SetLine(int32 LineNumber, const FString& Text)
{
	if (!IsLineEditable(LineNumber)) { return false; }
	const FString Clean = Text.Replace(TEXT("\t"), TEXT("    ")).TrimEnd();
	const bool bChanged = Lines[LineNumber - 1] != Clean;
	const FString Before = Lines[LineNumber - 1];
	Lines[LineNumber - 1] = Clean;
	if (bChanged)
	{
		if (UGitsVantSubsystem* V = GetWorld() ? GetWorld()->GetSubsystem<UGitsVantSubsystem>() : nullptr) { V->NoteEdit(Script, LineNumber); }
		// The log decides whether the text is kept (ADR-014); the terminal passes it unconditionally.
		if (const UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
		{
			if (UGitsTelemetrySubsystem* T = GI->GetSubsystem<UGitsTelemetrySubsystem>()) { if (T->HasConsent()) { T->Edit(LineNumber, Before, Clean); } }
		}
	}
	RefreshScreen();
	return true;
}

bool AGitsTerminal::IsFreeEdit() const
{
	return Script && Script->bFreeEdit;
}

int32 AGitsTerminal::InsertLineAfter(int32 LineNumber)
{
	if (!IsFreeEdit() || LineNumber < 0 || LineNumber > Lines.Num() || Lines.Num() >= 40) { return 0; }
	Lines.Insert(FString(), LineNumber);
	if (UGitsVantSubsystem* V = GetWorld() ? GetWorld()->GetSubsystem<UGitsVantSubsystem>() : nullptr) { V->NoteEdit(Script, LineNumber + 1); }
	RefreshScreen();
	return LineNumber + 1;
}

bool AGitsTerminal::RemoveLine(int32 LineNumber)
{
	if (!IsFreeEdit() || LineNumber < 1 || LineNumber > Lines.Num() || Lines.Num() <= 1) { return false; }
	Lines.RemoveAt(LineNumber - 1);
	if (UGitsVantSubsystem* V = GetWorld() ? GetWorld()->GetSubsystem<UGitsVantSubsystem>() : nullptr) { V->NoteEdit(Script, LineNumber); }
	RefreshScreen();
	return true;
}

void AGitsTerminal::SetSelectedLine(int32 LineNumber)
{
	SelectedLine = FMath::Clamp(LineNumber, 0, Lines.Num());
	RefreshScreen();
}

void AGitsTerminal::SetInUse(bool bNewInUse)
{
	bInUse = bNewInUse;
	if (bInUse)
	{
		if (UGitsVantSubsystem* V = GetWorld() ? GetWorld()->GetSubsystem<UGitsVantSubsystem>() : nullptr) { V->TerminalUsed(Script); }
	}
	if (!bInUse) { SelectedLine = 0; }
	if (!bInUse && !bStatusIsError && Status.IsEmpty()) { Status = IdlePrompt; }
	RefreshScreen();
}

int32 AGitsTerminal::RunCostNow(bool& bDiscounted) const
{
	UGitsVantSubsystem* V = GetWorld() ? GetWorld()->GetSubsystem<UGitsVantSubsystem>() : nullptr;
	bDiscounted = V && Script && V->IsDiscounted(Script);
	if (!Script) { return 0; }
	return bDiscounted ? Script->PredictedRunCost : Script->RunCost;
}

FGitsRunSummary AGitsTerminal::RunCurrent()
{
	FGitsRunSummary Summary;
	UGitsVantSubsystem* V = GetWorld() ? GetWorld()->GetSubsystem<UGitsVantSubsystem>() : nullptr;
	UGitsStationSubsystem* S = Station();
	auto Refuse = [this, &Summary, V](const FString& Reason)
	{
		Summary.bRefused = true;
		Summary.Message = Reason;
		Status = TEXT("VANT: ") + Reason;
		bStatusIsError = true;
		RefreshScreen();
		if (V) { V->Speak(Reason); }
	};
	// VANT will not spend the power before the player has said what they expect (ADR-006)...
	FString Reason;
	if (V && !V->CanRun(Script, Reason)) { Refuse(Reason); return Summary; }
	// ...and the bus has to cover the run. A committed or confirmed reading buys the discount.
	bool bDiscounted = false;
	const int32 Cost = RunCostNow(bDiscounted);
	if (S && Cost > S->GetPower())
	{
		Refuse(FString::Printf(TEXT("This run draws %d and the bus is holding %d. Ilse left a reserve cell on the generator."), Cost, S->GetPower()));
		return Summary;
	}
	if (S)
	{
		bThisTerminalRan = true;
		S->Run(Script, GetSourceText(), Summary);
		if (Summary.bRan)
		{
			S->ChargePower(Cost);
			if (V) { V->NoteRun(Script, Cost, bDiscounted, S->GetPower()); }
		}
		else if (Summary.bParseFailed && V) { V->NoteError(Script, Summary.DiagnosticCode, Summary.DiagnosticLine); }
		if (!Summary.bRan)
		{
			Status = Summary.Message;
			bStatusIsError = true;
			HighlightLine = 0;
			RefreshScreen();
		}
	}
	return Summary;
}

// --- station events ---------------------------------------------------------------------

void AGitsTerminal::HandleRunStarted()
{
	// A run belongs to the terminal whose script it came from; the others go quiet.
	if (UGitsStationSubsystem* S = Station()) { bThisTerminalRan = Script && S->GetCurrentScript() == Script; }
	if (!bThisTerminalRan)
	{
		if (HighlightLine != 0 || Status != IdlePrompt) { HighlightLine = 0; Status = IdlePrompt; bStatusIsError = false; RefreshScreen(); }
		return;
	}
	HighlightLine = 0;
	Status = TEXT("running");
	bStatusIsError = false;
	RefreshScreen();
}

void AGitsTerminal::HandleStep(int32 StepIndex)
{
	if (!bThisTerminalRan) { return; }
	if (UGitsStationSubsystem* S = Station())
	{
		HighlightLine = S->CurrentLine();
		if (S->IsRewinding()) { Status = HighlightLine > 0 ? FString::Printf(TEXT("rewind  line %d"), HighlightLine) : TEXT("rewind  before the first line"); }
		else if (S->IsPlaying()) { Status = HighlightLine > 0 ? FString::Printf(TEXT("running  line %d"), HighlightLine) : TEXT("running"); }
		else { Status = TEXT("done"); }
		bStatusIsError = false;
	}
	RefreshScreen();
}

void AGitsTerminal::HandleRunFinished()
{
	if (!bThisTerminalRan) { return; }
	if (UGitsStationSubsystem* S = Station())
	{
		const FGitsRunSummary& Sum = S->GetLastSummary();
		const bool bCompleted = Sum.Outcome == TEXT("completed");
		Status = bCompleted ? TEXT("done") : Sum.Message;
		bStatusIsError = !bCompleted;
		if (!bCompleted && S->HasTrace()) { HighlightLine = S->GetTrace().Outcome.Diagnostic.Span.Start.Line; }
	}
	RefreshScreen();
}

void AGitsTerminal::HandleMessage(const FString& Message)
{
	if (!bThisTerminalRan) { return; }
	if (UGitsStationSubsystem* S = Station())
	{
		if (S->GetLastSummary().bRefused) { Status = TEXT("VANT: ") + Message; bStatusIsError = true; RefreshScreen(); }
	}
}

// --- screen -----------------------------------------------------------------------------

FGitsScreenModel AGitsTerminal::BuildModel(bool bForOverlay) const
{
	FGitsScreenModel M;
	M.Title = Script ? Script->Title : TEXT("terminal");
	M.CodeLines = Lines;
	M.HighlightLine = HighlightLine;
	M.SelectedLine = bForOverlay ? SelectedLine : 0;
	if (Script) { M.EditableLines = Script->EditableLines; }
	M.Status = bForOverlay ? Status : (bInUse ? TEXT("in use") : Status);
	M.bStatusIsError = bStatusIsError;
	// Once the system works, the idle screen carries Ilse's journal entry: her logs live in the world.
	if (!bForOverlay && !bInUse && Script && !Script->LogEntry.IsEmpty())
	{
		if (const UGitsVantSubsystem* V = GetWorld() ? GetWorld()->GetSubsystem<UGitsVantSubsystem>() : nullptr)
		{
			if (V->IsComplete(Script))
			{
				M.MessageLines.Add(TEXT("VANT: her log, from the terminal's memory:"));
				M.MessageLines.Add(Script->LogEntry);
			}
		}
	}
	return M;
}

void AGitsTerminal::RefreshScreen()
{
	if (!Screen) { return; }
	if (UGitsScreenWidget* W = Cast<UGitsScreenWidget>(Screen->GetUserWidgetObject()))
	{
		W->SetModel(BuildModel(false));
	}
}

// --- interaction --------------------------------------------------------------------------

void AGitsTerminal::Interact_Implementation(APawn* Player)
{
	if (!Player) { return; }
	if (AGhostInTheStackPlayerController* PC = Cast<AGhostInTheStackPlayerController>(Player->GetController()))
	{
		PC->UseTerminal(this);
	}
}

FText AGitsTerminal::GetInteractPrompt_Implementation() const
{
	return FText::FromString(TEXT("use terminal"));
}
