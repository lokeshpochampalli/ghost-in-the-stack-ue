// Ghost in the Stack — trace access, replay, diff and golden serialisation.
//
// Consumers reach the trace through these accessors and never index the raw array
// (ADR-009). diff compares observable state only (ADR-002). serialise produces the
// compact one-line-per-step text the golden fixtures hold (ADR-015).
#pragma once

#include "CoreMinimal.h"
#include "GitsTypes.h"

enum class EGitsDivergenceKind : uint8 { Bindings, Output, Effects, Length, Outcome };

struct FGitsDiffResult
{
	bool bDiverged = false;
	/** Which statement boundary, counting from zero, first differed. -1 when none. */
	int32 AtBoundary = -1;
	int32 AStepIndex = -1;
	int32 BStepIndex = -1;
	TArray<EGitsDivergenceKind> Kinds;
	/** Human-readable lines describing the change. */
	TArray<FString> Summary;
};

namespace GitsTrace
{
	const FGitsStep& At(const FGitsTrace& Trace, int32 Index);
	int32 Length(const FGitsTrace& Trace);
	/** Indices of the statement-boundary steps, in order. */
	TArray<int32> StatementBoundaries(const FGitsTrace& Trace);
	/** Everything printed up to and including step Index. */
	TArray<FString> OutputAt(const FGitsTrace& Trace, int32 Index);
	/** The world at step Index, by folding recorded effects. Never consults an oracle. */
	FGitsWorldState WorldAt(const FGitsTrace& Trace, int32 Index);
	TArray<FGitsEffect> AllEffects(const FGitsTrace& Trace);
	/** The innermost frame's bindings at step Index. */
	TArray<TPair<FString, FGitsValue>> BindingsAt(const FGitsTrace& Trace, int32 Index);

	/** An oracle that answers from the recorded reads, in order. Replay must reproduce the trace. */
	FGitsWorldOracle ReplayOracle(const FGitsTrace& Trace);

	/** Where two runs first behaved differently, by observable state (ADR-002). */
	FGitsDiffResult Diff(const FGitsTrace& A, const FGitsTrace& B);

	/** The golden-trace text: one line per step, then outcome, counts and printed output. */
	FString Serialise(const FGitsTrace& Trace);
	FString DescribeOutcome(const FGitsTrace& Trace);
	FString DescribeEffect(const FGitsEffect& Effect);
	/** JSON.stringify-style quoting, used for printed lines and log messages. */
	FString QuoteJson(const FString& S);
}
