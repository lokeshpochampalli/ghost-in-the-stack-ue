// Ghost in the Stack — the fittings Sectors 2 to 4 add: a heater, a sprinkler, a conveyor.
//
// Each poses itself from one world key and animates toward it. None of them decides anything.
#pragma once

#include "CoreMinimal.h"
#include "GitsSystemActor.h"
#include "GitsFittings.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;

/** A wall heater. Its glow follows heater.<SystemId> (0 to 3, the ladder's settings). */
UCLASS(Blueprintable)
class GHOSTINTHESTACK_API AGitsHeater : public AGitsSystemActor
{
	GENERATED_BODY()

public:
	AGitsHeater();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Body;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPointLightComponent> Glow;

	/** The setting that means full glow. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Heater")
	float FullSetting = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Heater")
	float FullIntensity = 8.f;

	UFUNCTION(BlueprintPure, Category = "Heater")
	float GetSetting() const { return Target; }

	virtual void PoseFromWorld(const FGitsWorldState& World, bool bInstant) override;
	virtual void ResetPose() override;
	virtual void Tick(float DeltaTime) override;

private:
	float Target = 0.f;
	float Current = 0.f;
	void Apply();
};

/** A sprinkler head. Sprays while valve.<SystemId> is true. */
UCLASS(Blueprintable)
class GHOSTINTHESTACK_API AGitsSprinkler : public AGitsSystemActor
{
	GENERATED_BODY()

public:
	AGitsSprinkler();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Head;

	/** The spray: scaled from nothing to full while running. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Spray;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPointLightComponent> Mist;

	UFUNCTION(BlueprintPure, Category = "Sprinkler")
	bool IsRunning() const { return bTargetOn; }

	virtual void PoseFromWorld(const FGitsWorldState& World, bool bInstant) override;
	virtual void ResetPose() override;
	virtual void Tick(float DeltaTime) override;

private:
	bool bTargetOn = false;
	float Amount = 0.f;
	float Phase = 0.f;
	void Apply();
};

/** A conveyor. Its crates run along the belt while conveyor.<SystemId> is true. */
UCLASS(Blueprintable)
class GHOSTINTHESTACK_API AGitsConveyor : public AGitsSystemActor
{
	GENERATED_BODY()

public:
	AGitsConveyor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Belt;

	/** Crates riding the belt, spaced along its length. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TArray<TObjectPtr<UStaticMeshComponent>> Crates;

	/** The belt's length along its local x, in cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conveyor")
	float Length = 400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conveyor")
	float FullSpeed = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conveyor")
	int32 CrateCount = 3;

	/** The crate mesh; set from the level. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conveyor")
	TObjectPtr<UStaticMesh> CrateMesh;

	UFUNCTION(BlueprintPure, Category = "Conveyor")
	bool IsRunning() const { return bTargetOn; }

	virtual void PoseFromWorld(const FGitsWorldState& World, bool bInstant) override;
	virtual void ResetPose() override;
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

private:
	bool bTargetOn = false;
	float Speed = 0.f;
	float Offset = 0.f;
	void Place();
};
