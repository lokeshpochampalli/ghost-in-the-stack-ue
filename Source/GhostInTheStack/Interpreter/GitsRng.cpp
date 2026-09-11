#include "GitsRng.h"

namespace GitsRng
{
	double FMulberry32::Next()
	{
		State += 0x6d2b79f5u;
		uint32 T = State;
		T = (T ^ (T >> 15)) * (T | 1u);
		T ^= T + (T ^ (T >> 7)) * (T | 61u);
		return (double)(T ^ (T >> 14)) / 4294967296.0;
	}

	int32 FMulberry32::NextInt(int32 Bound)
	{
		return Bound <= 0 ? 0 : (int32)FMath::FloorToDouble(Next() * Bound);
	}

	uint32 HashString(const FString& Text)
	{
		uint32 Hash = 0x811c9dc5u;
		for (const TCHAR C : Text)
		{
			Hash ^= (uint32)(uint16)C;
			Hash *= 0x01000193u;
		}
		return Hash;
	}

	FString HashToken(const FString& Text)
	{
		return FString::Printf(TEXT("%08x"), HashString(Text));
	}
}
