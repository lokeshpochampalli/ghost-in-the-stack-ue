#include "GitsTags.h"
#include "Station/GitsScript.h"

namespace GitsTags
{
	const TArray<FString>& Concepts()
	{
		static const TArray<FString> List = {
			TEXT("variable-assignment"), TEXT("reassignment"), TEXT("data-types"), TEXT("type-coercion"),
			TEXT("arithmetic"), TEXT("string-ops"), TEXT("output"), TEXT("conditional"), TEXT("elif-chain"),
			TEXT("boolean-logic"), TEXT("comparison"), TEXT("truthiness"), TEXT("while-loop"), TEXT("for-loop"),
			TEXT("range"), TEXT("accumulator"), TEXT("nested-loop"), TEXT("augmented-assignment"),
			TEXT("list-literal"), TEXT("indexing"), TEXT("list-mutation"), TEXT("function-def"),
			TEXT("parameters"), TEXT("return-value"), TEXT("local-scope"), TEXT("call-stack"), TEXT("recursion"),
		};
		return List;
	}

	const TArray<FString>& Misconceptions()
	{
		// Each entry is a belief a novice actually holds, not a way of being wrong.
		static const TArray<FString> List = {
			TEXT("assignment-as-equality"), TEXT("parallel-assignment"), TEXT("sequence-ignored"),
			TEXT("type-confusion"), TEXT("branch-both"), TEXT("inverted-comparison"), TEXT("operator-confusion"),
			TEXT("loop-runs-once"), TEXT("fencepost"), TEXT("accumulator-reset"), TEXT("index-from-one"),
			TEXT("scope-leak"), TEXT("return-vs-print"), TEXT("arg-param-identity"), TEXT("recursion-no-return"),
		};
		return List;
	}

	bool IsConcept(const FString& Tag) { return Concepts().Contains(Tag); }
	bool IsMisconception(const FString& Tag) { return Misconceptions().Contains(Tag); }

	TArray<FString> ValidatePower(const UGitsScript* Script, int32 Budget)
	{
		TArray<FString> Problems;
		if (!Script) { Problems.Add(TEXT("no script")); return Problems; }
		if (Script->RunCost <= 0) { Problems.Add(TEXT("a run must cost something (RunCost is 0)")); }
		if (Script->PredictedRunCost >= Script->RunCost) { Problems.Add(FString::Printf(TEXT("PredictedRunCost %d is not below RunCost %d"), Script->PredictedRunCost, Script->RunCost)); }
		if (Budget < Script->RunCost) { Problems.Add(FString::Printf(TEXT("the sector budget %d cannot cover a full-price run of %d"), Budget, Script->RunCost)); }
		return Problems;
	}

	TArray<FString> Validate(const UGitsScript* Script)
	{
		TArray<FString> Problems;
		if (!Script) { Problems.Add(TEXT("no script")); return Problems; }
		for (const FString& C : Script->Concepts)
		{
			if (!IsConcept(C)) { Problems.Add(FString::Printf(TEXT("concept tag '%s' is not in the curriculum map"), *C)); }
		}
		const int32 LineCount = Script->Lines().Num();
		TSet<FString> Ids;
		for (const FGitsPrediction& P : Script->Predictions)
		{
			const FString Where = FString::Printf(TEXT("prediction '%s'"), *P.Id);
			if (P.Id.IsEmpty()) { Problems.Add(TEXT("a prediction has no id")); }
			if (Ids.Contains(P.Id)) { Problems.Add(Where + TEXT(": duplicate id")); }
			Ids.Add(P.Id);
			if (P.Prompt.IsEmpty()) { Problems.Add(Where + TEXT(": no prompt")); }
			if (P.AnchorLine < 1 || P.AnchorLine > LineCount) { Problems.Add(Where + FString::Printf(TEXT(": anchor line %d is outside the script"), P.AnchorLine)); }
			if (P.AnchorOccurrence < 1) { Problems.Add(Where + TEXT(": anchor occurrence must be 1 or more")); }
			if (P.Options.Num() < 2 || P.Options.Num() > 4) { Problems.Add(Where + FString::Printf(TEXT(": %d options, wanted 2 to 4"), P.Options.Num())); }
			bool bCorrectFound = false;
			TSet<FString> OptionIds;
			for (const FGitsPredictionOption& O : P.Options)
			{
				if (O.Id.IsEmpty()) { Problems.Add(Where + TEXT(": an option has no id")); }
				if (OptionIds.Contains(O.Id)) { Problems.Add(Where + FString::Printf(TEXT(": duplicate option id '%s'"), *O.Id)); }
				OptionIds.Add(O.Id);
				const bool bCorrect = O.Id == P.CorrectId;
				bCorrectFound |= bCorrect;
				if (bCorrect && !O.Misconception.IsEmpty()) { Problems.Add(Where + TEXT(": the correct option carries a misconception tag")); }
				if (!bCorrect && O.Misconception.IsEmpty()) { Problems.Add(Where + FString::Printf(TEXT(": distractor '%s' has no misconception tag; a distractor that is merely incorrect produces noise"), *O.Id)); }
				if (!O.Misconception.IsEmpty() && !IsMisconception(O.Misconception)) { Problems.Add(Where + FString::Printf(TEXT(": misconception tag '%s' is not in the map"), *O.Misconception)); }
			}
			if (!bCorrectFound) { Problems.Add(Where + FString::Printf(TEXT(": correct id '%s' names no option"), *P.CorrectId)); }
		}
		if (Script->RunCost <= 0) { Problems.Add(TEXT("a run must cost something (RunCost is 0)")); }
		if (Script->PredictedRunCost >= Script->RunCost) { Problems.Add(FString::Printf(TEXT("PredictedRunCost %d is not below RunCost %d; the discount must be real"), Script->PredictedRunCost, Script->RunCost)); }
		if (Script->TestCases.Num() > 0 && Script->Predictions.Num() > 0) { Problems.Add(TEXT("a Make script has tests and no predictions (ADR-012)")); }
		if (Script->TestCases.Num() > 0 && !Script->bFreeEdit) { Problems.Add(TEXT("a Make script must allow free editing")); }
		for (const FGitsTestCase& T : Script->TestCases)
		{
			if (T.Label.IsEmpty()) { Problems.Add(TEXT("a test case has no label; the label is what the player sees")); }
			if (T.ExpectedOutput.Num() == 0 && T.ExpectedWorld.Num() == 0) { Problems.Add(FString::Printf(TEXT("test '%s' expects nothing"), *T.Label)); }
		}
		if (!Script->GoalUnlocks.IsEmpty() && !Script->GoalUnlocks.Contains(TEXT("="))) { Problems.Add(TEXT("GoalUnlocks must be key=value")); }
		if (Script->PredictedRunCost < 0) { Problems.Add(TEXT("PredictedRunCost is negative")); }
		TSet<int32> Tiers;
		for (const FGitsHint& H : Script->Hints)
		{
			if (H.Tier < 1 || H.Tier > 3) { Problems.Add(FString::Printf(TEXT("hint tier %d is not 1, 2 or 3"), H.Tier)); }
			if (Tiers.Contains(H.Tier)) { Problems.Add(FString::Printf(TEXT("two hints at tier %d"), H.Tier)); }
			Tiers.Add(H.Tier);
			if (H.Text.IsEmpty()) { Problems.Add(FString::Printf(TEXT("hint tier %d has no text"), H.Tier)); }
		}
		return Problems;
	}
}
