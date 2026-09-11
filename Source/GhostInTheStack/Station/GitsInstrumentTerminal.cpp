#include "GitsInstrumentTerminal.h"
#include "UI/GitsScreen.h"
#include "GhostInTheStackPlayerController.h"
#include "Telemetry/GitsTelemetry.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Pawn.h"

AGitsInstrumentTerminal::AGitsInstrumentTerminal()
{
	PrimaryActorTick.bCanEverTick = false;
	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	SetRootComponent(Body);
	Body->SetCollisionProfileName(TEXT("BlockAll"));
	Screen = CreateDefaultSubobject<UWidgetComponent>(TEXT("Screen"));
	Screen->SetupAttachment(Body);
	// The terminal head's front face is at x=-20, z 95..140, like the script terminals.
	Screen->SetRelativeLocation(FVector(-20.6f, 0.f, 117.5f));
	Screen->SetRelativeRotation(FRotator(0.f, 180.f, 0.f));
	Screen->SetRelativeScale3D(FVector(0.1f));
	Screen->SetDrawSize(FVector2D(660.f, 450.f));
	Screen->SetWidgetSpace(EWidgetSpace::World);
	Screen->SetWidgetClass(UGitsScreenWidget::StaticClass());
	Screen->SetCollisionProfileName(TEXT("NoCollision"));
	Screen->SetTwoSided(false);
}

void AGitsInstrumentTerminal::BeginPlay()
{
	Super::BeginPlay();
	if (Screen)
	{
		Screen->InitWidget();
		if (UGitsScreenWidget* W = Cast<UGitsScreenWidget>(Screen->GetUserWidgetObject())) { W->FontSize = 20; }
	}
	Refresh();
}

void AGitsInstrumentTerminal::Refresh()
{
	if (!Screen) { return; }
	UGitsScreenWidget* W = Cast<UGitsScreenWidget>(Screen->GetUserWidgetObject());
	if (!W) { return; }
	const UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	const UGitsTelemetrySubsystem* T = GI ? GI->GetSubsystem<UGitsTelemetrySubsystem>() : nullptr;
	FGitsScreenModel M;
	M.Title = TEXT("STUDY TERMINAL");
	M.MessageLines.Add(Occasion == TEXT("pre") ? TEXT("before you start: consent, and eleven short programs to read") : TEXT("before you go: eleven programs to read, and three questionnaires"));
	M.MessageLines.Add(TEXT(""));
	M.MessageLines.Add(T && T->HasConsent() ? FString::Printf(TEXT("participant %s"), *T->GetParticipantCode()) : TEXT("no consent recorded; nothing is being recorded"));
	M.Status = TEXT("press E to use");
	W->SetModel(M);
}

void AGitsInstrumentTerminal::Interact_Implementation(APawn* Player)
{
	if (!Player) { return; }
	if (AGhostInTheStackPlayerController* PC = Cast<AGhostInTheStackPlayerController>(Player->GetController())) { PC->UseInstrumentTerminal(this); }
}

FText AGitsInstrumentTerminal::GetInteractPrompt_Implementation() const
{
	return FText::FromString(TEXT("use the study terminal"));
}
