// Ghost in the Stack — the study terminal: consent and the instruments, at the start and the end.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GitsInteractable.h"
#include "GitsInstrumentTerminal.generated.h"

class UStaticMeshComponent;
class UWidgetComponent;

UCLASS(Blueprintable)
class GHOSTINTHESTACK_API AGitsInstrumentTerminal : public AActor, public IGitsInteractable
{
	GENERATED_BODY()

public:
	AGitsInstrumentTerminal();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Body;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UWidgetComponent> Screen;

	/** "pre": consent and the pre-test. "post": the post-test and the questionnaires, then the export. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Study")
	FString Occasion = TEXT("pre");

	UFUNCTION(BlueprintCallable, Category = "Study")
	void Refresh();

	// IGitsInteractable
	virtual void Interact_Implementation(APawn* Player) override;
	virtual FText GetInteractPrompt_Implementation() const override;

protected:
	virtual void BeginPlay() override;
};
