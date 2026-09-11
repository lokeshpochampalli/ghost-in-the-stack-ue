// Ghost in the Stack — a script asset: one of Ilse's programs, bound to a terminal.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GitsScript.generated.h"

/** One answer to a prediction. Distractors carry the belief that would pick them. */
USTRUCT(BlueprintType)
struct FGitsPredictionOption
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Prediction")
	FString Id;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Prediction")
	FString Label;

	/** Empty on the correct answer, a misconception tag (Companion/GitsTags) on every distractor. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Prediction")
	FString Misconception;
};

/** A question VANT asks before the first run, anchored to a source line (ADR-005). */
USTRUCT(BlueprintType)
struct FGitsPrediction
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Prediction")
	FString Id;

	/** In VANT's voice. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Prediction", meta = (MultiLine = true))
	FString Prompt;

	/** 1-based line whose statement settles the question... */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Prediction")
	int32 AnchorLine = 1;

	/** ...and which execution of it, so a line inside a loop can be asked about. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Prediction")
	int32 AnchorOccurrence = 1;

	/** output, value, branch, count or order. Descriptive; the options carry the answer. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Prediction")
	FString Kind = TEXT("output");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Prediction")
	TArray<FGitsPredictionOption> Options;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Prediction")
	FString CorrectId;
};

/** A tiered nudge in Ilse's voice: 1 nudge, 2 explanation, 3 near-answer. Never a tooltip explaining Python. */
USTRUCT(BlueprintType)
struct FGitsHint
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hint", meta = (ClampMin = 1, ClampMax = 3))
	int32 Tier = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hint", meta = (MultiLine = true))
	FString Text;

	/** Charged from Phase 5; recorded now. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hint")
	int32 CostsPower = 0;
};

/**
 * Ilse's code for one station system, as a data asset under Content/Scripts, with what VANT
 * asks about it, the notes Ilse left, and what "fixed" looks like.
 */
UCLASS(BlueprintType)
class GHOSTINTHESTACK_API UGitsScript : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Shown at the top of the terminal screen. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Script")
	FString Title = TEXT("untitled");

	/** The program. Four-space indentation, Python subset per docs/LANGUAGE-SPEC.md. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Script", meta = (MultiLine = true))
	FString Source;

	/** Highest tier the script uses, 1 to 4. The parser refuses anything above it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Script", meta = (ClampMin = 1, ClampMax = 4))
	int32 Tier = 1;

	/** Per-level design budget in statement executions (ADR-021). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Script")
	int32 StatementCap = 2000;

	/** 1-based line numbers the player may edit. Empty means every line. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Script")
	TArray<int32> EditableLines;

	/** Station functions this script may call. Empty means all of them. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Script")
	TArray<FString> DeclaredBuiltins;

	/** Concept tags (Companion/GitsTags), for the curriculum table. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Curriculum")
	TArray<FString> Concepts;

	/** What VANT asks before the first run. Empty on a Make script. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Curriculum")
	TArray<FGitsPrediction> Predictions;

	/** Ilse's notes, revealed a tier at a time. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Curriculum")
	TArray<FGitsHint> Hints;

	/** VANT's line the first time the terminal is used. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative", meta = (MultiLine = true))
	FString Intro;

	/** VANT's line when a run leaves the goal met. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative", meta = (MultiLine = true))
	FString Outro;

	/** The goal as a world assertion: the key (door.inner) and the value it must hold (true). Empty key means no goal. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative")
	FString GoalKey;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative")
	FString GoalValue;

	/** The source split into lines, without trailing newline handling surprises. */
	TArray<FString> Lines() const
	{
		TArray<FString> Out;
		FString S = Source.Replace(TEXT("\r\n"), TEXT("\n"));
		S.ParseIntoArray(Out, TEXT("\n"), false);
		while (Out.Num() > 0 && Out.Last().IsEmpty()) { Out.Pop(); }
		return Out;
	}

	const FGitsPrediction* FindPrediction(const FString& Id) const
	{
		return Predictions.FindByPredicate([&Id](const FGitsPrediction& P) { return P.Id == Id; });
	}
};
