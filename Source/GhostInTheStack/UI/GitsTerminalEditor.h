// Ghost in the Stack — the terminal overlay: VANT asks, then the player reads, edits one line, runs.
//
// Keyboard and gamepad only, like the terminal it stands for. While a prediction is pending the
// overlay is VANT's question: up and down pick an answer, Enter commits (free). Otherwise it is
// the editor: up and down pick a line, typing edits it, Enter runs. F1 asks for one of Ilse's
// notes. Escape steps away. Only the lines the script marks editable can change.
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Station/GitsScript.h"

class AGitsTerminal;
class SGitsScreen;
class SEditableTextBox;
class STextBlock;
class SWidget;
class AGhostInTheStackPlayerController;
class UGitsVantSubsystem;

class SGitsTerminalEditor : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGitsTerminalEditor) {}
		SLATE_ARGUMENT(TWeakObjectPtr<AGitsTerminal>, Terminal)
		SLATE_ARGUMENT(TWeakObjectPtr<AGhostInTheStackPlayerController>, Controller)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SGitsTerminalEditor();
	/** Re-reads the terminal and VANT and redraws; switches between asking and editing. */
	void Refresh();
	/** Moves the selection to a line and puts it in the edit box. */
	void SelectLine(int32 LineNumber);
	void CommitEdit();
	void RunScript();

	// --- the question
	bool IsAsking() const { return bAsking; }
	/** Picks the Nth shown option (1-based) and commits it. For the console. */
	bool ChooseOption(int32 Index);
	void RequestHint();

	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& InFocusEvent) override;

private:
	int32 NextEditableLine(int32 From, int32 Direction) const;
	UGitsVantSubsystem* Vant() const;
	void RefreshQuestion();
	void MoveOption(int32 Direction);
	void CommitOption();
	void HandleVantLine(const FString& Line);

	TWeakObjectPtr<AGitsTerminal> Terminal;
	TWeakObjectPtr<AGhostInTheStackPlayerController> Controller;
	TSharedPtr<SGitsScreen> Screen;
	TSharedPtr<SGitsScreen> Question;
	TSharedPtr<SWidget> EditRow;
	TSharedPtr<SEditableTextBox> EditBox;
	TSharedPtr<STextBlock> VantText;
	TSharedPtr<STextBlock> Hint;
	int32 Selected = 0;

	bool bAsking = false;
	FString AskingId;
	TArray<FGitsPredictionOption> ShownOptions;
	int32 OptionIndex = 0;
	FDelegateHandle SpeakHandle;
};
