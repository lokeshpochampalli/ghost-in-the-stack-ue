// Copyright Epic Games, Inc. All Rights Reserved.


#include "GhostInTheStackPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "GhostInTheStackCameraManager.h"
#include "Blueprint/UserWidget.h"
#include "GhostInTheStack.h"
#include "Widgets/Input/SVirtualJoystick.h"

AGhostInTheStackPlayerController::AGhostInTheStackPlayerController()
{
	// set the player camera manager class
	PlayerCameraManagerClass = AGhostInTheStackCameraManager::StaticClass();
}

void AGhostInTheStackPlayerController::BeginPlay()
{
	Super::BeginPlay();

	
	// only spawn touch controls on local player controllers
	if (IsLocalPlayerController() && ShouldUseTouchControls())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

		} else {

			UE_LOG(LogGhostInTheStack, Error, TEXT("Could not spawn mobile controls widget."));

		}

	}
}

void AGhostInTheStackPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Context
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}
	}
	
}

bool AGhostInTheStackPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}

// --- terminals ------------------------------------------------------------------------------

#include "Station/GitsTerminal.h"
#include "Station/GitsStationSubsystem.h"
#include "UI/GitsTerminalEditor.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Engine.h"
#include "Camera/PlayerCameraManager.h"
#include "EngineUtils.h"
#include "Framework/Application/SlateApplication.h"

void AGhostInTheStackPlayerController::UseTerminal(AGitsTerminal* Terminal)
{
	if (!Terminal || !IsLocalPlayerController()) { return; }
	if (CurrentTerminal) { CloseTerminal(); }
	CurrentTerminal = Terminal;
	Terminal->SetInUse(true);
	Overlay = SNew(SGitsTerminalEditor).Terminal(Terminal).Controller(this);
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->AddViewportWidgetContent(Overlay.ToSharedRef(), 100);
	}
	FInputModeUIOnly Mode;
	Mode.SetWidgetToFocus(Overlay);
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
	SetInputMode(Mode);
	SetShowMouseCursor(false);
	if (APawn* P = GetPawn()) { P->DisableInput(this); }
	Overlay->SelectLine(Overlay.IsValid() ? Terminal->GetSelectedLine() : 1);
}

void AGhostInTheStackPlayerController::CloseTerminal()
{
	if (Overlay.IsValid())
	{
		if (GEngine && GEngine->GameViewport) { GEngine->GameViewport->RemoveViewportWidgetContent(Overlay.ToSharedRef()); }
		Overlay.Reset();
	}
	if (CurrentTerminal) { CurrentTerminal->SetInUse(false); }
	CurrentTerminal = nullptr;
	SetInputMode(FInputModeGameOnly());
	SetShowMouseCursor(false);
	if (APawn* P = GetPawn()) { P->EnableInput(this); }
}

void AGhostInTheStackPlayerController::EndPlay(const EEndPlayReason::Type Reason)
{
	CloseTerminal();
	Super::EndPlay(Reason);
}

AGitsTerminal* AGhostInTheStackPlayerController::FindTerminalNearby(float MaxDistance) const
{
	const APawn* P = GetPawn();
	if (!P) { return nullptr; }
	// Prefer what the camera is pointing at.
	if (PlayerCameraManager)
	{
		const FVector Start = PlayerCameraManager->GetCameraLocation();
		const FVector End = Start + PlayerCameraManager->GetCameraRotation().Vector() * MaxDistance;
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(GitsTerminal), false, P);
		if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
		{
			if (AGitsTerminal* T = Cast<AGitsTerminal>(Hit.GetActor())) { return T; }
		}
	}
	AGitsTerminal* Best = nullptr;
	float BestDist = MaxDistance;
	for (TActorIterator<AGitsTerminal> It(GetWorld()); It; ++It)
	{
		const float D = FVector::Dist(It->GetActorLocation(), P->GetActorLocation());
		if (D < BestDist) { BestDist = D; Best = *It; }
	}
	return Best;
}

AGitsTerminal* AGhostInTheStackPlayerController::TerminalForCommands() const
{
	return CurrentTerminal ? CurrentTerminal.Get() : FindTerminalNearby(600.f);
}

