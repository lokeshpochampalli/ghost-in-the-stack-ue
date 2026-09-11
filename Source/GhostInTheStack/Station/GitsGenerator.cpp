#include "GitsGenerator.h"
#include "GitsStationSubsystem.h"
#include "Companion/GitsVant.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"

AGitsGenerator::AGitsGenerator()
{
	PrimaryActorTick.bCanEverTick = false;
	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	SetRootComponent(Body);
	Body->SetCollisionProfileName(TEXT("BlockAll"));
}

int32 AGitsGenerator::Draw()
{
	UGitsStationSubsystem* S = GetWorld() ? GetWorld()->GetSubsystem<UGitsStationSubsystem>() : nullptr;
	UGitsVantSubsystem* V = GetWorld() ? GetWorld()->GetSubsystem<UGitsVantSubsystem>() : nullptr;
	if (!S) { return -1; }
	const int32 Before = S->GetPower();
	const int32 After = S->DrawReserve();
	if (After < 0)
	{
		if (V) { V->Speak(FString::Printf(TEXT("The bus is holding %d. The reserve cell would not add to that."), Before)); }
		return -1;
	}
	if (V)
	{
		V->NoteReserveDrawn(Before, After, S->GetReserveDraws());
		V->Speak(FString::Printf(TEXT("Reserve cell drawn. The bus is holding %d. Ilse left one of these; it will not keep doing this."), After));
	}
	return After;
}

void AGitsGenerator::Interact_Implementation(APawn* Player)
{
	Draw();
}

FText AGitsGenerator::GetInteractPrompt_Implementation() const
{
	return FText::FromString(TEXT("draw the reserve cell"));
}
