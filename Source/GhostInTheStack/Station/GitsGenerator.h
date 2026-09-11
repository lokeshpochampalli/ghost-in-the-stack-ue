// Ghost in the Stack — the generator: Ilse's reserve cell, the way back from a dead bus.
//
// Running out of power is a setback with a diegetic way out, not a game over screen. Every
// draw is recorded, so the data shows who needed it without the game punishing them for it.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GitsInteractable.h"
#include "GitsGenerator.generated.h"

class UStaticMeshComponent;

UCLASS(Blueprintable)
class GHOSTINTHESTACK_API AGitsGenerator : public AActor, public IGitsInteractable
{
	GENERATED_BODY()

public:
	AGitsGenerator();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Body;

	/** Draws the reserve cell. Returns the bus reading afterwards, or -1 if there was nothing to draw. */
	UFUNCTION(BlueprintCallable, Category = "Station")
	int32 Draw();

	// IGitsInteractable
	virtual void Interact_Implementation(APawn* Player) override;
	virtual FText GetInteractPrompt_Implementation() const override;
};
