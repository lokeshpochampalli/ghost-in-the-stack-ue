// The thirty golden traces (ADR-015). The fixtures are the spec: the C++ interpreter is
// correct when it reproduces every Fixtures/<name>.trace.txt byte for byte from
// Fixtures/<name>.py. One automation test per fixture, discovered from the folder.
#include "GitsTestUtil.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Per-fixture inputs the trace files cannot carry: the covers line and the cap. Mirrors the reference harness. */
	struct FFixtureInfo { const TCHAR* Name; const TCHAR* Covers; int32 Cap; };
	const FFixtureInfo Manifest[] = {
		{ TEXT("a1-l03-cold-store"), TEXT("Tier 1: assignment, reassignment, arithmetic, output. The first real level."), 0 },
		{ TEXT("tier1-literals"), TEXT("Tier 1: every literal kind, and how each prints."), 0 },
		{ TEXT("tier1-arithmetic"), TEXT("Tier 1: + - * / // % ** and their precedence."), 0 },
		{ TEXT("tier1-python-sign-rules"), TEXT("Tier 1: // floors toward negative infinity and % takes the divisor sign."), 0 },
		{ TEXT("tier1-strings"), TEXT("Tier 1: concatenation, repetition, len, and conversion."), 0 },
		{ TEXT("tier1-builtins"), TEXT("Tier 1: print, len, int, float, str, bool."), 0 },
		{ TEXT("tier2-conditional"), TEXT("Tier 2: if, elif, else, and which branch runs."), 0 },
		{ TEXT("tier2-boolean-logic"), TEXT("Tier 2: and, or, not, and short-circuit evaluation."), 0 },
		{ TEXT("tier2-comparison"), TEXT("Tier 2: all six comparison operators."), 0 },
		{ TEXT("tier2-truthiness"), TEXT("Tier 2: what counts as false."), 0 },
		{ TEXT("tier3-while"), TEXT("Tier 3: while, and the iteration steps the scrubber collapses."), 0 },
		{ TEXT("tier3-for-range"), TEXT("Tier 3: for over range in its one, two and three argument forms."), 0 },
		{ TEXT("tier3-lists"), TEXT("Tier 3: list literals, indexing including negative, len, in, append."), 0 },
		{ TEXT("tier3-list-is-a-reference"), TEXT("Tier 3: two names for one list, the parallel-assignment misconception."), 0 },
		{ TEXT("tier3-accumulator"), TEXT("Tier 3: augmented assignment against the long form, and nested loops."), 0 },
		{ TEXT("tier3-break-continue"), TEXT("Tier 3: break and continue, and which one skips what."), 0 },
		{ TEXT("tier3-nested-loop"), TEXT("Tier 3: a nested loop, and break leaving only the inner one."), 0 },
		{ TEXT("tier4-function"), TEXT("Tier 4: def, positional parameters, return, and calling."), 0 },
		{ TEXT("tier4-return-forms"), TEXT("Tier 4: a returned value, a bare return, and running off the end."), 0 },
		{ TEXT("tier4-local-scope"), TEXT("Tier 4: names bound in a function are not visible outside it."), 0 },
		{ TEXT("tier4-recursion"), TEXT("Tier 4: the call stack growing and unwinding. The diagram view is for this."), 0 },
		{ TEXT("tier4-nested-calls"), TEXT("Tier 4: a call inside a call, and one function calling another."), 0 },
		{ TEXT("tier4-return-out-of-loop"), TEXT("Tier 4: return unwinds the loop as well as the block."), 0 },
		{ TEXT("station-effects"), TEXT("Station effect builtins. The evaluator emits, it never applies."), 0 },
		{ TEXT("station-oracle-read"), TEXT("ADR-003: wait is an effect, so the next read sees the advanced world."), 0 },
		{ TEXT("outcome-name-not-defined"), TEXT("A runtime error stops the program and keeps every step before it."), 0 },
		{ TEXT("outcome-type-mismatch"), TEXT("Adding a number to text, reported without jargon."), 0 },
		{ TEXT("outcome-division-by-zero"), TEXT("Division by zero."), 0 },
		{ TEXT("outcome-call-depth"), TEXT("Runaway recursion stops with a message, not a stack overflow."), 4000 },
		{ TEXT("outcome-statement-cap"), TEXT("ADR-021: the design budget, as a diagnosable outcome rather than a crash."), 40 },
	};

	const FFixtureInfo* FindInfo(const FString& Name)
	{
		for (const FFixtureInfo& F : Manifest) { if (Name == F.Name) { return &F; } }
		return nullptr;
	}

	/** A deterministic facility: the cold store warms by one degree a tick from a base of 4. */
	FGitsValue StationOracle(const FGitsWorldState& World, const FGitsQuery& Query)
	{
		const FGitsWorldValue* Clock = World.Find(GitsWorld::ClockKey);
		const int64 Ticks = (Clock && Clock->Kind == FGitsWorldValue::EKind::Number) ? (int64)Clock->Number : 0;
		return FGitsValue::MakeInt(Query.Id == TEXT("cold_store") ? 4 + Ticks : Ticks);
	}

	FString FixturesDir() { return FPaths::Combine(FPaths::ProjectDir(), TEXT("Fixtures")); }
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FGitsGoldenTest, "GhostInTheStack.Interpreter.Golden", GitsTest::TestFlags)

