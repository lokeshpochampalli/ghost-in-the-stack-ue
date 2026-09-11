#include "GitsVent.h"
#include "Components/StaticMeshComponent.h"

AGitsVent::AGitsVent()
{
	Grille = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Grille"));
	SetRootComponent(Grille);
	Grille->SetCollisionProfileName(TEXT("BlockAll"));
	Fan = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Fan"));
	Fan->SetupAttachment(Grille);
	// The fan sits just behind the grille face (x=-7 on the kit piece) and spins about x.
	Fan->SetRelativeLocation(FVector(-4.f, 0.f, 40.f));
	Fan->SetCollisionProfileName(TEXT("NoCollision"));
	Fan->SetMobility(EComponentMobility::Movable);
	SystemId = TEXT("reclaimer");
}

void AGitsVent::PoseFromWorld(const FGitsWorldState& World, bool bInstant)
{
	bTargetOn = ReadBool(World, TEXT("vent.") + SystemId, false);
	if (bInstant) { Speed = bTargetOn ? FullSpeed : 0.f; }
}

void AGitsVent::ResetPose()
{
	bTargetOn = false;
	Speed = 0.f;
	Angle = 0.f;
	if (Fan) { Fan->SetRelativeRotation(FRotator::ZeroRotator); }
}

void AGitsVent::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	// Spins up and coasts down: an old fan does nothing suddenly.
	Speed = FMath::FInterpConstantTo(Speed, bTargetOn ? FullSpeed : 0.f, DeltaTime, FullSpeed * 0.6f);
	if (Speed > 0.f && Fan)
	{
		Angle = FMath::Fmod(Angle + Speed * DeltaTime, 360.f);
		Fan->SetRelativeRotation(FRotator(0.f, 0.f, Angle));
	}
}
