#include "GitsTerminalEditor.h"
#include "GitsScreen.h"
#include "Station/GitsTerminal.h"
#include "Station/GitsScript.h"
#include "Station/GitsStationSubsystem.h"
#include "Companion/GitsVant.h"
#include "GhostInTheStackPlayerController.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Styling/CoreStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Engine/World.h"

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
				// VANT's question, shown while a prediction is pending.
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 6, 0, 0)
				[
					SAssignNew(Question, SGitsScreen).FontSize(FontSize)
				]
				// The edit row, shown otherwise.
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 6, 0, 0)
				[
					SAssignNew(EditRow, SBorder)
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
							.OnKeyDownHandler_Lambda([this](const FGeometry& G, const FKeyEvent& E) -> FReply
							{
								const FKey K = E.GetKey();
								if (K == EKeys::Insert || K == EKeys::Delete || K == EKeys::Up || K == EKeys::Down || K == EKeys::F1 || K == EKeys::Escape)
								{
									return OnKeyDown(G, E);
								}
								return FReply::Unhandled();
							})
						]
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 0)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 12, 0)
					[
						SNew(STextBlock).Font(GitsStyle::Mono(16)).ColorAndOpacity(GitsStyle::Copper).Text(FText::FromString(TEXT("VANT")))
					]
					+ SHorizontalBox::Slot().FillWidth(1.f)
					[
						SAssignNew(VantText, STextBlock).Font(GitsStyle::Mono(16)).ColorAndOpacity(GitsStyle::Bone).AutoWrapText(true)
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 6, 0, 0)
				[
					SAssignNew(Hint, STextBlock)
					.Font(GitsStyle::Mono(16))
					.ColorAndOpacity(GitsStyle::Bone)
				]
			]
		]
	];
	if (UGitsVantSubsystem* V = Vant())
	{
		SpeakHandle = V->OnSpeak.AddSP(this, &SGitsTerminalEditor::HandleVantLine);
		VantText->SetText(FText::FromString(V->GetLastLine()));
	}
	if (AGitsTerminal* T = Terminal.Get())
	{
		const int32 First = NextEditableLine(0, +1);
		Selected = First;
		T->SetSelectedLine(Selected);
	}
	Refresh();
}

SGitsTerminalEditor::~SGitsTerminalEditor()
{
	if (UGitsVantSubsystem* V = Vant()) { V->OnSpeak.Remove(SpeakHandle); }
}

UGitsVantSubsystem* SGitsTerminalEditor::Vant() const
{
	const AGitsTerminal* T = Terminal.Get();
	return T && T->GetWorld() ? T->GetWorld()->GetSubsystem<UGitsVantSubsystem>() : nullptr;
}

void SGitsTerminalEditor::HandleVantLine(const FString& Line)
{
	if (VantText.IsValid()) { VantText->SetText(FText::FromString(Line)); }
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

// --- asking ---------------------------------------------------------------------------------

void SGitsTerminalEditor::RefreshQuestion()
{
	AGitsTerminal* T = Terminal.Get();
	UGitsVantSubsystem* V = Vant();
	const bool bWasAsking = bAsking;
	bAsking = false;
	if (T && V && T->Script)
	{
		const TArray<FString> Pending = V->Pending(T->Script);
		if (Pending.Num() > 0)
		{
			bAsking = true;
			if (!bWasAsking || AskingId != Pending[0])
			{
				AskingId = Pending[0];
				uint32 Seed = 0;
				ShownOptions = V->Show(T->Script, AskingId, Seed);
				OptionIndex = 0;
				if (ShownOptions.Num() > 0) { V->Select(T->Script, AskingId, ShownOptions[0].Id); }
			}
		}
	}
	if (Question.IsValid()) { Question->SetVisibility(bAsking ? EVisibility::Visible : EVisibility::Collapsed); }
	if (EditRow.IsValid()) { EditRow->SetVisibility(bAsking ? EVisibility::Collapsed : EVisibility::Visible); }
	if (bAsking && Question.IsValid() && T && T->Script)
	{
		const FGitsPrediction* P = T->Script->FindPrediction(AskingId);
		FGitsScreenModel M;
		M.Title = TEXT("VANT asks");
		if (P)
		{
			M.MessageLines.Add(TEXT("VANT: ") + P->Prompt);
			M.MessageLines.Add(TEXT(""));
			for (int32 i = 0; i < ShownOptions.Num(); ++i)
			{
				M.MessageLines.Add(FString::Printf(TEXT("%s %d   %s"), i == OptionIndex ? TEXT(">") : TEXT(" "), i + 1, *ShownOptions[i].Label));
			}
		}
		M.Status = TEXT("committing is free. it reveals nothing; the run does.");
		Question->SetModel(M);
	}
	if (Hint.IsValid())
	{
		bool bDiscounted = false;
		const int32 Cost = T ? T->RunCostNow(bDiscounted) : 0;
		const bool bFree = T && T->IsFreeEdit();
		Hint->SetText(FText::FromString(bAsking
			? TEXT("up/down: choose   enter: commit (free)   f1: one of ilse's notes   esc: step away")
			: FString::Printf(TEXT("up/down: pick a line   type: change it%s   enter: run, draws %d%s   f1: a note   esc: step away"),
				bFree ? TEXT("   ins: new line   del: drop line") : TEXT(""), Cost, bDiscounted ? TEXT(" (predicted)") : TEXT(""))));
	}
	if (bWasAsking && !bAsking && EditBox.IsValid())
	{
		FSlateApplication::Get().SetKeyboardFocus(EditBox, EFocusCause::SetDirectly);
	}
	else if (bAsking && !bWasAsking)
	{
		FSlateApplication::Get().SetKeyboardFocus(AsShared(), EFocusCause::SetDirectly);
	}
}

void SGitsTerminalEditor::MoveOption(int32 Direction)
{
	if (ShownOptions.Num() == 0) { return; }
	OptionIndex = (OptionIndex + Direction + ShownOptions.Num()) % ShownOptions.Num();
	AGitsTerminal* T = Terminal.Get();
	if (UGitsVantSubsystem* V = Vant()) { if (T) { V->Select(T->Script, AskingId, ShownOptions[OptionIndex].Id); } }
	RefreshQuestion();
}

void SGitsTerminalEditor::CommitOption()
{
	AGitsTerminal* T = Terminal.Get();
	UGitsVantSubsystem* V = Vant();
	if (!T || !V || !bAsking || !ShownOptions.IsValidIndex(OptionIndex)) { return; }
	V->Select(T->Script, AskingId, ShownOptions[OptionIndex].Id);
	V->Commit(T->Script, AskingId, T->GetSourceText());
	Refresh();
}

bool SGitsTerminalEditor::ChooseOption(int32 Index)
{
	if (!bAsking || !ShownOptions.IsValidIndex(Index - 1)) { return false; }
	OptionIndex = Index - 1;
	CommitOption();
	return true;
}

void SGitsTerminalEditor::RequestHint()
{
	AGitsTerminal* T = Terminal.Get();
	if (UGitsVantSubsystem* V = Vant()) { if (T) { V->RevealNextHint(T->Script); } }
}

// --- editing --------------------------------------------------------------------------------

void SGitsTerminalEditor::Refresh()
{
	AGitsTerminal* T = Terminal.Get();
	if (!T || !Screen.IsValid()) { return; }
	RefreshQuestion();
	Screen->SetModel(T->BuildModel(true));
	if (!bAsking && EditBox.IsValid() && Selected >= 1 && Selected <= T->GetLines().Num())
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
		if (!bAsking) { FSlateApplication::Get().SetKeyboardFocus(EditBox, EFocusCause::SetDirectly); }
	}
	Refresh();
}

