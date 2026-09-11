// Ghost in the Stack — the research instruments as data (port of src/instruments, ADR-018).
//
// An instrument is items, a scale, a scoring function and the events it emits; one generic
// panel renders any of them. Items as data means the exact instrument used can be printed into
// the appendix from source. Every item is skippable and every score reports `answered`.
// The published questionnaires' wording is a working draft until a human verifies it.
#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

struct FGitsLikertItem
{
	FString Id;
	FString Text;
	FString Dimension;
	/** Agreement is a bad thing; flip about the scale's midpoint before scoring. */
	bool bReversed = false;
};

struct FGitsScale
{
	FString Id;
	TArray<FString> Labels;
	int32 FirstValue = 1;
	FString SkipLabel = TEXT("Prefer not to answer");
	int32 Low() const { return FirstValue; }
	int32 High() const { return FirstValue + Labels.Num() - 1; }
};

struct FGitsQuizOption
{
	FString Id;
	FString Text;
	/** Empty on the correct option; a misconception tag on every distractor. */
	FString Misconception;
};

struct FGitsQuizItem
{
	FString Id;
	FString PairId;
	TCHAR Form = TEXT('A');
	FString Prompt;
	/** Must parse and run in our own interpreter; the test checks the correct option against it. */
	FString Source;
	/** "output" or "error". */
	FString Expected;
	TArray<FGitsQuizOption> Options;
	FString CorrectOptionId;
	FString Concept;
	int32 Tier = 1;
};

/** Responses: a number for a Likert item, an option id for a quiz item; absent means skipped. */
struct FGitsResponses
{
	TMap<FString, double> Numbers;
	TMap<FString, FString> Options;
};

struct FGitsInstrument
{
	enum class EKind : uint8 { Likert, Quiz };
	enum class EScoring : uint8 { Sus, Imi, Meega, Quiz };
	EKind Kind = EKind::Likert;
	EScoring Scoring = EScoring::Sus;
	FString Id;
	FString Title;
	FString Blurb;
	FString Provenance;
	// Likert
	FGitsScale Scale;
	TArray<FGitsLikertItem> Items;
	TArray<FString> Dimensions;
	// Quiz
	TCHAR Form = TEXT('A');
	TArray<FGitsQuizItem> QuizItems;

	int32 ItemCount() const { return Kind == EKind::Likert ? Items.Num() : QuizItems.Num(); }
	/** The score as the export's self-describing record: the reference's LikertScore or QuizScore. */
	TSharedPtr<FJsonObject> Score(const FGitsResponses& Responses) const;
	/** Number of items answered. */
	int32 Answered(const FGitsResponses& Responses) const;
};

namespace GitsInstruments
{
	const FGitsInstrument& Sus();
	const FGitsInstrument& Imi();
	const FGitsInstrument& MeegaPlus();
	const FGitsInstrument& TracingTest(TCHAR Form);
	/** Which form a participant sits first: from a hash of their code, the other one after. */
	TCHAR FormFor(const FString& ParticipantCode, const FString& Occasion);
	/** The options in the order they are shown: shuffled from hash(code:itemId), recoverable after the fact. */
	TArray<FGitsQuizOption> PresentedOptions(const FGitsQuizItem& Item, const FString& ParticipantCode);
	/** Mean of the values present, or unset. */
	bool Mean(const TArray<double>& Values, double& Out);
	bool Median(const TArray<double>& Values, double& Out);
	/** Reads a Likert response with reverse-keyed items flipped; false when skipped or out of range. */
	bool ReadLikert(const FGitsScale& Scale, const FGitsLikertItem& Item, const FGitsResponses& Responses, double& Out);
}
