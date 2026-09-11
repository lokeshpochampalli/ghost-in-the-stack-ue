// Ghost in the Stack — a station system: an actor that poses itself from the world state.
//
// Systems never decide anything. They receive the folded world at the play head and
// animate toward it. A Blueprint subclass can react in OnStateChanged; C++ subclasses
// override PoseFromWorld.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interpreter/GitsTypes.h"
#include "GitsSystemActor.generated.h"

UCLASS(Abstract, Blueprintable)
class GHOSTINTHESTACK_API AGitsSystemActor : public AActor
{
	GENERATED_BODY()

public:
	AGitsSystemActor();

	/** The id scripts use: open_door("inner") drives the door whose SystemId is "inner". */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Station")
	FString SystemId = TEXT("main");

	/** Called by the station whenever the play head moves. bInstant skips animation. */
	virtual void PoseFromWorld(const FGitsWorldState& World, bool bInstant) {}

	/** Poses from the level's initial world before any run. */
	virtual void ResetPose() {}

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	/** Helpers for subclasses reading their own keys. */
	bool ReadBool(const FGitsWorldState& World, const FString& Key, bool Default) const;
	double ReadNumber(const FGitsWorldState& World, const FString& Key, double Default) const;
};

/** A sliding door. Opens when the world holds door.<SystemId>=true. */
UCLASS()
class GHOSTINTHESTACK_API AGitsDoor : public AGitsSystemActor
{
	GENERATED_BODY()

public:
	AGitsDoor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Frame;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Panel;

	/** How far the panel rises when open, in cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
	float OpenHeight = 215.f;

	/** Panel travel speed in cm per second. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
	float Speed = 180.f;

	UPROPERTY(BlueprintReadOnly, Category = "Door")
	bool bTargetOpen = false;

	UFUNCTION(BlueprintPure, Category = "Door")
	bool IsOpen() const { return bTargetOpen && FMath::IsNearlyEqual(PanelZ, OpenHeight, 1.f); }

	UFUNCTION(BlueprintPure, Category = "Door")
	bool IsMoving() const { return !FMath::IsNearlyEqual(PanelZ, bTargetOpen ? OpenHeight : 0.f, 0.5f); }

	/** Blueprint hook: the door was told to open or close. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Door")
	void OnStateChanged(bool bOpen);

	virtual void PoseFromWorld(const FGitsWorldState& World, bool bInstant) override;
	virtual void ResetPose() override;
	virtual void Tick(float DeltaTime) override;

private:
	float PanelZ = 0.f;
	void ApplyPanel();
};

/** A ceiling light. Brightness follows light.<SystemId> (0 to 10). */
UCLASS()
class GHOSTINTHESTACK_API AGitsLight : public AGitsSystemActor
{
	GENERATED_BODY()

public:
	AGitsLight();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UPointLightComponent> Light;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Fitting;

	/** Intensity at level 10, in candela. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light")
	float FullIntensity = 1200.f;

	/** Level before any script runs, 0 to 10. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light")
	float InitialLevel = 0.f;

	/** Emergency lighting: on only when the bus is out, off otherwise. Ignores the power factor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light")
	bool bEmergencyOnly = false;

	virtual void PoseFromWorld(const FGitsWorldState& World, bool bInstant) override;
	virtual void ResetPose() override;
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
	float TargetLevel = 0.f;
	float CurrentLevel = 0.f;
	FDelegateHandle PowerHandle;
	void ApplyLevel();
};
