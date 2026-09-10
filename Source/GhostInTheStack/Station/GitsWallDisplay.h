// Ghost in the Stack — a wall display: what the code actually produced.
//
// Shows the station log as a run plays: printed lines, effects as plain sentences, and
// at the end the outcome or VANT's message. It reads the station's play head; it never
// runs anything.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GitsWallDisplay.generated.h"

class UWidgetComponent;
class UStaticMeshComponent;
class UGitsStationSubsystem;

UCLASS(Blueprintable)
class GHOSTINTHESTACK_API AGitsWallDisplay : public AActor
{
	GENERATED_BODY()

public:
	AGitsWallDisplay();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Body;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UWidgetComponent> Screen;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display")
	FString Title = TEXT("STATION LOG");

	/** Shown before any run. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display", meta = (MultiLine = true))
	FString IdleText = TEXT("no run recorded");

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
	void Refresh();
	void HandleStep(int32) { Refresh(); }
	void HandleRun() { Refresh(); }
	void HandleMessage(const FString&) { Refresh(); }
	UGitsStationSubsystem* Station() const;
	static FString DescribeEffect(const struct FGitsEffect& Effect);

	FDelegateHandle StepHandle, StartHandle, FinishHandle, MessageHandle;
};
