#include "GitsScreen.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"
#include "Misc/Paths.h"
#include "Interpreter/GitsLexer.h"

namespace GitsStyle
{
	const FLinearColor Slate = FLinearColor::FromSRGBColor(FColor(0x3A, 0x47, 0x50));
	const FLinearColor Copper = FLinearColor::FromSRGBColor(FColor(0x5F, 0x8A, 0x7D));
	const FLinearColor Bone = FLinearColor::FromSRGBColor(FColor(0xE8, 0xE4, 0xDA));
	const FLinearColor Ink = FLinearColor::FromSRGBColor(FColor(0x1C, 0x21, 0x26));
	const FLinearColor Amber = FLinearColor::FromSRGBColor(FColor(0xD9, 0x9A, 0x2B));
	const FLinearColor Phosphor = FLinearColor::FromSRGBColor(FColor(0xF2, 0xE6, 0xC8));
	const FLinearColor ScreenBack = FLinearColor::FromSRGBColor(FColor(0x14, 0x18, 0x1B));

	FSlateFontInfo Mono(int32 Size)
	{
		static const FString FontPath = FPaths::EngineContentDir() / TEXT("Slate/Fonts/DroidSansMono.ttf");
		return FSlateFontInfo(FontPath, Size);
	}

	FLinearColor ColorFor(ETint Tint)
	{
		switch (Tint)
		{
		case ETint::Keyword: return Copper;
		case ETint::Builtin: return FLinearColor::FromSRGBColor(FColor(0x9C, 0xC4, 0xB6));
		case ETint::Number: return FLinearColor::FromSRGBColor(FColor(0xD6, 0xC7, 0x9A));
		case ETint::String: return FLinearColor::FromSRGBColor(FColor(0xC9, 0xB3, 0x8C));
		case ETint::Comment: return FLinearColor::FromSRGBColor(FColor(0x8A, 0x93, 0x99));
		case ETint::Op: return FLinearColor::FromSRGBColor(FColor(0xB8, 0xB2, 0xA4));
		default: return Phosphor;
		}
	}

	TArray<FRun> Tint(const FString& Line)
	{
		TArray<FRun> Runs;
		auto Push = [&Runs](const FString& Text, ETint T)
		{
			if (Text.IsEmpty()) { return; }
			if (Runs.Num() > 0 && Runs.Last().Tint == T) { Runs.Last().Text += Text; }
			else { Runs.Add({ Text, T }); }
		};
		int32 i = 0;
		const int32 N = Line.Len();
		while (i < N)
		{
			const TCHAR C = Line[i];
			if (C == '#') { Push(Line.Mid(i), ETint::Comment); break; }
			if (C == '"' || C == '\'')
			{
				int32 j = i + 1;
				while (j < N && Line[j] != C) { if (Line[j] == '\\') { ++j; } ++j; }
				j = FMath::Min(j + 1, N);
				Push(Line.Mid(i, j - i), ETint::String);
				i = j;
				continue;
			}
			if (FChar::IsDigit(C))
			{
				int32 j = i;
				while (j < N && (FChar::IsAlnum(Line[j]) || Line[j] == '.')) { ++j; }
				Push(Line.Mid(i, j - i), ETint::Number);
				i = j;
				continue;
			}
			if (FChar::IsAlpha(C) || C == '_')
			{
				int32 j = i;
				while (j < N && (FChar::IsAlnum(Line[j]) || Line[j] == '_')) { ++j; }
				const FString Word = Line.Mid(i, j - i);
				ETint T = ETint::Name;
				if (GitsLexer::IsKeyword(Word)) { T = ETint::Keyword; }
				else
				{
					int32 k = j;
					while (k < N && Line[k] == ' ') { ++k; }
					if (k < N && Line[k] == '(') { T = ETint::Builtin; }
				}
				Push(Word, T);
				i = j;
				continue;
			}
			if (C == ' ') { Push(TEXT(" "), ETint::Plain); ++i; continue; }
			Push(FString::Chr(C), ETint::Op);
			++i;
		}
		return Runs;
	}
}

// --- SGitsScreen -------------------------------------------------------------------------

