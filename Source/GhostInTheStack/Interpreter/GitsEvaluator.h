// Ghost in the Stack — evaluator.
//
// Runs a parsed program and records one step per AST node evaluation (ADR-001), with
// full frame snapshots, output, effects and oracle reads on each step. Takes a
// WorldOracle and records every read so replay never consults the world (ADR-003).
// Enforces two caps in two units (ADR-021): 50,000 raw steps as a safety net and a
// per-level budget of statement executions. Never throws: a runtime error becomes an
// outcome, with every step before it kept.
#pragma once

#include "CoreMinimal.h"
#include "GitsAst.h"
#include "GitsParser.h"

struct FGitsRunOptions
{
	/** Per-level design budget, counted in statement executions (ADR-021). */
	int32 StatementCap = GitsLimits::DefaultStatementCap;
	int32 Seed = 0;
	/** When set, only these station functions exist. Empty means all of them. */
	bool bRestrictBuiltins = false;
	TSet<FString> Builtins;
};

namespace GitsEvaluator
{
	/** Runs to completion, or to whichever cap or error fires first. */
	FGitsTrace Run(const FGitsProgram& Program, const FGitsWorldState& InitialWorld, FGitsWorldOracle Oracle, const FGitsRunOptions& Options = FGitsRunOptions());

	/** Parses and runs. A parse diagnostic becomes an error outcome with no steps. */
	FGitsTrace RunSource(const FString& Source, const FGitsWorldState& InitialWorld, FGitsWorldOracle Oracle, const FGitsRunOptions& Options = FGitsRunOptions(), EGitsTier Tier = EGitsTier::Four);

	/** The names of the station builtins, for level validation. */
	const TArray<FString>& StationBuiltinNames();
}
