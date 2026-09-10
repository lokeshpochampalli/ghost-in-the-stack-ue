// Ghost in the Stack — a script asset: one of Ilse's programs, bound to a terminal.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GitsScript.generated.h"

/**
 * Ilse's code for one station system, as a data asset under Content/Scripts.
 * The source is the program the terminal shows; the level's tier and builtins gate it.
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

	/** The source split into lines, without trailing newline handling surprises. */
	TArray<FString> Lines() const
	{
		TArray<FString> Out;
		FString S = Source.Replace(TEXT("\r\n"), TEXT("\n"));
		S.ParseIntoArray(Out, TEXT("\n"), false);
		while (Out.Num() > 0 && Out.Last().IsEmpty()) { Out.Pop(); }
		return Out;
	}
};
