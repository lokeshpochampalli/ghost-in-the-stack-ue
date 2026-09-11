// Ghost in the Stack — the wall gauge: what the bus is holding, as a screen on the wall.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GitsPowerGauge.generated.h"

class UWidgetComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class GHOSTINTHESTACK_API AGitsPowerGauge : public AActor
{
	GENERATED_BODY()

public:
	AGitsPowerGauge();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Body;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UWidgetComponent> Screen;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gauge")
	FString Title = TEXT("POWER BUS");

	UFUNCTION(BlueprintCallable, Category = "Gauge")
	void Refresh();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
	FDelegateHandle PowerHandle;
};
