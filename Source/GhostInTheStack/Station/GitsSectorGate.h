// Ghost in the Stack — the way through to the next sector: a hatch that opens only when the
// world says the sector's airlock is released, and then loads the next map.
#pragma once

#include "CoreMinimal.h"
#include "GitsSystemActor.h"
#include "GitsInteractable.h"
#include "GitsSectorGate.generated.h"

class UStaticMeshComponent;
class UWidgetComponent;

UCLASS(Blueprintable)
class GHOSTINTHESTACK_API AGitsSectorGate : public AGitsSystemActor, public IGitsInteractable
{
	GENERATED_BODY()

public:
	AGitsSectorGate();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Frame;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Panel;

	/** The sign over the hatch. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UWidgetComponent> Sign;

	/** The world key that releases the gate, e.g. door.inner. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gate")
	FString DoorKey = TEXT("door.inner");

	/** The map to load: /Game/Sectors/L_Sector2_Greenhouse. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gate")
	FString NextLevel;

	/** What the sign says: "SECTOR 2  GREENHOUSE". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gate")
	FString Label;

	/** VANT's line when the gate is tried sealed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gate", meta = (MultiLine = true))
	FString SealedLine = TEXT("Sealed. Every system in this sector has to run before the station lets you through.");

	UFUNCTION(BlueprintPure, Category = "Gate")
	bool IsReleased() const { return bReleased; }

	UFUNCTION(BlueprintCallable, Category = "Gate")
	void Refresh();

	virtual void PoseFromWorld(const FGitsWorldState& World, bool bInstant) override;
	virtual void ResetPose() override;

	// IGitsInteractable
	virtual void Interact_Implementation(APawn* Player) override;
	virtual FText GetInteractPrompt_Implementation() const override;

protected:
	virtual void BeginPlay() override;

private:
	bool bReleased = false;
};
