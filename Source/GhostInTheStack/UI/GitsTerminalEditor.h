// Ghost in the Stack — the terminal overlay: read the script, edit one line, run it.
//
// Keyboard only, like the terminal it stands for: up and down pick a line, typing edits
// it, Enter runs, Escape steps away. Only the lines the script marks editable can change.
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class AGitsTerminal;
class SGitsScreen;
class SEditableTextBox;
class STextBlock;
class AGhostInTheStackPlayerController;

class SGitsTerminalEditor : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGitsTerminalEditor) {}
		SLATE_ARGUMENT(TWeakObjectPtr<AGitsTerminal>, Terminal)
		SLATE_ARGUMENT(TWeakObjectPtr<AGhostInTheStackPlayerController>, Controller)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	/** Re-reads the terminal and redraws. */
	void Refresh();
	/** Moves the selection to a line and puts it in the edit box. */
	void SelectLine(int32 LineNumber);
	void CommitEdit();
	void RunScript();

	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& InFocusEvent) override;

private:
	int32 NextEditableLine(int32 From, int32 Direction) const;

	TWeakObjectPtr<AGitsTerminal> Terminal;
	TWeakObjectPtr<AGhostInTheStackPlayerController> Controller;
	TSharedPtr<SGitsScreen> Screen;
	TSharedPtr<SEditableTextBox> EditBox;
	TSharedPtr<STextBlock> Hint;
	int32 Selected = 0;
};