void AGhostInTheStackPlayerController::GitsUse()
{
	if (AGitsTerminal* T = FindTerminalNearby(600.f)) { UseTerminal(T); }
	else { UE_LOG(LogGhostInTheStack, Warning, TEXT("GitsUse: no terminal within reach")); }
}

void AGhostInTheStackPlayerController::GitsRun()
{
	if (AGitsTerminal* T = TerminalForCommands())
	{
		const FGitsRunSummary S = T->RunCurrent();
		UE_LOG(LogGhostInTheStack, Display, TEXT("GitsRun: ran=%d refused=%d parseFailed=%d outcome=%s message=%s"), S.bRan, S.bRefused, S.bParseFailed, *S.Outcome, *S.Message);
		if (Overlay.IsValid()) { Overlay->Refresh(); }
	}
	else { UE_LOG(LogGhostInTheStack, Warning, TEXT("GitsRun: no terminal within reach")); }
}

void AGhostInTheStackPlayerController::GitsSetLine(int32 Line, const FString& Text)
{
	if (AGitsTerminal* T = TerminalForCommands())
	{
		const bool bOk = T->SetLine(Line, Text);
		UE_LOG(LogGhostInTheStack, Display, TEXT("GitsSetLine %d: %s -> %s"), Line, *Text, bOk ? TEXT("ok") : TEXT("refused (not editable)"));
		if (Overlay.IsValid()) { Overlay->Refresh(); }
	}
}

void AGhostInTheStackPlayerController::GitsReset()
{
	if (AGitsTerminal* T = TerminalForCommands()) { T->ResetToScript(); if (Overlay.IsValid()) { Overlay->Refresh(); } }
}

void AGhostInTheStackPlayerController::GitsClose()
{
	CloseTerminal();
}

void AGhostInTheStackPlayerController::GitsRewind()
{
	if (UGitsStationSubsystem* S = GetWorld()->GetSubsystem<UGitsStationSubsystem>()) { UE_LOG(LogGhostInTheStack, Display, TEXT("GitsRewind: %d"), S->BeginRewind()); }
}

void AGhostInTheStackPlayerController::GitsResume()
{
	if (UGitsStationSubsystem* S = GetWorld()->GetSubsystem<UGitsStationSubsystem>()) { S->EndRewind(); UE_LOG(LogGhostInTheStack, Display, TEXT("GitsResume: state=%d"), (int32)S->GetPlayState()); }
}

void AGhostInTheStackPlayerController::GitsVerifyRewind()
{
	if (UGitsStationSubsystem* S = GetWorld()->GetSubsystem<UGitsStationSubsystem>())
	{
		FString Report;
		const bool bOk = S->VerifyRewind(Report);
		UE_LOG(LogGhostInTheStack, Display, TEXT("GitsVerifyRewind: ok=%d %s"), bOk, *Report);
	}
}

void AGhostInTheStackPlayerController::GitsStatus()
{
	if (UGitsStationSubsystem* S = GetWorld()->GetSubsystem<UGitsStationSubsystem>())
	{
		const FGitsRunSummary& Sum = S->GetLastSummary();
		UE_LOG(LogGhostInTheStack, Display, TEXT("GitsStatus: ran=%d refused=%d outcome=%s steps=%d statements=%d state=%d head=%d clock=%.2f output=[%s] minFps=%.1f avgFps=%.1f frames=%d worstFrame=%d maxSeekMs=%.2f message=%s"),
			Sum.bRan, Sum.bRefused, *Sum.Outcome, Sum.Steps, Sum.Statements, (int32)S->GetPlayState(), S->GetPlayIndex(), S->GetPlayClock(), *FString::Join(Sum.Output, TEXT(" | ")),
			S->PlaybackMinFps, S->PlaybackAvgFps, S->PlaybackFrames, S->PlaybackWorstFrame, S->MaxSeekMs, *Sum.Message);
		for (const auto& P : S->GetCurrentWorld()) { UE_LOG(LogGhostInTheStack, Display, TEXT("  world %s=%s"), *P.Key, *P.Value.ToText()); }
	}
}
