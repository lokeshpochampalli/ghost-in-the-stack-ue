#include "GitsSectorGate.h"
#include "Companion/GitsVant.h"
#include "UI/GitsScreen.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "GhostInTheStack.h"

AGitsSectorGate::AGitsSectorGate()
{
	Frame = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Frame"));
	SetRootComponent(Frame);
	Frame->SetCollisionProfileName(TEXT("BlockAll"));
	Panel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Panel"));
	Panel->SetupAttachment(Frame);
	Panel->SetCollisionProfileName(TEXT("BlockAll"));
	Sign = CreateDefaultSubobject<UWidgetComponent>(TEXT("Sign"));
	Sign->SetupAttachment(Frame);
	// Over the lintel, facing the way the player comes (the frame's -x side).
	Sign->SetRelativeLocation(FVector(-18.f, 0.f, 262.f));
	Sign->SetRelativeRotation(FRotator(0.f, 180.f, 0.f));
	Sign->SetRelativeScale3D(FVector(0.1f));
	Sign->SetDrawSize(FVector2D(1400.f, 300.f));
	Sign->SetWidgetSpace(EWidgetSpace::World);
	Sign->SetWidgetClass(UGitsScreenWidget::StaticClass());
	Sign->SetCollisionProfileName(TEXT("NoCollision"));
	Sign->SetTwoSided(false);
	SystemId = TEXT("gate");
}

void AGitsSectorGate::BeginPlay()
{
	if (Sign)
	{
		Sign->InitWidget();
		if (UGitsScreenWidget* W = Cast<UGitsScreenWidget>(Sign->GetUserWidgetObject())) { W->FontSize = 40; }
	}
	Super::BeginPlay();
}

void AGitsSectorGate::PoseFromWorld(const FGitsWorldState& World, bool bInstant)
{
	const bool bWas = bReleased;
	bReleased = ReadBool(World, DoorKey, false);
	if (bWas != bReleased) { Refresh(); }
}

void AGitsSectorGate::ResetPose()
{
	bReleased = false;
	Refresh();
}

void AGitsSectorGate::Refresh()
{
	if (!Sign) { return; }
	UGitsScreenWidget* W = Cast<UGitsScreenWidget>(Sign->GetUserWidgetObject());
	if (!W) { return; }
	FGitsScreenModel M;
	M.Title = Label;
	M.Status = bReleased ? TEXT("released   E to go through") : TEXT("SEALED");
	M.bStatusIsError = !bReleased;
	W->SetModel(M);
}

void AGitsSectorGate::Interact_Implementation(APawn* Player)
{
	if (!bReleased)
	{
		if (UGitsVantSubsystem* V = GetWorld() ? GetWorld()->GetSubsystem<UGitsVantSubsystem>() : nullptr) { V->Speak(SealedLine); }
		return;
	}
	if (NextLevel.IsEmpty()) { UE_LOG(LogGhostInTheStack, Warning, TEXT("sector gate %s has no next level"), *GetName()); return; }
	UE_LOG(LogGhostInTheStack, Display, TEXT("sector gate: through to %s"), *NextLevel);
	UGameplayStatics::OpenLevel(this, FName(*NextLevel));
}

FText AGitsSectorGate::GetInteractPrompt_Implementation() const
{
	return FText::FromString(bReleased ? TEXT("go through") : TEXT("sealed"));
}
