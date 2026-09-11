// Ghost in the Stack — the power bus as rules: what "low" means and what the lights do about it.
//
// Running draws power; a committed prediction discounts it (ADR-006). Below half the budget
// the corridor dims in stages; at zero only the emergency lighting and the torch are on, and
// the reserve cell at the generator is the way back. Plain functions so the stages are testable.
#pragma once

#include "CoreMinimal.h"

namespace GitsPower
{
	enum class EStage : uint8 { Nominal, Low, Critical, Out };

	/** The stage for a bus holding Power of Budget. A bus with no budget declared is nominal. */
	inline EStage StageFor(int32 Power, int32 Budget)
	{
		if (Budget <= 0) { return EStage::Nominal; }
		if (Power <= 0) { return EStage::Out; }
		const float Fraction = (float)Power / (float)Budget;
		if (Fraction > 0.5f) { return EStage::Nominal; }
		if (Fraction > 0.25f) { return EStage::Low; }
		return EStage::Critical;
	}

	/** How bright the corridor rig runs at each stage, as a multiplier on the scripted level. */
	inline float LightFactor(EStage Stage)
	{
		switch (Stage)
		{
		case EStage::Nominal: return 1.f;
		case EStage::Low: return 0.45f;
		case EStage::Critical: return 0.15f;
		default: return 0.f;
		}
	}

	inline const TCHAR* StageName(EStage Stage)
	{
		switch (Stage)
		{
		case EStage::Nominal: return TEXT("nominal");
		case EStage::Low: return TEXT("low");
		case EStage::Critical: return TEXT("critical");
		default: return TEXT("out");
		}
	}
}
