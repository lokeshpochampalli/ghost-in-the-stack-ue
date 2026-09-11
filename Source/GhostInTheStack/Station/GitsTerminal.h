// Ghost in the Stack — a diegetic terminal: Ilse's script on a screen in the world.
//
// The screen shows the script with the executing line highlighted while a run plays.
// Using the terminal opens an overlay where one line at a time can be edited and the
// script run. The terminal never decides anything: it hands source to the station and
// reflects what the station reports.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GitsInteractable.h"
#include "UI/GitsScreen.h"
#include "GitsTerminal.generated.h"

class UGitsScript;
class UWidgetComponent;
class UStaticMeshComponent;
class UGitsStationSubsystem;
struct FGitsRunSummary;

UCLASS(Blueprintable)
class GHOSTINTHESTACK_API AGitsTerminal : public AActor, public IGitsInteractable
{
	GENERATED_BODY()

public:
	AGitsTerminal();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Body;

	/** The world-space screen on the terminal's head. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UWidgetComponent> Screen;

	/** The script this terminal holds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terminal")
	TObjectPtr<UGitsScript> Script;

	/** Shown while nobody is using it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terminal")
	FString IdlePrompt = TEXT("press E to use");

	// --- the source as the player has it
	UFUNCTION(BlueprintPure, Category = "Terminal") const TArray<FString>& GetLines() const { return Lines; }
	UFUNCTION(BlueprintPure, Category = "Terminal") FString GetSourceText() const;
	UFUNCTION(BlueprintCallable, Category = "Terminal") bool SetLine(int32 LineNumber, const FString& Text);
	UFUNCTION(BlueprintPure, Category = "Terminal") bool IsLineEditable(int32 LineNumber) const;
	UFUNCTION(BlueprintCallable, Category = "Terminal") void ResetToScript();
	/** Make scripts only: a new empty line after LineNumber (0 = at the top). Returns the new line's number, 0 if refused. */
	UFUNCTION(BlueprintCallable, Category = "Terminal") int32 InsertLineAfter(int32 LineNumber);
	UFUNCTION(BlueprintCallable, Category = "Terminal") bool RemoveLine(int32 LineNumber);
	UFUNCTION(BlueprintPure, Category = "Terminal") bool IsFreeEdit() const;

	/** Runs the current source through the station, if VANT allows it and the bus can pay. */
	UFUNCTION(BlueprintCallable, Category = "Terminal")
	FGitsRunSummary RunCurrent();
	/** What the next run would draw: the discounted price once a reading is committed or confirmed. */
	UFUNCTION(BlueprintPure, Category = "Terminal")
	int32 RunCostNow(bool& bDiscounted) const;

	/** The overlay's selection, mirrored on the screen. */
	UFUNCTION(BlueprintCallable, Category = "Terminal") void SetSelectedLine(int32 LineNumber);
	int32 GetSelectedLine() const { return SelectedLine; }
	void SetInUse(bool bInUse);

	/** The model both the world screen and the overlay draw. */
	FGitsScreenModel BuildModel(bool bForOverlay) const;

	// IGitsInteractable
	virtual void Interact_Implementation(APawn* Player) override;
	virtual FText GetInteractPrompt_Implementation() const override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void OnConstruction(const FTransform& Transform) override;

private:
	void RefreshScreen();
	void HandleStep(int32 StepIndex);
	void HandleRunStarted();
	void HandleRunFinished();
	void HandleMessage(const FString& Message);
	UGitsStationSubsystem* Station() const;

	TArray<FString> Lines;
	int32 SelectedLine = 0;
	int32 HighlightLine = 0;
	bool bInUse = false;
	FString Status;
	bool bStatusIsError = false;
	bool bThisTerminalRan = false;
	FDelegateHandle StepHandle, StartHandle, FinishHandle, MessageHandle;
};