void FGitsGoldenTest::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	TArray<FString> Files;
	IFileManager::Get().FindFiles(Files, *FPaths::Combine(FixturesDir(), TEXT("*.py")), true, false);
	Files.Sort();
	for (const FString& F : Files)
	{
		const FString Name = FPaths::GetBaseFilename(F);
		OutBeautifiedNames.Add(Name);
		OutTestCommands.Add(Name);
	}
}

bool FGitsGoldenTest::RunTest(const FString& Name)
{
	const FFixtureInfo* Info = FindInfo(Name);
	if (!Info) { AddError(FString::Printf(TEXT("%s has no manifest entry (covers text and cap)"), *Name)); return false; }

	FString Source, Expected;
	if (!FFileHelper::LoadFileToString(Source, *FPaths::Combine(FixturesDir(), Name + TEXT(".py")))) { AddError(TEXT("cannot read source")); return false; }
	if (!FFileHelper::LoadFileToString(Expected, *FPaths::Combine(FixturesDir(), Name + TEXT(".trace.txt")))) { AddError(TEXT("cannot read expected trace")); return false; }
	Expected.ReplaceInline(TEXT("\r\n"), TEXT("\n"));
	Source.ReplaceInline(TEXT("\r\n"), TEXT("\n"));

	FGitsParseResult Parsed = GitsParser::Parse(Source);
	if (Parsed.Diagnostics.Num() > 0)
	{
		for (const FGitsDiagnostic& D : Parsed.Diagnostics) { AddError(FString::Printf(TEXT("%s does not parse: %s"), *Name, *GitsDiagnostics::Format(D))); }
		return false;
	}
	FGitsRunOptions Options;
	if (Info->Cap > 0) { Options.StatementCap = Info->Cap; }
	const FGitsTrace Trace = GitsEvaluator::Run(Parsed.Program, FGitsWorldState(), FGitsWorldOracle(&StationOracle), Options);

	const FString Actual = FString::Printf(TEXT("-- %s\n-- covers: %s\n\n%s\n"), Info->Name, Info->Covers, *GitsTrace::Serialise(Trace));
	if (Actual == Expected) { return true; }

	TArray<FString> A, E;
	Actual.ParseIntoArrayLines(A, false);
	Expected.ParseIntoArrayLines(E, false);
	int32 Line = 0;
	while (Line < A.Num() && Line < E.Num() && A[Line] == E[Line]) { ++Line; }
	AddError(FString::Printf(TEXT("%s: trace differs at line %d\n  expected: %s\n  actual:   %s"), *Name, Line + 1,
		E.IsValidIndex(Line) ? *E[Line] : TEXT("<end>"), A.IsValidIndex(Line) ? *A[Line] : TEXT("<end>")));
	const FString OutDir = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Tests"), TEXT("Golden"));
	IFileManager::Get().MakeDirectory(*OutDir, true);
	FFileHelper::SaveStringToFile(Actual, *FPaths::Combine(OutDir, Name + TEXT(".actual.txt")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	return false;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGitsGoldenCoverageTest, "GhostInTheStack.Interpreter.GoldenCoverage", GitsTest::TestFlags)
bool FGitsGoldenCoverageTest::RunTest(const FString&)
{
	TArray<FString> Files;
	IFileManager::Get().FindFiles(Files, *FPaths::Combine(FixturesDir(), TEXT("*.py")), true, false);
	TestEqual(TEXT("thirty fixtures present"), Files.Num(), 30);
	for (const FString& F : Files)
	{
		TestNotNull(*FString::Printf(TEXT("manifest entry for %s"), *F), FindInfo(FPaths::GetBaseFilename(F)));
	}
	TestEqual(TEXT("manifest size"), (int32)UE_ARRAY_COUNT(Manifest), 30);
	return true;
}

#endif
