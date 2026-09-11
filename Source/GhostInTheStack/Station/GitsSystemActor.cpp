#include "GitsSystemActor.h"
#include "GitsStationSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/World.h"

// --- base ---------------------------------------------------------------------------

AGitsSystemActor::AGitsSystemActor()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AGitsSystemActor::BeginPlay()
{
	Super::BeginPlay();
	if (UGitsStationSubsystem* Station = GetWorld()->GetSubsystem<UGitsStationSubsystem>())
	{
		Station->RegisterSystem(this);
	}
	ResetPose();
}

void AGitsSystemActor::EndPlay(const EEndPlayReason::Type Reason)
{
	if (UWorld* World = GetWorld())
	{
		if (UGitsStationSubsystem* Station = World->GetSubsystem<UGitsStationSubsystem>()) { Station->UnregisterSystem(this); }
	}
	Super::EndPlay(Reason);
}

bool AGitsSystemActor::ReadBool(const FGitsWorldState& World, const FString& Key, bool Default) const
{
	const FGitsWorldValue* V = World.Find(Key);
	if (!V) { return Default; }
	switch (V->Kind)
	{
	case FGitsWorldValue::EKind::Bool: return V->Bool;
	case FGitsWorldValue::EKind::Number: return V->Number != 0.0;
	case FGitsWorldValue::EKind::String: return !V->String.IsEmpty();
	}
	return Default;
}

double AGitsSystemActor::ReadNumber(const FGitsWorldState& World, const FString& Key, double Default) const
{
	const FGitsWorldValue* V = World.Find(Key);
	if (!V) { return Default; }
	switch (V->Kind)
	{
	case FGitsWorldValue::EKind::Number: return V->Number;
	case FGitsWorldValue::EKind::Bool: return V->Bool ? 1.0 : 0.0;
	default: return Default;
	}
}

// --- door ---------------------------------------------------------------------------

AGitsDoor::AGitsDoor()
{
	Frame = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Frame"));
	SetRootComponent(Frame);
	Frame->SetCollisionProfileName(TEXT("BlockAll"));
	Panel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Panel"));
	Panel->SetupAttachment(Frame);
	Panel->SetCollisionProfileName(TEXT("BlockAll"));
	Panel->SetMobility(EComponentMobility::Movable);
	SystemId = TEXT("inner");
}

void AGitsDoor::PoseFromWorld(const FGitsWorldState& World, bool bInstant)
{
	const bool bOpen = ReadBool(World, TEXT("door.") + SystemId, false);
	if (bOpen != bTargetOpen)
	{
		bTargetOpen = bOpen;
		OnStateChanged(bOpen);
	}
	if (bInstant)
	{
		PanelZ = bTargetOpen ? OpenHeight : 0.f;
		ApplyPanel();
	}
}

void AGitsDoor::ResetPose()
{
	bTargetOpen = false;
	PanelZ = 0.f;
	ApplyPanel();
}

void AGitsDoor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	const float Target = bTargetOpen ? OpenHeight : 0.f;
	if (!FMath::IsNearlyEqual(PanelZ, Target, 0.01f))
	{
		PanelZ = FMath::FInterpConstantTo(PanelZ, Target, DeltaTime, Speed);
		ApplyPanel();
	}
}

void AGitsDoor::ApplyPanel()
{
	if (Panel) { Panel->SetRelativeLocation(FVector(0.f, 0.f, PanelZ)); }
}

// --- light --------------------------------------------------------------------------

AGitsLight::AGitsLight()
{
	Fitting = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Fitting"));
	SetRootComponent(Fitting);
	Fitting->SetCollisionProfileName(TEXT("NoCollision"));
	Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
	Light->SetupAttachment(Fitting);
	Light->SetRelativeLocation(FVector(0.f, 0.f, -20.f));
	Light->SetMobility(EComponentMobility::Movable);
	Light->SetLightColor(FLinearColor(1.f, 0.82f, 0.62f));
	Light->SetAttenuationRadius(900.f);
	Light->SetIntensityUnits(ELightUnits::Candelas);
	Light->SetIntensity(0.f);
	SystemId = TEXT("corridor");
}

void AGitsLight::PoseFromWorld(const FGitsWorldState& World, bool bInstant)
{
	if (!SwitchKey.IsEmpty()) { TargetLevel = ReadBool(World, SwitchKey, false) ? 10.f : 0.f; }
	else { TargetLevel = (float)FMath::Clamp(ReadNumber(World, TEXT("light.") + SystemId, InitialLevel), 0.0, 10.0); }
	if (bInstant) { CurrentLevel = TargetLevel; ApplyLevel(); }
}

void AGitsLight::ResetPose()
{
	TargetLevel = CurrentLevel = InitialLevel;
	ApplyLevel();
}

void AGitsLight::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!FMath::IsNearlyEqual(CurrentLevel, TargetLevel, 0.01f))
	{
		CurrentLevel = FMath::FInterpTo(CurrentLevel, TargetLevel, DeltaTime, 4.f);
		ApplyLevel();
	}
	else if (bBeacon && CurrentLevel > 0.f)
	{
		BeaconPhase += DeltaTime * 2.2f;
		ApplyLevel();
	}
}

void AGitsLight::BeginPlay()
{
	Super::BeginPlay();
	if (UGitsStationSubsystem* S = GetWorld()->GetSubsystem<UGitsStationSubsystem>())
	{
		PowerHandle = S->OnPowerChanged.AddUObject(this, &AGitsLight::ApplyLevel);
	}
	ApplyLevel();
}

void AGitsLight::EndPlay(const EEndPlayReason::Type Reason)
{
	if (UWorld* World = GetWorld())
	{
		if (UGitsStationSubsystem* S = World->GetSubsystem<UGitsStationSubsystem>()) { S->OnPowerChanged.Remove(PowerHandle); }
	}
	Super::EndPlay(Reason);
}

void AGitsLight::ApplyLevel()
{
	if (!Light) { return; }
	// The scripted level says what the light is asked for; the bus says what it gets.
	const UGitsStationSubsystem* S = GetWorld() ? GetWorld()->GetSubsystem<UGitsStationSubsystem>() : nullptr;
	const GitsPower::EStage Stage = S ? S->GetPowerStage() : GitsPower::EStage::Nominal;
	float Factor = GitsPower::LightFactor(Stage);
	if (bEmergencyOnly) { Factor = Stage == GitsPower::EStage::Out ? 1.f : 0.f; }
	const float Pulse = (bBeacon && CurrentLevel > 0.f) ? 0.55f + 0.45f * FMath::Sin(BeaconPhase) : 1.f;
	Light->SetIntensity(FullIntensity * CurrentLevel / 10.f * Factor * Pulse);
}
