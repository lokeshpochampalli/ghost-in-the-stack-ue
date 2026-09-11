// Ghost in the Stack — VANT's caption: what the station just said, wherever the player looks.
//
// Text only until the dialogue is final (CLAUDE.md: no voice acting before Phase 11). A line
// stays up long enough to read, then fades.
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class STextBlock;

class SGitsVantCaption : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGitsVantCaption) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void SetLine(const FString& Line);
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	TSharedPtr<STextBlock> Text;
	float HoldSeconds = 0.f;
	float Age = 0.f;
	bool bShowing = false;
};
