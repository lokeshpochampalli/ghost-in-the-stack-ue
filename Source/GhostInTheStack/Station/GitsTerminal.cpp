#include "GitsTerminal.h"
#include "GitsScript.h"
#include "GitsStationSubsystem.h"
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
	Lines[LineNumber - 1] = Text.Replace(TEXT("\t"), TEXT("    ")).TrimEnd();
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
	if (!bInUse) { SelectedLine = 0; }
	if (!bInUse && !bStatusIsError && Status.IsEmpty()) { Status = IdlePrompt; }
	RefreshScreen();
}

FGitsRunSummary AGitsTerminal::RunCurrent()
{
	FGitsRunSummary Summary;
	if (UGitsStationSubsystem* S = Station())
	{
		bThisTerminalRan = true;
		S->Run(Script, GetSourceText(), Summary);
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
	if (!bThisTerminalRan) { return; }
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
		Status = FString::Printf(TEXT("running  line %d"), HighlightLine);
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
