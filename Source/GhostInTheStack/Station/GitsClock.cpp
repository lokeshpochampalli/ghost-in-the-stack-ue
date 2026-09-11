#include "GitsClock.h"
#include "UI/GitsScreen.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"

AGitsClock::AGitsClock()
{
	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	SetRootComponent(Body);
	Body->SetCollisionProfileName(TEXT("BlockAll"));
	Screen = CreateDefaultSubobject<UWidgetComponent>(TEXT("Screen"));
	Screen->SetupAttachment(Body);
	// The wall gauge panel: face at x=-6, 120 x 70, placed at half scale by the level.
	Screen->SetRelativeLocation(FVector(-6.6f, 0.f, 35.f));
	Screen->SetRelativeRotation(FRotator(0.f, 180.f, 0.f));
	Screen->SetRelativeScale3D(FVector(0.1f));
	Screen->SetDrawSize(FVector2D(1140.f, 640.f));
	Screen->SetWidgetSpace(EWidgetSpace::World);
	Screen->SetWidgetClass(UGitsScreenWidget::StaticClass());
	Screen->SetCollisionProfileName(TEXT("NoCollision"));
	Screen->SetTwoSided(false);
	SystemId = TEXT("shift");
}

void AGitsClock::BeginPlay()
{
	Super::BeginPlay();
	if (Screen)
	{
		Screen->InitWidget();
		if (UGitsScreenWidget* W = Cast<UGitsScreenWidget>(Screen->GetUserWidgetObject())) { W->FontSize = 64; }
	}
	Seconds = StartSeconds;
	Refresh();
}

void AGitsClock::PoseFromWorld(const FGitsWorldState& World, bool bInstant)
{
	const bool bOn = ReadBool(World, TEXT("clock.") + SystemId, false);
	if (bOn != bRunning) { bRunning = bOn; ShownMinute = -1; Refresh(); }
}

void AGitsClock::ResetPose()
{
	bRunning = false;
	Seconds = StartSeconds;
	ShownMinute = -1;
	Refresh();
}

void AGitsClock::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!bRunning) { return; }
	Seconds = FMath::Fmod(Seconds + DeltaTime * TimeScale, 24.f * 3600.f);
	const int32 Minute = (int32)(Seconds / 60.f);
	if (Minute != ShownMinute) { ShownMinute = Minute; Refresh(); }
}

void AGitsClock::Refresh()
{
	if (!Screen) { return; }
	UGitsScreenWidget* W = Cast<UGitsScreenWidget>(Screen->GetUserWidgetObject());
	if (!W) { return; }
	FGitsScreenModel M;
	M.Title = Title;
	if (bRunning)
	{
		const int32 Minute = (int32)(Seconds / 60.f);
		M.MessageLines.Add(FString::Printf(TEXT("%02d:%02d"), (Minute / 60) % 24, Minute % 60));
		M.Status = TEXT("shift two");
	}
	else
	{
		M.MessageLines.Add(TEXT("--:--"));
		M.Status = TEXT("no shift set");
		M.bStatusIsError = true;
	}
	W->SetModel(M);
	Screen->RequestRedraw();
}
