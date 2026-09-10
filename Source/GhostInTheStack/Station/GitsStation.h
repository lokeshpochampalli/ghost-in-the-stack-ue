// Ghost in the Stack — per-level station configuration.
//
// One per level. Declares the world the scripts start from (sensor readings) and how
// reading builtins answer. The oracle is deterministic: a sensor reads its declared value
// plus any drift per tick of wait().
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interpreter/GitsTypes.h"
#include "GitsStation.generated.h"

USTRUCT(BlueprintType)
struct FGitsSensorSpec
{
	GENERATED_BODY()

	/** The id read_sensor() names. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Station") FString Id;
	/** Reading before any wait(). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Station") float Value = 0.f;
	/** Change per tick of wait(). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Station") float DriftPerTick = 0.f;
};

UCLASS(Blueprintable)
class GHOSTINTHESTACK_API AGitsStation : public AActor
{
	GENERATED_BODY()

public:
	AGitsStation();

	/** Sensors this level's instruments can read. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Station")
	TArray<FGitsSensorSpec> Sensors;

	/** Keys the world starts with, e.g. door.inner=false so displays can show state before a run. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Station")
	TMap<FString, bool> InitialSwitches;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Station")
	TMap<FString, float> InitialLevels;

	/** VANT's refusal when nothing changed since the last run. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Station", meta = (MultiLine = true))
	FString UnchangedSourceMessage = TEXT("Nothing has changed, so nothing new will happen. Change a line, then run it again.");

	FGitsWorldState InitialWorld() const;
	/** Answers read_sensor(id) from the folded world: declared value plus drift times the clock. */
	FGitsValue Read(const FGitsWorldState& World, const FGitsQuery& Query) const;
};
