// Ghost in the Stack — the shift clock: a wall panel that keeps station time once clock.<id> is true.
#pragma once

#include "CoreMinimal.h"
#include "GitsSystemActor.h"
#include "GitsClock.generated.h"

class UStaticMeshComponent;
class UWidgetComponent;

UCLASS(Blueprintable)
class GHOSTINTHESTACK_API AGitsClock : public AGitsSystemActor
{
	GENERATED_BODY()

public:
	AGitsClock();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Body;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UWidgetComponent> Screen;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clock")
	FString Title = TEXT("SHIFT CLOCK");

	/** Station time when the clock starts, in seconds since midnight. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clock")
	float StartSeconds = 6.f * 3600.f + 40.f * 60.f;

	/** How fast station time runs relative to real time; the demo does not last a shift. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clock")
	float TimeScale = 60.f;

	virtual void PoseFromWorld(const FGitsWorldState& World, bool bInstant) override;
	virtual void ResetPose() override;
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

private:
	void Refresh();
	bool bRunning = false;
	float Seconds = 0.f;
	int32 ShownMinute = -1;
};
