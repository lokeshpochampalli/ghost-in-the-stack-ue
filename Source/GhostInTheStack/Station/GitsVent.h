// Ghost in the Stack — a vent: a grille whose fan spins when vent.<id> is true.
#pragma once

#include "CoreMinimal.h"
#include "GitsSystemActor.h"
#include "GitsVent.generated.h"

class UStaticMeshComponent;

UCLASS(Blueprintable)
class GHOSTINTHESTACK_API AGitsVent : public AGitsSystemActor
{
	GENERATED_BODY()

public:
	AGitsVent();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Grille;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Fan;

	/** Fan speed when running, in degrees per second. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vent")
	float FullSpeed = 540.f;

	UFUNCTION(BlueprintPure, Category = "Vent")
	bool IsRunning() const { return bTargetOn; }

	virtual void PoseFromWorld(const FGitsWorldState& World, bool bInstant) override;
	virtual void ResetPose() override;
	virtual void Tick(float DeltaTime) override;

private:
	bool bTargetOn = false;
	float Speed = 0.f;
	float Angle = 0.f;
};
