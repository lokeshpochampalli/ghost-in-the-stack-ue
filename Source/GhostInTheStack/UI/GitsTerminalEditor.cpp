#include "GitsTerminalEditor.h"
#include "GitsScreen.h"
#include "Station/GitsTerminal.h"
#include "Station/GitsScript.h"
#include "Station/GitsStationSubsystem.h"
#include "GhostInTheStackPlayerController.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Styling/CoreStyle.h"
#include "Framework/Application/SlateApplication.h"

void SGitsTerminalEditor::Construct(const FArguments& InArgs)
{
	Terminal = InArgs._Terminal;
	Controller = InArgs._Controller;
	const int32 FontSize = 20;
	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(FLinearColor(0.f, 0.f, 0.f, 0.55f))
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(980.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SAssignNew(Screen, SGitsScreen).FontSize(FontSize)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 6, 0, 0)
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
					.BorderBackgroundColor(GitsStyle::ScreenBack)
					.Padding(FMargin(16, 8))
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 10, 0)
						[
							SNew(STextBlock).Font(GitsStyle::Mono(FontSize)).ColorAndOpacity(GitsStyle::Copper).Text(FText::FromString(TEXT("edit >")))
						]
						+ SHorizontalBox::Slot().FillWidth(1.f)
						[
							SAssignNew(EditBox, SEditableTextBox)
							.Font(GitsStyle::Mono(FontSize))
							.ForegroundColor(GitsStyle::Phosphor)
							.BackgroundColor(GitsStyle::Ink)
							.OnTextCommitted_Lambda([this](const FText&, ETextCommit::Type Kind)
							{
								CommitEdit();
								if (Kind == ETextCommit::OnEnter) { RunScript(); }
							})
						]
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 6, 0, 0)
				[
					SAssignNew(Hint, STextBlock)
					.Font(GitsStyle::Mono(16))
					.ColorAndOpacity(GitsStyle::Bone)
					.Text(FText::FromString(TEXT("up/down: pick a line   type: change it   enter: run   esc: step away")))
				]
			]
		]
	];
	if (AGitsTerminal* T = Terminal.Get())
	{
		const int32 First = NextEditableLine(0, +1);
		Selected = First;
		T->SetSelectedLine(Selected);
	}
	Refresh();
}

int32 SGitsTerminalEditor::NextEditableLine(int32 From, int32 Direction) const
{
	const AGitsTerminal* T = Terminal.Get();
	if (!T) { return 0; }
	const int32 N = T->GetLines().Num();
	for (int32 L = From + Direction; L >= 1 && L <= N; L += Direction)
	{
		if (T->IsLineEditable(L)) { return L; }
	}
	return From;
}

void SGitsTerminalEditor::Refresh()
{
	AGitsTerminal* T = Terminal.Get();
	if (!T || !Screen.IsValid()) { return; }
	Screen->SetModel(T->BuildModel(true));
	if (EditBox.IsValid() && Selected >= 1 && Selected <= T->GetLines().Num())
	{
		if (EditBox->GetText().ToString() != T->GetLines()[Selected - 1] && !EditBox->HasKeyboardFocus())
		{
			EditBox->SetText(FText::FromString(T->GetLines()[Selected - 1]));
		}
	}
}

void SGitsTerminalEditor::SelectLine(int32 LineNumber)
{
	AGitsTerminal* T = Terminal.Get();
	if (!T) { return; }
	CommitEdit();
	Selected = FMath::Clamp(LineNumber, 1, FMath::Max(1, T->GetLines().Num()));
	T->SetSelectedLine(Selected);
	if (EditBox.IsValid())
	{
		EditBox->SetText(FText::FromString(T->GetLines().IsValidIndex(Selected - 1) ? T->GetLines()[Selected - 1] : TEXT("")));
		FSlateApplication::Get().SetKeyboardFocus(EditBox, EFocusCause::SetDirectly);
	}
	Refresh();
}

void SGitsTerminalEditor::CommitEdit()
{
	AGitsTerminal* T = Terminal.Get();
	if (!T || !EditBox.IsValid() || Selected < 1) { return; }
	const FString Text = EditBox->GetText().ToString();
	if (T->GetLines().IsValidIndex(Selected - 1) && T->GetLines()[Selected - 1] != Text)
	{
		T->SetLine(Selected, Text);
	}
}

void SGitsTerminalEditor::RunScript()
{
	AGitsTerminal* T = Terminal.Get();
	if (!T) { return; }
	CommitEdit();
	const FGitsRunSummary Summary = T->RunCurrent();
	Refresh();
	// A run that started is watched in the world, not through the overlay. Closing releases
	// the controller's reference, so hold one until this call returns.
	if (Summary.bRan)
	{
		TSharedRef<SGitsTerminalEditor> KeepAlive = SharedThis(this);
		if (AGhostInTheStackPlayerController* PC = Controller.Get()) { PC->CloseTerminal(); }
	}
}

FReply SGitsTerminalEditor::OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& InFocusEvent)
{
	if (EditBox.IsValid()) { return FReply::Handled().SetUserFocus(EditBox.ToSharedRef(), EFocusCause::SetDirectly); }
	return FReply::Handled();
}

FReply SGitsTerminalEditor::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Escape)
	{
		CommitEdit();
		if (AGhostInTheStackPlayerController* PC = Controller.Get()) { PC->CloseTerminal(); }
		return FReply::Handled();
	}
	if (Key == EKeys::Up) { SelectLine(NextEditableLine(Selected, -1)); return FReply::Handled(); }
	if (Key == EKeys::Down) { SelectLine(NextEditableLine(Selected, +1)); return FReply::Handled(); }
	return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
}
