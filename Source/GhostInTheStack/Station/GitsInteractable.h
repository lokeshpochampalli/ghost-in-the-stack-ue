// Ghost in the Stack — things the player can use by looking at them and pressing Interact.
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GitsInteractable.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UGitsInteractable : public UInterface
{
	GENERATED_BODY()
};

class GHOSTINTHESTACK_API IGitsInteractable
{
	GENERATED_BODY()

public:
	/** The player used this. */
	UFUNCTION(BlueprintNativeEvent, Category = "Station")
	void Interact(APawn* Player);

	/** Short prompt shown when the player looks at it, e.g. "use terminal". */
	UFUNCTION(BlueprintNativeEvent, Category = "Station")
	FText GetInteractPrompt() const;
};