void SGitsScreen::Construct(const FArguments& InArgs)
{
	FontSize = InArgs._FontSize;
	MaxCodeRows = InArgs._MaxCodeRows;
	MaxMessageRows = InArgs._MaxMessageRows;
	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(GitsStyle::ScreenBack)
		.Padding(FMargin(FontSize * 0.9f, FontSize * 0.6f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, FontSize * 0.5f)
			[
				SAssignNew(TitleText, STextBlock)
				.Font(GitsStyle::Mono(FontSize))
				.ColorAndOpacity(GitsStyle::Copper)
			]
			+ SVerticalBox::Slot().FillHeight(1.f)
			[
				SAssignNew(Body, SVerticalBox)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, FontSize * 0.5f, 0, 0)
			[
				SAssignNew(StatusText, STextBlock)
				.Font(GitsStyle::Mono(FontSize))
				.ColorAndOpacity(GitsStyle::Bone)
				.AutoWrapText(true)
			]
		]
	];
}

void SGitsScreen::SetModel(const FGitsScreenModel& InModel)
{
	Model = InModel;
	Rebuild();
}

TSharedRef<SWidget> SGitsScreen::MakeCodeLine(int32 LineNumber, const FString& Line)
{
	const bool bHighlight = LineNumber == Model.HighlightLine;
	const bool bSelected = LineNumber == Model.SelectedLine;
	const bool bEditable = Model.EditableLines.Num() == 0 || Model.EditableLines.Contains(LineNumber);
	TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
	Row->AddSlot().AutoWidth().Padding(0, 0, FontSize * 0.7f, 0)
	[
		SNew(STextBlock)
		.Font(GitsStyle::Mono(FontSize))
		.Text(FText::FromString(FString::Printf(TEXT("%2d"), LineNumber)))
		.ColorAndOpacity(bSelected ? GitsStyle::Bone : FLinearColor(GitsStyle::Slate.R * 2.2f, GitsStyle::Slate.G * 2.2f, GitsStyle::Slate.B * 2.2f, 1.f))
	];
	Row->AddSlot().AutoWidth().Padding(0, 0, FontSize * 0.4f, 0)
	[
		SNew(STextBlock)
		.Font(GitsStyle::Mono(FontSize))
		.Text(FText::FromString(bSelected ? TEXT(">") : (bHighlight ? TEXT("▸") : TEXT(" "))))
		.ColorAndOpacity(bSelected ? GitsStyle::Bone : GitsStyle::Copper)
	];
	for (const GitsStyle::FRun& R : GitsStyle::Tint(Line))
	{
		FLinearColor Color = GitsStyle::ColorFor(R.Tint);
		if (!bEditable && Model.SelectedLine > 0) { Color.A = 0.55f; }
		Row->AddSlot().AutoWidth()
		[
			SNew(STextBlock)
			.Font(GitsStyle::Mono(FontSize))
			.Text(FText::FromString(R.Text))
			.ColorAndOpacity(Color)
		];
	}
	FLinearColor Back = FLinearColor::Transparent;
	if (bHighlight) { Back = FLinearColor(GitsStyle::Copper.R, GitsStyle::Copper.G, GitsStyle::Copper.B, 0.28f); }
	else if (bSelected) { Back = FLinearColor(GitsStyle::Bone.R, GitsStyle::Bone.G, GitsStyle::Bone.B, 0.12f); }
	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(Back)
		.Padding(FMargin(FontSize * 0.3f, FontSize * 0.08f))
		[ Row ];
}

