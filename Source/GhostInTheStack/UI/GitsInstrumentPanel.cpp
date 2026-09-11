#include "GitsInstrumentPanel.h"
#include "GitsScreen.h"
#include "GhostInTheStackPlayerController.h"
#include "Telemetry/GitsTelemetry.h"
#include "Companion/GitsVant.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

void SGitsInstrumentPanel::Construct(const FArguments& InArgs)
{
	Controller = InArgs._Controller;
	Occasion = InArgs._Occasion;
	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(FLinearColor(0.f, 0.f, 0.f, 0.6f))
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(1000.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					// A fixed height: the screen fits its rows to what it is given, and a
					// height that followed the content would chase its own tail.
					SNew(SBox).HeightOverride(620.f)
					[
						SAssignNew(Screen, SGitsScreen).FontSize(20)
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 6, 0, 0)
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
					.BorderBackgroundColor(FLinearColor(0.f, 0.f, 0.f, 0.85f))
					.Padding(FMargin(10.f, 6.f))
					[
						SAssignNew(Hint, STextBlock).Font(GitsStyle::Mono(16)).ColorAndOpacity(GitsStyle::Bone)
					]
				]
			]
		]
	];
	UGitsTelemetrySubsystem* T = Telemetry();
	if (Occasion == TEXT("pre"))
	{
		if (T && T->HasConsent()) { Sequence.Add(&GitsInstruments::TracingTest(GitsInstruments::FormFor(T->GetParticipantCode(), TEXT("pre")))); Step = EStep::Instrument; StartInstrument(0); }
		else { Step = EStep::Consent; }
	}
	else
	{
		const FString Code = T ? T->GetParticipantCode() : FString();
		Sequence = { &GitsInstruments::TracingTest(GitsInstruments::FormFor(Code, TEXT("post"))), &GitsInstruments::MeegaPlus(), &GitsInstruments::Sus(), &GitsInstruments::Imi() };
		if (T && T->HasConsent()) { Step = EStep::Instrument; StartInstrument(0); }
		else { Step = EStep::Consent; }
	}
	Refresh();
}

UGitsTelemetrySubsystem* SGitsInstrumentPanel::Telemetry() const
{
	const AGhostInTheStackPlayerController* PC = Controller.Get();
	const UGameInstance* GI = PC && PC->GetWorld() ? PC->GetWorld()->GetGameInstance() : nullptr;
	return GI ? GI->GetSubsystem<UGitsTelemetrySubsystem>() : nullptr;
}

int32 SGitsInstrumentPanel::OptionCount() const
{
	switch (Step)
	{
	case EStep::Consent: return 3;
	case EStep::Experience: return 4;
	case EStep::Instrument:
	{
		const FGitsInstrument* I = Sequence.IsValidIndex(InstrumentIndex) ? Sequence[InstrumentIndex] : nullptr;
		if (!I) { return 0; }
		return I->Kind == FGitsInstrument::EKind::Quiz ? ShownOptions.Num() : I->Scale.Labels.Num();
	}
	default: return 1;
	}
}