void SGitsTerminalEditor::CommitEdit()
{
	AGitsTerminal* T = Terminal.Get();
	if (!T || !EditBox.IsValid() || Selected < 1 || bAsking) { return; }
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
	if (!bAsking && EditBox.IsValid()) { return FReply::Handled().SetUserFocus(EditBox.ToSharedRef(), EFocusCause::SetDirectly); }
	return FReply::Handled();
}

FReply SGitsTerminalEditor::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	const bool bUp = Key == EKeys::Up || Key == EKeys::Gamepad_DPad_Up || Key == EKeys::Gamepad_LeftStick_Up;
	const bool bDown = Key == EKeys::Down || Key == EKeys::Gamepad_DPad_Down || Key == EKeys::Gamepad_LeftStick_Down;
	const bool bAccept = Key == EKeys::Enter || Key == EKeys::Gamepad_FaceButton_Bottom;
	const bool bBack = Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right;
	const bool bHint = Key == EKeys::F1 || Key == EKeys::Gamepad_FaceButton_Top;
	if (bBack)
	{
		CommitEdit();
		if (AGhostInTheStackPlayerController* PC = Controller.Get()) { PC->CloseTerminal(); }
		return FReply::Handled();
	}
	if (bHint) { RequestHint(); return FReply::Handled(); }
	if (bAsking)
	{
		if (bUp) { MoveOption(-1); return FReply::Handled(); }
		if (bDown) { MoveOption(+1); return FReply::Handled(); }
		if (bAccept) { CommitOption(); return FReply::Handled(); }
		return FReply::Handled();
	}
	if (bUp) { SelectLine(NextEditableLine(Selected, -1)); return FReply::Handled(); }
	if (bDown) { SelectLine(NextEditableLine(Selected, +1)); return FReply::Handled(); }
	if (Key == EKeys::Gamepad_FaceButton_Bottom) { RunScript(); return FReply::Handled(); }
	if (Key == EKeys::Insert || Key == EKeys::Gamepad_DPad_Right)
	{
		if (AGitsTerminal* T = Terminal.Get())
		{
			CommitEdit();
			const int32 NewLine = T->InsertLineAfter(Selected);
			if (NewLine > 0) { SelectLine(NewLine); }
		}
		return FReply::Handled();
	}
	if (Key == EKeys::Delete || Key == EKeys::Gamepad_DPad_Left)
	{
		if (AGitsTerminal* T = Terminal.Get())
		{
			if (T->IsFreeEdit() && EditBox.IsValid() && EditBox->GetText().IsEmpty() && T->RemoveLine(Selected)) { SelectLine(FMath::Max(1, Selected - 1)); }
		}
		return FReply::Handled();
	}
	return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
}
