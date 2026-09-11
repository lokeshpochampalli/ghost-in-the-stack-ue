#include "GitsPowerGauge.h"
#include "GitsStationSubsystem.h"
#include "GitsPower.h"
#include "UI/GitsScreen.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/World.h"

AGitsPowerGauge::AGitsPowerGauge()
{
	PrimaryActorTick.bCanEverTick = false;
	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	SetRootComponent(Body);
	Body->SetCollisionProfileName(TEXT("BlockAll"));

	Screen = CreateDefaultSubobject<UWidgetComponent>(TEXT("Screen"));
	Screen->SetupAttachment(Body);
	// Same panel as the wall display (face at x=-6, 120 x 70), placed at half scale by the level.
	Screen->SetRelativeLocation(FVector(-6.6f, 0.f, 35.f));
	Screen->SetRelativeRotation(FRotator(0.f, 180.f, 0.f));
	Screen->SetRelativeScale3D(FVector(0.1f));
	Screen->SetDrawSize(FVector2D(1140.f, 640.f));
	Screen->SetWidgetSpace(EWidgetSpace::World);
	Screen->SetWidgetClass(UGitsScreenWidget::StaticClass());
	Screen->SetCollisionProfileName(TEXT("NoCollision"));
	Screen->SetTwoSided(false);
}

void AGitsPowerGauge::BeginPlay()
{
	Super::BeginPlay();
	if (Screen)
	{
		Screen->InitWidget();
		if (UGitsScreenWidget* W = Cast<UGitsScreenWidget>(Screen->GetUserWidgetObject())) { W->FontSize = 44; }
	}
	if (UGitsStationSubsystem* S = GetWorld()->GetSubsystem<UGitsStationSubsystem>())
	{
		PowerHandle = S->OnPowerChanged.AddUObject(this, &AGitsPowerGauge::Refresh);
	}
	Refresh();
}

void AGitsPowerGauge::EndPlay(const EEndPlayReason::Type Reason)
{
	if (UWorld* World = GetWorld())
	{
		if (UGitsStationSubsystem* S = World->GetSubsystem<UGitsStationSubsystem>()) { S->OnPowerChanged.Remove(PowerHandle); }
	}
	Super::EndPlay(Reason);
}

void AGitsPowerGauge::Refresh()
{
	if (!Screen) { return; }
	UGitsScreenWidget* W = Cast<UGitsScreenWidget>(Screen->GetUserWidgetObject());
	UGitsStationSubsystem* S = GetWorld() ? GetWorld()->GetSubsystem<UGitsStationSubsystem>() : nullptr;
	if (!W || !S) { return; }
	const int32 Power = S->GetPower();
	const int32 Budget = S->GetPowerBudget();
	const GitsPower::EStage Stage = S->GetPowerStage();
	FGitsScreenModel M;
	M.Title = Title;
	// A bar of twenty cells: the kind of thing painted on a panel in 1978.
	const int32 Cells = 20;
	const int32 Lit = Budget > 0 ? FMath::Clamp(FMath::RoundToInt(Cells * (float)Power / (float)Budget), Power > 0 ? 1 : 0, Cells) : 0;
	FString Bar = TEXT("[");
	for (int32 i = 0; i < Cells; ++i) { Bar += i < Lit ? TEXT("#") : TEXT("."); }
	Bar += TEXT("]");
	M.MessageLines.Add(Bar);
	M.MessageLines.Add(FString::Printf(TEXT("holding %d of %d"), Power, Budget));
	if (S->GetReserveDraws() > 0) { M.MessageLines.Add(FString::Printf(TEXT("reserve drawn %d"), S->GetReserveDraws())); }
	switch (Stage)
	{
	case GitsPower::EStage::Nominal: M.Status = TEXT("nominal"); break;
	case GitsPower::EStage::Low: M.Status = TEXT("low: rig dimming"); break;
	case GitsPower::EStage::Critical: M.Status = TEXT("critical"); M.bStatusIsError = true; break;
	default: M.Status = TEXT("OUT. see the generator"); M.bStatusIsError = true; break;
	}
	W->SetModel(M);
	Screen->RequestRedraw();
}