void SGitsInstrumentPanel::Refresh()
{
	if (!Screen.IsValid()) { return; }
	UGitsTelemetrySubsystem* T = Telemetry();
	FGitsScreenModel M;
	FString HintText;
	switch (Step)
	{
	case EStep::Consent:
	{
		M.Title = TEXT("STUDY TERMINAL  consent");
		M.MessageLines.Add(TEXT("VANT: Before anything is recorded. Your participant code is:"));
		M.MessageLines.Add(TEXT(""));
		M.MessageLines.Add(FString::Printf(TEXT("        %s"), T ? *T->GetParticipantCode() : TEXT("?")));
		M.MessageLines.Add(TEXT(""));
		M.MessageLines.Add(TEXT("Write it down. It is the only thing that links you to what is recorded, and quoting it is how you have everything erased. No name, no email, nothing identifying is stored."));
		M.MessageLines.Add(TEXT(""));
		const TCHAR* Options[] = { TEXT("I consent: record what I do in the game, never the text I type"), TEXT("I consent, and the text of my edits may be kept as well"), TEXT("I do not consent") };
		for (int32 i = 0; i < 3; ++i) { M.MessageLines.Add(FString::Printf(TEXT("%s %d   %s"), i == Selection ? TEXT(">") : TEXT(" "), i + 1, Options[i])); }
		M.Status = FString::Printf(TEXT("consent sheet %s"), UGitsTelemetrySubsystem::ConsentVersion);
		HintText = TEXT("up/down: choose   enter: confirm   esc: do not consent and step away");
		break;
	}
	case EStep::Experience:
	{
		M.Title = TEXT("STUDY TERMINAL  one question");
		M.MessageLines.Add(TEXT("VANT: How much programming have you done before today?"));
		M.MessageLines.Add(TEXT(""));
		const TCHAR* Options[] = { TEXT("None"), TEXT("Some, on my own"), TEXT("Formal teaching, a course or a module"), TEXT("Prefer not to say") };
		for (int32 i = 0; i < 4; ++i) { M.MessageLines.Add(FString::Printf(TEXT("%s %d   %s"), i == Selection ? TEXT(">") : TEXT(" "), i + 1, Options[i])); }
		M.Status = TEXT("a band, never free text");
		HintText = TEXT("up/down: choose   enter: confirm");
		break;
	}
	case EStep::Instrument:
	{
		const FGitsInstrument* I = Sequence.IsValidIndex(InstrumentIndex) ? Sequence[InstrumentIndex] : nullptr;
		if (!I) { break; }
		M.Title = FString::Printf(TEXT("%s  %d of %d"), *I->Title, ItemIndex + 1, I->ItemCount());
		if (I->Kind == FGitsInstrument::EKind::Quiz)
		{
			const FGitsQuizItem& Item = I->QuizItems[ItemIndex];
			FString Src = Item.Source; Src.ParseIntoArray(M.CodeLines, TEXT("\n"), false);
			while (M.CodeLines.Num() && M.CodeLines.Last().IsEmpty()) { M.CodeLines.Pop(); }
			M.MessageLines.Add(TEXT("VANT: ") + Item.Prompt);
			for (int32 i = 0; i < ShownOptions.Num(); ++i)
			{
				M.MessageLines.Add(FString::Printf(TEXT("%s %d   %s"), i == Selection ? TEXT(">") : TEXT(" "), i + 1, *ShownOptions[i].Text.Replace(TEXT("\n"), TEXT("  /  "))));
			}
		}
		else
		{
			const FGitsLikertItem& Item = I->Items[ItemIndex];
			if (ItemIndex == 0) { M.MessageLines.Add(TEXT("VANT: ") + I->Blurb); M.MessageLines.Add(TEXT("")); }
			M.MessageLines.Add(Item.Text);
			M.MessageLines.Add(TEXT(""));
			for (int32 i = 0; i < I->Scale.Labels.Num(); ++i)
			{
				M.MessageLines.Add(FString::Printf(TEXT("%s %d   %s"), i == Selection ? TEXT(">") : TEXT(" "), i + 1, *I->Scale.Labels[i]));
			}
		}
		M.Status = FString::Printf(TEXT("answered %d   skipped %d"), I->Answered(Responses), ItemIndex - I->Answered(Responses));
		HintText = TEXT("up/down: choose   enter: answer   s: skip this one");
		break;
	}
	case EStep::Exported:
		M.Title = TEXT("STUDY TERMINAL  done");
		if (ExportError.IsEmpty())
		{
			M.MessageLines.Add(TEXT("VANT: Thank you. That is the last of it."));
			M.MessageLines.Add(TEXT(""));
			M.MessageLines.Add(TEXT("Your session is exported, under your code only:"));
			M.MessageLines.Add(ExportPath);
		}
		else { M.MessageLines.Add(TEXT("!") + ExportError); M.bStatusIsError = true; }
		M.Status = TEXT("enter: step away");
		HintText = TEXT("enter: step away");
		break;
	default:
		M.Title = TEXT("STUDY TERMINAL");
		M.MessageLines.Add(TEXT("VANT: Done. Go and play."));
		M.Status = TEXT("enter: step away");
		HintText = TEXT("enter: step away");
		break;
	}
	Screen->SetModel(M);
	if (Hint.IsValid()) { Hint->SetText(FText::FromString(HintText)); }
}

void SGitsInstrumentPanel::Move(int32 Direction)
{
	const int32 N = OptionCount();
	if (N <= 0) { return; }
	Selection = (Selection + Direction + N) % N;
	Refresh();
}

