#include "GitsFittings.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"

// --- heater -------------------------------------------------------------------------------

AGitsHeater::AGitsHeater()
{
	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	SetRootComponent(Body);
	Body->SetCollisionProfileName(TEXT("BlockAll"));
	Glow = CreateDefaultSubobject<UPointLightComponent>(TEXT("Glow"));
	Glow->SetupAttachment(Body);
	// In front of the fins (the piece's face is at -x), low, warm.
	Glow->SetRelativeLocation(FVector(-30.f, 0.f, 40.f));
	Glow->SetIntensityUnits(ELightUnits::Candelas);
	Glow->SetIntensity(0.f);
	Glow->SetLightColor(FLinearColor(1.0f, 0.45f, 0.15f));
	Glow->SetAttenuationRadius(320.f);
	Glow->SetCastShadows(false);
	Glow->SetMobility(EComponentMobility::Movable);
	SystemId = TEXT("corridor");
}

void AGitsHeater::PoseFromWorld(const FGitsWorldState& World, bool bInstant)
{
	Target = (float)ReadNumber(World, TEXT("heater.") + SystemId, 0.0);
	if (bInstant) { Current = Target; Apply(); }
}

void AGitsHeater::ResetPose() { Target = 0.f; Current = 0.f; Apply(); }

void AGitsHeater::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!FMath::IsNearlyEqual(Current, Target, 0.01f))
	{
		// Elements take their time: a heater winds up, it does not switch.
		Current = FMath::FInterpTo(Current, Target, DeltaTime, 1.2f);
		Apply();
	}
}

void AGitsHeater::Apply()
{
	if (Glow) { Glow->SetIntensity(FullIntensity * FMath::Clamp(Current / FMath::Max(FullSetting, 0.01f), 0.f, 1.f)); }
}

// --- sprinkler ----------------------------------------------------------------------------

AGitsSprinkler::AGitsSprinkler()
{
	Head = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Head"));
	SetRootComponent(Head);
	Head->SetCollisionProfileName(TEXT("NoCollision"));
	Spray = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Spray"));
	Spray->SetupAttachment(Head);
	Spray->SetRelativeLocation(FVector(0.f, 0.f, -2.f));
	Spray->SetRelativeScale3D(FVector(1.f, 1.f, 0.f));
	Spray->SetCollisionProfileName(TEXT("NoCollision"));
	Spray->SetMobility(EComponentMobility::Movable);
	Spray->SetCastShadow(false);
	Mist = CreateDefaultSubobject<UPointLightComponent>(TEXT("Mist"));
	Mist->SetupAttachment(Head);
	Mist->SetRelativeLocation(FVector(0.f, 0.f, -60.f));
	Mist->SetIntensityUnits(ELightUnits::Candelas);
	Mist->SetIntensity(0.f);
	Mist->SetLightColor(FLinearColor(0.6f, 0.8f, 1.0f));
	Mist->SetAttenuationRadius(260.f);
	Mist->SetCastShadows(false);
	Mist->SetMobility(EComponentMobility::Movable);
	SystemId = TEXT("channel");
}

void AGitsSprinkler::PoseFromWorld(const FGitsWorldState& World, bool bInstant)
{
	bTargetOn = ReadBool(World, TEXT("valve.") + SystemId, false);
	if (bInstant) { Amount = bTargetOn ? 1.f : 0.f; Apply(); }
}

void AGitsSprinkler::ResetPose() { bTargetOn = false; Amount = 0.f; Phase = 0.f; Apply(); }

void AGitsSprinkler::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	const float Want = bTargetOn ? 1.f : 0.f;
	if (!FMath::IsNearlyEqual(Amount, Want, 0.005f) || Amount > 0.f)
	{
		Amount = FMath::FInterpConstantTo(Amount, Want, DeltaTime, 1.6f);
		Phase += DeltaTime * 9.f;
		Apply();
	}
}

void AGitsSprinkler::Apply()
{
	// The spray column grows down from the head and shimmers a little while it runs.
	// A thin column, not a pillar: the kit piece is 28 cm across, the spray a third of that.
	const float Shimmer = (Amount > 0.f ? 1.f + 0.08f * FMath::Sin(Phase) : 1.f) * 0.3f;
	if (Spray) { Spray->SetRelativeScale3D(FVector(Shimmer, Shimmer, Amount * 0.7f)); }
	if (Mist) { Mist->SetIntensity(2.5f * Amount); }
}

// --- conveyor -----------------------------------------------------------------------------

AGitsConveyor::AGitsConveyor()
{
	Belt = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Belt"));
	SetRootComponent(Belt);
	Belt->SetCollisionProfileName(TEXT("BlockAll"));
	SystemId = TEXT("line");
}

void AGitsConveyor::BeginPlay()
{
	for (int32 i = 0; i < CrateCount; ++i)
	{
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this, *FString::Printf(TEXT("Crate%d"), i));
		C->SetupAttachment(Belt);
		C->SetStaticMesh(CrateMesh);
		C->SetCollisionProfileName(TEXT("NoCollision"));
		C->SetMobility(EComponentMobility::Movable);
		C->RegisterComponent();
		Crates.Add(C);
	}
	Place();
	// After the crates exist, so the first pose finds them.
	Super::BeginPlay();
}

void AGitsConveyor::PoseFromWorld(const FGitsWorldState& World, bool bInstant)
{
	bTargetOn = ReadBool(World, TEXT("conveyor.") + SystemId, false);
	if (bInstant) { Speed = bTargetOn ? FullSpeed : 0.f; }
}

void AGitsConveyor::ResetPose() { bTargetOn = false; Speed = 0.f; Offset = 0.f; Place(); }

void AGitsConveyor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Speed = FMath::FInterpConstantTo(Speed, bTargetOn ? FullSpeed : 0.f, DeltaTime, FullSpeed * 0.8f);
	if (Speed > 0.f)
	{
		Offset = FMath::Fmod(Offset + Speed * DeltaTime, Length);
		Place();
	}
}

void AGitsConveyor::Place()
{
	// Crates ride the top of the belt (z 40 on the kit piece) and wrap around its length.
	const float Spacing = Length / FMath::Max(1, Crates.Num());
	for (int32 i = 0; i < Crates.Num(); ++i)
	{
		const float X = FMath::Fmod(Offset + i * Spacing, Length) - Length * 0.5f + 30.f;
		if (Crates[i]) { Crates[i]->SetRelativeLocation(FVector(X, 0.f, 40.f)); }
	}
}
