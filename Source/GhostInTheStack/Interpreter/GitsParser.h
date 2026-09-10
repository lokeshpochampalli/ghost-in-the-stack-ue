// Ghost in the Stack — parser with the recognition pass.
//
// Recursive descent over the token stream, gated by tier. Excluded syntax is recognised
// and named (ADR-007) rather than rejected with a generic message. The parser is total:
// it never throws and always returns a program, with every problem in Diagnostics
// sorted by position.
#pragma once

#include "CoreMinimal.h"
#include "GitsAst.h"

struct FGitsParseOptions
{
	/** The level's tier. Constructs above it are refused in the not-yet register. */
	EGitsTier Tier = EGitsTier::Four;
};

struct FGitsParseResult
{
	FGitsProgram Program;
	TArray<FGitsDiagnostic> Diagnostics;
};

namespace GitsParser
{
	FGitsParseResult Parse(const FString& Source, const FGitsParseOptions& Options = FGitsParseOptions());
	/** Diagnostic codes in source order, for tests. */
	TArray<FString> CodeNames(const FGitsParseResult& Result);
}
