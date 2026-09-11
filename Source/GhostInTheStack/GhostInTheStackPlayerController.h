// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GhostInTheStackPlayerController.generated.h"

class UInputMappingContext;
class UUserWidget;
class AGitsTerminal;
class SGitsTerminalEditor;
class SGitsVantCaption;

/**
 *  Simple first person Player Controller
 *  Manages the input mapping context.
 *  Overrides the Player Camera Manager class.
 *  Owns the terminal overlay: using a terminal takes the keyboard, stepping away gives it back.
 */
UCLASS(abstract, config="Game")
class GHOSTINTHESTACK_API AGhostInTheStackPlayerController : public APlayerController
{
	GENERATED_BODY()

public:

	/** Constructor */
	AGhostInTheStackPlayerController();

	// --- terminals

	/** Opens the editing overlay for a terminal and hands it the keyboard. */
	UFUNCTION(BlueprintCallable, Category = "Station")
	void UseTerminal(AGitsTerminal* Terminal);

	/** Closes the overlay and returns to first-person control. */
	UFUNCTION(BlueprintCallable, Category = "Station")
	void CloseTerminal();

	UFUNCTION(BlueprintPure, Category = "Station")
	AGitsTerminal* GetCurrentTerminal() const { return CurrentTerminal; }

	/** The terminal the player is looking at or standing next to, else null. */
	UFUNCTION(BlueprintPure, Category = "Station")
	AGitsTerminal* FindTerminalNearby(float MaxDistance = 400.f) const;

	// --- console commands, for testing the station without touching the keyboard
	/** Uses the nearest terminal. */
	UFUNCTION(Exec) void GitsUse();
	/** Runs the current or nearest terminal's script. */
	UFUNCTION(Exec) void GitsRun();
	/** Replaces one line of the current or nearest terminal's script. */
	UFUNCTION(Exec) void GitsSetLine(int32 Line, const FString& Text);
	/** Restores the script as Ilse wrote it. */
	UFUNCTION(Exec) void GitsReset();
	/** Logs the last run's outcome, output and playback frame rate. */
	UFUNCTION(Exec) void GitsStatus();
	/** Closes the terminal overlay. */
	UFUNCTION(Exec) void GitsClose();
	/** Starts rewinding the last run (as if the rewind key were held). */
	UFUNCTION(Exec) void GitsRewind();
	/** Releases the rewind: playback resumes forward. */
	UFUNCTION(Exec) void GitsResume();
	/** Seeks backwards through every statement, checking world and line at each, and logs the timing. */
	UFUNCTION(Exec) void GitsVerifyRewind();
	/** Answers VANT's question with the Nth shown option (1-based) and commits it. */
	UFUNCTION(Exec) void GitsPredict(int32 Index);
	/** Asks for the next of Ilse's notes. */
	UFUNCTION(Exec) void GitsHint();
	/** Logs where every prediction of the current or nearest terminal's script stands. */
	UFUNCTION(Exec) void GitsVantStatus();

protected:

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	/** Mobile controls widget to spawn */
	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	/** Pointer to the mobile controls widget */
	UPROPERTY()
	TObjectPtr<UUserWidget> MobileControlsWidget;

	/** VANT's caption over the viewport. */
	TSharedPtr<SGitsVantCaption> Caption;
	FDelegateHandle SpeakHandle;

	/** If true, the player will use UMG touch controls even if not playing on mobile platforms */
	UPROPERTY(EditAnywhere, Config, Category = "Input|Touch Controls")
	bool bForceTouchControls = false;

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;

	/** Returns true if the player should use UMG touch controls */
	bool ShouldUseTouchControls() const;

	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
	AGitsTerminal* TerminalForCommands() const;

	UPROPERTY()
	TObjectPtr<AGitsTerminal> CurrentTerminal;
	TSharedPtr<SGitsTerminalEditor> Overlay;
};