void SGitsScreen::Rebuild()
{
	if (TitleText.IsValid()) { TitleText->SetText(FText::FromString(Model.Title)); }
	if (Body.IsValid())
	{
		Body->ClearChildren();
		// A window of the code, kept around the line the player is on or the line executing.
		const int32 CodeCount = Model.CodeLines.Num();
		int32 FirstCode = 0, LastCode = CodeCount;
		const int32 CodeRows = FitCodeRows > 0 ? (MaxCodeRows > 0 ? FMath::Min(FitCodeRows, MaxCodeRows) : FitCodeRows) : MaxCodeRows;
		if (CodeRows > 0 && CodeCount > CodeRows)
		{
			const int32 Anchor = Model.SelectedLine > 0 ? Model.SelectedLine : (Model.HighlightLine > 0 ? Model.HighlightLine : 1);
			FirstCode = FMath::Clamp(Anchor - 1 - CodeRows / 2, 0, CodeCount - CodeRows);
			LastCode = FirstCode + CodeRows;
		}
		for (int32 i = FirstCode; i < LastCode; ++i)
		{
			Body->AddSlot().AutoHeight()[ MakeCodeLine(i + 1, Model.CodeLines[i]) ];
		}
		if (Model.CodeLines.Num() > 0 && Model.MessageLines.Num() > 0)
		{
			Body->AddSlot().AutoHeight().Padding(0, FontSize * 0.5f, 0, 0)[ SNew(SBox).HeightOverride(FontSize * 0.3f) ];
		}
		// The newest messages win the space.
		const int32 MessageRows = FitMessageRows > 0 ? (MaxMessageRows > 0 ? FMath::Min(FitMessageRows, MaxMessageRows) : FitMessageRows) : MaxMessageRows;
		const int32 FirstMessage = (MessageRows > 0 && Model.MessageLines.Num() > MessageRows) ? Model.MessageLines.Num() - MessageRows : 0;
		for (int32 mi = FirstMessage; mi < Model.MessageLines.Num(); ++mi)
		{
			const FString& M = Model.MessageLines[mi];
			const bool bVant = M.StartsWith(TEXT("VANT"));
			const bool bErr = M.StartsWith(TEXT("!"));
			Body->AddSlot().AutoHeight().Padding(0, FontSize * 0.05f)
			[
				SNew(STextBlock)
				.Font(GitsStyle::Mono(FontSize))
				.Text(FText::FromString(bErr ? M.Mid(1) : M))
				.ColorAndOpacity(bErr ? GitsStyle::Amber : (bVant ? GitsStyle::Copper : GitsStyle::Phosphor))
				.AutoWrapText(true)
			];
		}
	}
	if (StatusText.IsValid())
	{
		StatusText->SetText(FText::FromString(Model.Status));
		StatusText->SetColorAndOpacity(Model.bStatusIsError ? GitsStyle::Amber : GitsStyle::Bone);
	}
}

void SGitsScreen::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	const float Height = AllottedGeometry.GetLocalSize().Y;
	if (Height <= 0.f || !TitleText.IsValid() || !StatusText.IsValid()) { return; }
	// One text row, as the font actually renders it; code rows carry a little inner padding.
	const float TextH = FMath::Max(1.f, TitleText->GetDesiredSize().Y);
	const float RowH = TextH + FontSize * 0.16f;
	const float MsgH = TextH + FontSize * 0.1f;
	const float Fixed = FontSize * 0.6f * 2.f + (TextH + FontSize * 0.5f) + (StatusText->GetDesiredSize().Y + FontSize * 0.5f);
	const float Avail = Height - Fixed;
	int32 CodeBudget = 0, MessageBudget = 0;
	if (Model.CodeLines.Num() > 0)
	{
		// Code first; keep room for a couple of message rows under it when there are any.
		const float MsgReserve = Model.MessageLines.Num() > 0 ? FontSize * 0.8f + FMath::Min(Model.MessageLines.Num(), 2) * MsgH : 0.f;
		CodeBudget = FMath::Max(1, FMath::FloorToInt((Avail - MsgReserve) / RowH));
		const float CodeUsed = FMath::Min(CodeBudget, Model.CodeLines.Num()) * RowH;
		MessageBudget = FMath::Max(1, FMath::FloorToInt((Avail - CodeUsed - FontSize * 0.8f) / MsgH));
	}
	else
	{
		MessageBudget = FMath::Max(1, FMath::FloorToInt(Avail / MsgH));
	}
	if (CodeBudget != FitCodeRows || MessageBudget != FitMessageRows)
	{
		FitCodeRows = CodeBudget;
		FitMessageRows = MessageBudget;
		Rebuild();
	}
}

// --- UGitsScreenWidget -------------------------------------------------------------------

TSharedRef<SWidget> UGitsScreenWidget::RebuildWidget()
{
	Screen = SNew(SGitsScreen).FontSize(FontSize).MaxCodeRows(MaxCodeRows).MaxMessageRows(MaxMessageRows);
	if (bHasPending) { Screen->SetModel(Pending); }
	return Screen.ToSharedRef();
}

void UGitsScreenWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	Screen.Reset();
}

void UGitsScreenWidget::SetModel(const FGitsScreenModel& InModel)
{
	Pending = InModel;
	bHasPending = true;
	if (Screen.IsValid()) { Screen->SetModel(InModel); }
}
