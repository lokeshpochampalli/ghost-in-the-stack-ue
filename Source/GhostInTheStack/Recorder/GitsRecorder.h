// Ghost in the Stack — the recorder: trace step index <-> world time, and loop structure.
//
// Playback runs one statement boundary per beat, so world time is beats. Rewind moves the
// clock backwards and asks which step the clock is on; nothing is simulated in reverse
// (CLAUDE.md). Loop contexts are computed the way the reference ribbon collapses loops
// (ADR-008, scrubberModel.ts): a loop owns every boundary whose span sits inside its own.
#pragma once

#include "CoreMinimal.h"
#include "Interpreter/GitsTypes.h"

/** Which loop a step is inside, and how far through it. */
struct FGitsLoopContext
{
	FString NodeId;
	/** "for i" or "while x < 3": the header without its outcome. */
	FString Header;
	int32 Line = 0;
	/** 1-based iteration this step belongs to; 0 when the loop never ran or has ended. */
	int32 Iteration = 0;
	/** How many iterations the loop ran in total. */
	int32 Total = 0;
	/** True on the boundary that found the loop finished. */
	bool bEnded = false;

	FString Describe() const;
};

class GHOSTINTHESTACK_API FGitsRecorder
{
public:
	void Build(const FGitsTrace& Trace, float StatementsPerSecond);
	void Clear();
	bool IsBuilt() const { return Bounds.Num() > 0; }

	/** Statement boundaries, in order: one beat each. */
	const TArray<int32>& Boundaries() const { return Bounds; }
	int32 NumBeats() const { return Bounds.Num(); }
	float BeatInterval() const { return Interval; }
	/** World time at which the run is over: one beat after the last boundary lands. */
	float TotalTime() const { return (Bounds.Num() + 1) * Interval; }

	/** The step the play head is on at world time T: -1 before the first beat, the last step at or after TotalTime. */
	int32 StepAtTime(float T) const;
	/** World time at which the beat carrying StepIndex lands. */
	float TimeOfStep(int32 StepIndex) const;
	/** 0-based beat of the boundary that owns StepIndex, or -1. */
	int32 BeatOfStep(int32 StepIndex) const;

	/** Loops enclosing StepIndex, outermost first. Empty outside any loop. */
	TArray<FGitsLoopContext> LoopsAt(int32 StepIndex) const;

private:
	TArray<int32> Bounds;
	TArray<int32> StepToBeat;
	/** Per beat: the loop stack, outermost first. */
	TArray<TArray<FGitsLoopContext>> BeatLoops;
	int32 LastStep = -1;
	float Interval = 0.4f;
};
