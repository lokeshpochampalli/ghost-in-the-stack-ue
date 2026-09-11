#include "GitsVantCaption.h"
#include "GitsScreen.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"

void SGitsVantCaption::Construct(const FArguments& InArgs)
{
	SetVisibility(EVisibility::HitTestInvisible);
	ChildSlot
	.HAlign(HAlign_Center)
	.VAlign(VAlign_Bottom)
	.Padding(FMargin(0, 0, 0, 72))
	[
		SNew(SBox).MaxDesiredWidth(1100.f)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
			.BorderBackgroundColor(FLinearColor(GitsStyle::Ink.R, GitsStyle::Ink.G, GitsStyle::Ink.B, 0.82f))
			.Padding(FMargin(22, 12))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top).Padding(0, 0, 14, 0)
				[
					SNew(STextBlock).Font(GitsStyle::Mono(20)).ColorAndOpacity(GitsStyle::Copper).Text(FText::FromString(TEXT("VANT")))
				]
				+ SHorizontalBox::Slot().FillWidth(1.f)
				[
					SAssignNew(Text, STextBlock).Font(GitsStyle::Mono(20)).ColorAndOpacity(GitsStyle::Bone).AutoWrapText(true)
				]
			]
		]
	];
	SetRenderOpacity(0.f);
}

void SGitsVantCaption::SetLine(const FString& Line)
{
	if (Text.IsValid()) { Text->SetText(FText::FromString(Line)); }
	// Long enough to read at a comfortable pace, never gone in a blink.
	HoldSeconds = FMath::Clamp(2.5f + 0.045f * Line.Len(), 3.f, 10.f);
	Age = 0.f;
	bShowing = true;
	SetRenderOpacity(1.f);
}

void SGitsVantCaption::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	if (!bShowing) { return; }
	Age += InDeltaTime;
	const float Fade = 0.7f;
	if (Age <= HoldSeconds) { SetRenderOpacity(1.f); }
	else if (Age < HoldSeconds + Fade) { SetRenderOpacity(1.f - (Age - HoldSeconds) / Fade); }
	else { SetRenderOpacity(0.f); bShowing = false; }
}
