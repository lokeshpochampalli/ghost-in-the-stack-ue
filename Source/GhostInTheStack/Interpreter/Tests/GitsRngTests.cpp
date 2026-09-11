// RNG parity with the reference src/core/rng.ts: same seed, same draws, same shuffle.
#include "GitsTestUtil.h"
#include "Interpreter/GitsRng.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGitsRngParity, "GhostInTheStack.Interpreter.Rng.Parity", GitsTest::TestFlags)
bool FGitsRngParity::RunTest(const FString&)
{
	// Values computed by running the TypeScript algorithm (see docs/STATION-LAYER.md, VANT).
	TestEqual(TEXT("fnv1a abc"), GitsRng::HashString(TEXT("abc")), 440920331u);
	TestEqual(TEXT("fnv1a s1p1"), GitsRng::HashString(TEXT("s1p1")), 301899966u);
	TestEqual(TEXT("fnv1a empty"), GitsRng::HashString(TEXT("")), 2166136261u);
	TestEqual(TEXT("hash token"), GitsRng::HashToken(TEXT("abc")), FString(TEXT("1a47e90b")));
	{
		GitsRng::FMulberry32 Rng(1);
		TestTrue(TEXT("seed 1 draw 1"), FMath::IsNearlyEqual(Rng.Next(), 0.6270739405881613, 1e-12));
		TestTrue(TEXT("seed 1 draw 2"), FMath::IsNearlyEqual(Rng.Next(), 0.002735721180215478, 1e-12));
		TestTrue(TEXT("seed 1 draw 3"), FMath::IsNearlyEqual(Rng.Next(), 0.5274470399599522, 1e-12));
	}
	{
		GitsRng::FMulberry32 Rng(GitsRng::HashString(TEXT("s1p1")));
		TestTrue(TEXT("seed s1p1 draw 1"), FMath::IsNearlyEqual(Rng.Next(), 0.7851370004937053, 1e-12));
		TestTrue(TEXT("seed s1p1 draw 2"), FMath::IsNearlyEqual(Rng.Next(), 0.408889687852934, 1e-12));
	}
	{
		GitsRng::FMulberry32 Rng(GitsRng::HashString(TEXT("s1p1")));
		const TArray<FString> Out = Rng.Shuffle(TArray<FString>{ TEXT("a"), TEXT("b"), TEXT("c"), TEXT("d") });
		TestEqual(TEXT("shuffle s1p1"), FString::Join(Out, TEXT("")), FString(TEXT("cabd")));
	}
	{
		GitsRng::FMulberry32 Rng(42);
		const TArray<FString> Out = Rng.Shuffle(TArray<FString>{ TEXT("a"), TEXT("b"), TEXT("c") });
		TestEqual(TEXT("shuffle 42"), FString::Join(Out, TEXT("")), FString(TEXT("cab")));
		TestEqual(TEXT("bound 0"), GitsRng::FMulberry32(7).NextInt(0), 0);
	}
	return true;
}

#endif
