// Ghost in the Stack — the study terminal's overlay: consent, then the instruments, one item at a time.
//
// One generic panel for every instrument (ADR-018). Keyboard and gamepad: up and down choose,
// Enter answers, S skips, Escape declines at the consent screen and does nothing after it (a
// questionnaire is finished or skipped item by item, not abandoned by accident).
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Telemetry/GitsInstruments.h"

class SGitsScreen;
class STextBlock;
class AGhostInTheStackPlayerController;
class UGitsTelemetrySubsystem;

class SGitsInstrumentPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGitsInstrumentPanel) {}
		SLATE_ARGUMENT(TWeakObjectPtr<AGhostInTheStackPlayerController>, Controller)
		/** "pre" or "post". */
		SLATE_ARGUMENT(FString, Occasion)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Chooses the Nth shown option (1-based) and confirms it. For the console. */
	bool Answer(int32 Index);
	/** Skips the current item. */
	bool Skip();
	bool IsFinished() const { return bFinished; }
	FString Describe() const;

	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual bool SupportsKeyboardFocus() const override { return true; }

private:
	enum class EStep : uint8 { Consent, Experience, Instrument, Exported, Done };

	UGitsTelemetrySubsystem* Telemetry() const;
	void Refresh();
	void Move(int32 Direction);
	void Confirm();
	void StartInstrument(int32 Index);
	void AdvanceItem();
	void CompleteInstrument();
	void FinishSequence();
	int32 OptionCount() const;

	TWeakObjectPtr<AGhostInTheStackPlayerController> Controller;
	FString Occasion;
	TSharedPtr<SGitsScreen> Screen;
	TSharedPtr<STextBlock> Hint;

	EStep Step = EStep::Consent;
	int32 Selection = 0;
	bool bCodeCapture = false;
	TArray<const FGitsInstrument*> Sequence;
	int32 InstrumentIndex = -1;
	int32 ItemIndex = 0;
	TArray<FGitsQuizOption> ShownOptions;
	FGitsResponses Responses;
	double ItemShownAt = 0.0;
	FString ExportPath;
	FString ExportError;
	bool bFinished = false;
};
