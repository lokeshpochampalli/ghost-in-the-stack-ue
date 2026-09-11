// Ghost in the Stack — the project's only source of randomness (port of src/core/rng.ts).
//
// A seeded PRNG. Nothing in the game samples a distribution; it shuffles a handful of
// prediction options and must do so identically on every machine and every rerun, because
// research data is worthless if a session cannot be replayed. Seeds are recorded (ADR-011).
#pragma once

#include "CoreMinimal.h"

namespace GitsRng
{
	/** mulberry32: 32 bits of state, one multiply-xor-shift round per draw. */
	struct FMulberry32
	{
		uint32 State = 0;
		explicit FMulberry32(uint32 Seed) : State(Seed) {}
		/** A float in [0, 1). */
		double Next();
		/** A whole number in [0, Bound). */
		int32 NextInt(int32 Bound);
		/** Fisher-Yates, walked from the end so the draw sequence is fixed by length. */
		template <typename T>
		TArray<T> Shuffle(const TArray<T>& Items)
		{
			TArray<T> Out = Items;
			for (int32 i = Out.Num() - 1; i > 0; --i)
			{
				const int32 j = NextInt(i + 1);
				Out.Swap(i, j);
			}
			return Out;
		}
	};

	/** FNV-1a, 32-bit, over UTF-16 code units as JavaScript's charCodeAt sees them. */
	uint32 HashString(const FString& Text);
	/** The hash as eight hex digits, for a telemetry payload. */
	FString HashToken(const FString& Text);
}