void SGitsInstrumentPanel::StartInstrument(int32 Index)
{
	InstrumentIndex = Index;
	ItemIndex = 0;
	Selection = 0;
	Responses = FGitsResponses();
	const FGitsInstrument* I = Sequence.IsValidIndex(Index) ? Sequence[Index] : nullptr;
	UGitsTelemetrySubsystem* T = Telemetry();
	if (!I) { FinishSequence(); return; }
	if (T)
	{
		// A questionnaire is not part of any shift; the level is nulled so its answers cannot
		// quietly join a level in the analysis.
		T->ClearLevel();
		TSharedPtr<FJsonObject> P = UGitsTelemetrySubsystem::Payload();
		P->SetStringField(TEXT("instrumentId"), I->Id);
		P->SetStringField(TEXT("occasion"), Occasion);
		P->SetNumberField(TEXT("items"), I->ItemCount());
		T->Record(TEXT("instrument_started"), P);
	}
	if (I->Kind == FGitsInstrument::EKind::Quiz) { ShownOptions = GitsInstruments::PresentedOptions(I->QuizItems[0], T ? T->GetParticipantCode() : FString()); }
	ItemShownAt = FPlatformTime::Seconds();
}

void SGitsInstrumentPanel::AdvanceItem()
{
	const FGitsInstrument* I = Sequence[InstrumentIndex];
	++ItemIndex;
	Selection = 0;
	if (ItemIndex >= I->ItemCount()) { CompleteInstrument(); return; }
	if (I->Kind == FGitsInstrument::EKind::Quiz)
	{
		UGitsTelemetrySubsystem* T = Telemetry();
		ShownOptions = GitsInstruments::PresentedOptions(I->QuizItems[ItemIndex], T ? T->GetParticipantCode() : FString());
	}
	ItemShownAt = FPlatformTime::Seconds();
	Refresh();
}

void SGitsInstrumentPanel::CompleteInstrument()
{
	const FGitsInstrument* I = Sequence[InstrumentIndex];
	if (UGitsTelemetrySubsystem* T = Telemetry())
	{
		TSharedPtr<FJsonObject> P = UGitsTelemetrySubsystem::Payload();
		P->SetStringField(TEXT("instrumentId"), I->Id);
		P->SetStringField(TEXT("occasion"), Occasion);
		P->SetNumberField(TEXT("answered"), I->Answered(Responses));
		P->SetNumberField(TEXT("total"), I->ItemCount());
		P->SetObjectField(TEXT("score"), I->Score(Responses));
		T->Record(TEXT("instrument_completed"), P);
	}
	if (InstrumentIndex + 1 < Sequence.Num()) { StartInstrument(InstrumentIndex + 1); Refresh(); }
	else { FinishSequence(); }
}

void SGitsInstrumentPanel::FinishSequence()
{
	UGitsTelemetrySubsystem* T = Telemetry();
	if (Occasion == TEXT("post") && T)
	{
		TSharedPtr<FJsonObject> P = UGitsTelemetrySubsystem::Payload();
		P->SetNumberField(TEXT("durationMs"), T->NowMs() - T->GetEvents()[0].Timestamp);
		T->Record(TEXT("session_end"), P);
		if (!T->Export(ExportPath, ExportError)) { ExportPath.Reset(); }
		Step = EStep::Exported;
	}
	else { Step = EStep::Done; }
	Selection = 0;
	bFinished = true;
	Refresh();
}

