// Ghost in the Stack — the station's screens: a monospace phosphor panel in Slate.
//
// One widget draws both the terminal (code with keyword tint and a highlighted line) and
// the wall display (message lines). It is plain Slate so it renders identically on a
// world-space widget component and in the viewport overlay. The visual direction from
// CLAUDE.md: a game terminal, not an IDE; warm phosphor; amber only for power and errors.
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Blueprint/UserWidget.h"
#include "GitsScreen.generated.h"

class SVerticalBox;
class STextBlock;

/** What a screen shows. Set the whole thing; the widget rebuilds what changed. */
struct FGitsScreenModel
{
	FString Title;
	/** Code lines, tinted. Empty for a message-only screen. */
	TArray<FString> CodeLines;
	/** 1-based line to highlight as executing, 0 for none. */
	int32 HighlightLine = 0;
	/** 1-based line the player has selected for editing, 0 for none. */
	int32 SelectedLine = 0;
	/** Lines the player may edit; empty means all. */
	TArray<int32> EditableLines;
	/** Free text lines below the code, e.g. output, effects, VANT. */
	TArray<FString> MessageLines;
	/** One-line status at the bottom. */
	FString Status;
	/** Draw the status in amber: an error or a refusal. */
	bool bStatusIsError = false;
};

namespace GitsStyle
{
	FSlateFontInfo Mono(int32 Size);
	extern const FLinearColor Slate;
	extern const FLinearColor Copper;
	extern const FLinearColor Bone;
	extern const FLinearColor Ink;
	extern const FLinearColor Amber;
	extern const FLinearColor Phosphor;   // the screen's text
	extern const FLinearColor ScreenBack; // the screen's glass

	enum class ETint : uint8 { Plain, Keyword, Name, Number, String, Comment, Op, Builtin };
	struct FRun { FString Text; ETint Tint; };
	/** Splits a source line into tint runs. Comments are Ilse's notes and stay readable. */
	TArray<FRun> Tint(const FString& Line);
	FLinearColor ColorFor(ETint Tint);
}

class SGitsScreen : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGitsScreen) : _FontSize(18), _MaxCodeRows(0), _MaxMessageRows(0) {}
		SLATE_ARGUMENT(int32, FontSize)
		/** Most code lines shown at once (0 = all); the window follows the selected or executing line. */
		SLATE_ARGUMENT(int32, MaxCodeRows)
		/** Most message lines shown at once (0 = all); the newest are kept. */
		SLATE_ARGUMENT(int32, MaxMessageRows)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void SetModel(const FGitsScreenModel& InModel);
	const FGitsScreenModel& GetModel() const { return Model; }
	/** Fits the code and message windows to the space the screen actually has. */
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	void Rebuild();
	TSharedRef<SWidget> MakeCodeLine(int32 LineNumber, const FString& Line);

	FGitsScreenModel Model;
	int32 FontSize = 18;
	int32 MaxCodeRows = 0;
	int32 MaxMessageRows = 0;
	/** Row budgets fitted from the geometry (0 = not fitted yet, show everything). */
	int32 FitCodeRows = 0;
	int32 FitMessageRows = 0;
	TSharedPtr<STextBlock> TitleText;
	TSharedPtr<SVerticalBox> Body;
	TSharedPtr<STextBlock> StatusText;
	TSharedPtr<SVerticalBox> StatusBox;
};

/** The UMG wrapper a widget component needs. Holds one SGitsScreen. */
UCLASS()
class GHOSTINTHESTACK_API UGitsScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetModel(const FGitsScreenModel& InModel);
	int32 FontSize = 18;
	/** Row budgets, so a long script or a long report scrolls instead of running under the status line. */
	int32 MaxCodeRows = 0;
	int32 MaxMessageRows = 0;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

private:
	TSharedPtr<SGitsScreen> Screen;
	FGitsScreenModel Pending;
	bool bHasPending = false;
};
