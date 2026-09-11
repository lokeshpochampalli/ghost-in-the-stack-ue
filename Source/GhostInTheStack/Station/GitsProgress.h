// Ghost in the Stack — what the player has restored, carried across the sector maps.
//
// One map per sector. The world subsystems (station, VANT) start fresh with each map; this
// game-instance subsystem remembers which sectors are complete so a sector's exit stays open
// when the player comes back through it.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GitsProgress.generated.h"

UCLASS()
class GHOSTINTHESTACK_API UGitsProgressSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** Marks the sector of the given map (its package name) complete. */
	UFUNCTION(BlueprintCallable, Category = "Progress")
	void MarkSectorComplete(const FString& MapName) { CompletedSectors.Add(Normalise(MapName)); }

	UFUNCTION(BlueprintPure, Category = "Progress")
	bool IsSectorComplete(const FString& MapName) const { return CompletedSectors.Contains(Normalise(MapName)); }

	UFUNCTION(BlueprintPure, Category = "Progress")
	int32 CompletedCount() const { return CompletedSectors.Num(); }

	/** The map name the current world was loaded from, without the PIE prefix. */
	static FString MapNameOf(const UWorld* World);

private:
	static FString Normalise(const FString& MapName);
	TSet<FString> CompletedSectors;
};