void SGitsInstrumentPanel::Confirm()
{
	UGitsTelemetrySubsystem* T = Telemetry();
	switch (Step)
	{
	case EStep::Consent:
		if (Selection == 2)
		{
			if (AGhostInTheStackPlayerController* PC = Controller.Get()) { PC->CloseInstrumentPanel(); }
			return;
		}
		bCodeCapture = Selection == 1;
		Step = EStep::Experience;
		Selection = 0;
		Refresh();
		return;
	case EStep::Experience:
	{
		static const TCHAR* Bands[] = { TEXT("none"), TEXT("some"), TEXT("formal"), TEXT("declined") };
		uint32 Seed = 0;
		if (const AGhostInTheStackPlayerController* PC = Controller.Get())
		{
			if (const UGitsVantSubsystem* V = PC->GetWorld() ? PC->GetWorld()->GetSubsystem<UGitsVantSubsystem>() : nullptr) { Seed = V->GetSessionSeed(); }
		}
		if (T) { T->RecordConsent(bCodeCapture, Bands[FMath::Clamp(Selection, 0, 3)], Seed); }
		if (Sequence.Num() == 0 && T) { Sequence.Add(&GitsInstruments::TracingTest(GitsInstruments::FormFor(T->GetParticipantCode(), Occasion))); }
		Step = EStep::Instrument;
		StartInstrument(0);
		Refresh();
		return;
	}
	case EStep::Instrument:
	{
		const FGitsInstrument* I = Sequence[InstrumentIndex];
		const double Latency = (FPlatformTime::Seconds() - ItemShownAt) * 1000.0;
		FString ItemId;
		TSharedPtr<FJsonObject> P = UGitsTelemetrySubsystem::Payload();
		if (I->Kind == FGitsInstrument::EKind::Quiz)
		{
			if (!ShownOptions.IsValidIndex(Selection)) { return; }
			ItemId = I->QuizItems[ItemIndex].Id;
			Responses.Options.Add(ItemId, ShownOptions[Selection].Id);
			P->SetStringField(TEXT("value"), ShownOptions[Selection].Id);
		}
		else
		{
			ItemId = I->Items[ItemIndex].Id;
			const double Value = I->Scale.FirstValue + Selection;
			Responses.Numbers.Add(ItemId, Value);
			P->SetNumberField(TEXT("value"), Value);
		}
		if (T)
		{
			P->SetStringField(TEXT("instrumentId"), I->Id);
			P->SetStringField(TEXT("occasion"), Occasion);
			P->SetStringField(TEXT("itemId"), ItemId);
			P->SetBoolField(TEXT("skipped"), false);
			P->SetNumberField(TEXT("latencyMs"), FMath::RoundToDouble(Latency));
			T->Record(TEXT("instrument_response"), P);
		}
		AdvanceItem();
		return;
	}
	default:
		if (AGhostInTheStackPlayerController* PC = Controller.Get()) { PC->CloseInstrumentPanel(); }
		return;
	}
}

bool SGitsInstrumentPanel::Skip()
{
	if (Step != EStep::Instrument) { return false; }
	const FGitsInstrument* I = Sequence[InstrumentIndex];
	const FString ItemId = I->Kind == FGitsInstrument::EKind::Quiz ? I->QuizItems[ItemIndex].Id : I->Items[ItemIndex].Id;
	if (UGitsTelemetrySubsystem* T = Telemetry())
	{
		// A skip is recorded; an item never reached is not. The difference is the distinction.
		TSharedPtr<FJsonObject> P = UGitsTelemetrySubsystem::Payload();
		P->SetStringField(TEXT("instrumentId"), I->Id);
		P->SetStringField(TEXT("occasion"), Occasion);
		P->SetStringField(TEXT("itemId"), ItemId);
		P->SetField(TEXT("value"), MakeShared<FJsonValueNull>());
		P->SetBoolField(TEXT("skipped"), true);
		P->SetNumberField(TEXT("latencyMs"), FMath::RoundToDouble((FPlatformTime::Seconds() - ItemShownAt) * 1000.0));
		T->Record(TEXT("instrument_response"), P);
	}
	AdvanceItem();
	return true;
}

bool SGitsInstrumentPanel::Answer(int32 Index)
{
	if (Index < 1 || Index > OptionCount()) { return false; }
	Selection = Index - 1;
	Confirm();
	return true;
}

FString SGitsInstrumentPanel::Describe() const
{
	const FGitsInstrument* I = Sequence.IsValidIndex(InstrumentIndex) ? Sequence[InstrumentIndex] : nullptr;
	return FString::Printf(TEXT("step=%d instrument=%s item=%d/%d finished=%d export=%s %s"), (int32)Step, I ? *I->Id : TEXT("-"), ItemIndex, I ? I->ItemCount() : 0, bFinished, *ExportPath, *ExportError);
}

FReply SGitsInstrumentPanel::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Up || Key == EKeys::Gamepad_DPad_Up) { Move(-1); return FReply::Handled(); }
	if (Key == EKeys::Down || Key == EKeys::Gamepad_DPad_Down) { Move(+1); return FReply::Handled(); }
	if (Key == EKeys::Enter || Key == EKeys::Gamepad_FaceButton_Bottom) { Confirm(); return FReply::Handled(); }
	if (Key == EKeys::S || Key == EKeys::Gamepad_FaceButton_Top) { Skip(); return FReply::Handled(); }
	if (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right)
	{
		if (Step == EStep::Consent || Step == EStep::Exported || Step == EStep::Done)
		{
			if (AGhostInTheStackPlayerController* PC = Controller.Get()) { PC->CloseInstrumentPanel(); }
		}
		return FReply::Handled();
	}
	return FReply::Handled();
}
