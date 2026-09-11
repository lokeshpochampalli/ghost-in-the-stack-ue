// Ghost in the Stack — the curriculum map (port of src/content/tags.ts).
//
// Adding a tag requires adding it here first. The concept list produces the curriculum
// table the dissertation needs; the misconception list is what makes the telemetry say
// something rather than count something. A script naming a tag that is not here is a
// content error, reported by Validate.
#pragma once

#include "CoreMinimal.h"

class UGitsScript;

namespace GitsTags
{
	const TArray<FString>& Concepts();
	const TArray<FString>& Misconceptions();
	bool IsConcept(const FString& Tag);
	bool IsMisconception(const FString& Tag);

	/** Every problem with a script's predictions, hints, tags and costs, in plain language. Empty means valid. */
	TArray<FString> Validate(const UGitsScript* Script);
	/** The power rules: a run costs something, the discount is real, and the sector's budget covers a run. */
	TArray<FString> ValidatePower(const UGitsScript* Script, int32 Budget);
}
