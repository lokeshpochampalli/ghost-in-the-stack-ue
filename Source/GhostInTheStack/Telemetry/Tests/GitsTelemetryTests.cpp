// The consent gate, the export bundle, the PII tripwire, and the instruments.
#include "Interpreter/Tests/GitsTestUtil.h"
#include "Telemetry/GitsTelemetry.h"
#include "Engine/GameInstance.h"
#include "Telemetry/GitsInstruments.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"

#if WITH_DEV_AUTOMATION_TESTS

using namespace GitsTest;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGitsConsentGate, "GhostInTheStack.Telemetry.ConsentGate", GitsTest::TestFlags)
bool FGitsConsentGate::RunTest(const FString&)
{
	// The hard requirement of the ethics application: no event is written before consent is
	// recorded. Enforced in the log, so adding a second entry point cannot route around it.
	// The subsystem's ClassWithin is the game instance; the test gives it one.
	UGameInstance* GI = NewObject<UGameInstance>(GetTransientPackage());
	UGitsTelemetrySubsystem* T = NewObject<UGitsTelemetrySubsystem>(GI);
	T->InitialiseForTest(TEXT("copper-lantern-47"), TEXT("session-test"));
	AddExpectedError(TEXT("refused to record"), EAutomationExpectedErrorFlags::Contains, 0);
	TestFalse(TEXT("refuses a scrub before consent"), T->Record(TEXT("scrub")));
	for (const FString& Type : UGitsTelemetrySubsystem::EventTypes())
	{
		TestFalse(*FString::Printf(TEXT("refuses %s before consent"), *Type), T->Record(Type));
	}
	TestFalse(TEXT("refuses the edit helper too"), T->Edit(1, TEXT("a"), TEXT("b")));
	// Throwing is not enough: nothing may have reached the store.
	TestEqual(TEXT("store empty after refusals"), T->GetEvents().Num(), 0);
	FString Path, Error;
	TestFalse(TEXT("no export without consent"), T->Export(Path, Error));
	TestTrue(TEXT("export says why"), Error.Contains(TEXT("No consent")));

	T->RecordConsent(false, TEXT("none"), 12345u);
	TestTrue(TEXT("consent on file"), T->HasConsent());
	TestEqual(TEXT("session_start is the first event"), T->GetEvents().Num(), 1);
	TestEqual(TEXT("its type"), T->GetEvents()[0].Type, FString(TEXT("session_start")));
	TestEqual(TEXT("seed recorded"), T->GetEvents()[0].Payload->GetNumberField(TEXT("seed")), 12345.0);
	TestTrue(TEXT("records after consent"), T->Record(TEXT("scrub")));
	TestFalse(TEXT("refuses a type outside the taxonomy"), T->Record(TEXT("prediction_committed")));
	TestEqual(TEXT("sequence is monotonic"), T->GetEvents()[1].Sequence, 1);

	// Without code capture, edits are hashed and say so.
	TestTrue(TEXT("edit records"), T->Edit(6, TEXT("target = 5"), TEXT("target = 4")));
	const FGitsTelemetryEvent& E = T->GetEvents().Last();
	TestTrue(TEXT("hashed"), E.Payload->GetBoolField(TEXT("hashed")));
	TestNotEqual(TEXT("before is not the text"), E.Payload->GetStringField(TEXT("before")), FString(TEXT("target = 5")));
	TestEqual(TEXT("hash is eight hex digits"), E.Payload->GetStringField(TEXT("before")).Len(), 8);

	// The bundle reads back as format version 1 with the events inside.
	const FString Json = T->BuildBundleJson();
	TSharedPtr<FJsonObject> Root;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
	TestTrue(TEXT("bundle parses"), FJsonSerializer::Deserialize(Reader, Root) && Root.IsValid());
	if (Root.IsValid())
	{
		TestEqual(TEXT("format version"), Root->GetNumberField(TEXT("formatVersion")), 1.0);
		TestEqual(TEXT("participant"), Root->GetStringField(TEXT("participantCode")), FString(TEXT("copper-lantern-47")));
		TestEqual(TEXT("three events"), Root->GetArrayField(TEXT("events")).Num(), 3);
		TestTrue(TEXT("consent version"), Root->GetObjectField(TEXT("consent"))->GetStringField(TEXT("version")) == UGitsTelemetrySubsystem::ConsentVersion);
	}
	TestEqual(TEXT("bundle is clean"), UGitsTelemetrySubsystem::FindPii(Json).Num(), 0);

	// Erasure forgets everything, consent included, and the gate closes again.
	T->Erase();
	TestFalse(TEXT("no consent after erasure"), T->HasConsent());
	TestEqual(TEXT("nothing held"), T->GetEvents().Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGitsPiiTripwire, "GhostInTheStack.Telemetry.PiiTripwire", GitsTest::TestFlags)
bool FGitsPiiTripwire::RunTest(const FString&)
{
	TestEqual(TEXT("email"), UGitsTelemetrySubsystem::FindPii(TEXT("{\"note\":\"write to someone@example.org\"}")).Num(), 1);
	TestEqual(TEXT("ip"), UGitsTelemetrySubsystem::FindPii(TEXT("{\"from\":\"192.168.1.20\"}")).Num(), 1);
	TestEqual(TEXT("name field"), UGitsTelemetrySubsystem::FindPii(TEXT("{\"firstName\": \"x\"}")).Num(), 1);
	TestEqual(TEXT("clean"), UGitsTelemetrySubsystem::FindPii(TEXT("{\"participantCode\":\"copper-lantern-47\",\"score\":3.5}")).Num(), 0);
	TestEqual(TEXT("codes look right"), UGitsTelemetrySubsystem::GenerateParticipantCode(7).Len() > 8, true);
	TestTrue(TEXT("code shape"), UGitsTelemetrySubsystem::GenerateParticipantCode(7).Contains(TEXT("-")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGitsTracingItems, "GhostInTheStack.Telemetry.Instruments.TracingItems", GitsTest::TestFlags)
bool FGitsTracingItems::RunTest(const FString&)
{
	// An outcome-measure item that teaches a falsehood would invalidate the headline result and
	// be invisible to any schema check: every item runs through our own interpreter.
	for (TCHAR Form : { TEXT('A'), TEXT('B') })
	{
		const FGitsInstrument& I = GitsInstruments::TracingTest(Form);
		TestEqual(TEXT("eleven items"), I.QuizItems.Num(), 11);
		for (const FGitsQuizItem& Item : I.QuizItems)
		{
			const FGitsQuizOption* Correct = Item.Options.FindByPredicate([&Item](const FGitsQuizOption& O) { return O.Id == Item.CorrectOptionId; });
			TestNotNull(TEXT("has a correct option"), Correct);
			if (!Correct) { continue; }
			TestTrue(TEXT("correct carries no tag"), Correct->Misconception.IsEmpty());
			for (const FGitsQuizOption& O : Item.Options) { if (O.Id != Correct->Id) { TestFalse(*FString::Printf(TEXT("%s distractor tagged"), *Item.Id), O.Misconception.IsEmpty()); } }
			FGitsTrace T = Exec(Item.Source);
			if (Item.Expected == TEXT("output"))
			{
				TestEqual(*FString::Printf(TEXT("%s completes"), *Item.Id), (int32)T.Outcome.Kind, (int32)EGitsOutcomeKind::Completed);
				TestEqual(*FString::Printf(TEXT("%s prints its correct option"), *Item.Id), FString::Join(T.Output, TEXT("\n")), Correct->Text);
			}
			else
			{
				TestEqual(*FString::Printf(TEXT("%s stops with an error"), *Item.Id), (int32)T.Outcome.Kind, (int32)EGitsOutcomeKind::Error);
			}
		}
	}
	// Forms alternate per participant and swap for the post-test; presented order is deterministic.
	const TCHAR Pre = GitsInstruments::FormFor(TEXT("copper-lantern-47"), TEXT("pre"));
	const TCHAR Post = GitsInstruments::FormFor(TEXT("copper-lantern-47"), TEXT("post"));
	TestTrue(TEXT("pre and post differ"), Pre != Post);
	const FGitsQuizItem& First = GitsInstruments::TracingTest(TEXT('A')).QuizItems[0];
	const TArray<FGitsQuizOption> O1 = GitsInstruments::PresentedOptions(First, TEXT("copper-lantern-47"));
	const TArray<FGitsQuizOption> O2 = GitsInstruments::PresentedOptions(First, TEXT("copper-lantern-47"));
	TestEqual(TEXT("same order twice"), O1[0].Id, O2[0].Id);
	TestEqual(TEXT("all options present"), O1.Num(), First.Options.Num());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGitsLikertScoring, "GhostInTheStack.Telemetry.Instruments.Scoring", GitsTest::TestFlags)
bool FGitsLikertScoring::RunTest(const FString&)
{
	const FGitsInstrument& Sus = GitsInstruments::Sus();
	TestEqual(TEXT("ten SUS items"), Sus.Items.Num(), 10);
	FGitsResponses R;
	// Strongly agree with the positive items and strongly disagree with the negative ones: 100.
	for (const FGitsLikertItem& I : Sus.Items) { R.Numbers.Add(I.Id, I.bReversed ? 1.0 : 5.0); }
	TSharedPtr<FJsonObject> S = Sus.Score(R);
	TestEqual(TEXT("SUS best case"), S->GetNumberField(TEXT("overall")), 100.0);
	TestTrue(TEXT("complete"), S->GetBoolField(TEXT("complete")));
	// All fives, positive and negative alike: 50.
	for (const FGitsLikertItem& I : Sus.Items) { R.Numbers.Add(I.Id, 5.0); }
	TestEqual(TEXT("SUS straight-lined fives"), Sus.Score(R)->GetNumberField(TEXT("overall")), 50.0);
	// A skipped item: no SUS score, and the score says so.
	R.Numbers.Remove(TEXT("sus-3"));
	S = Sus.Score(R);
	TestTrue(TEXT("nine items is not a SUS score"), !S->HasTypedField<EJson::Number>(TEXT("overall")));
	TestEqual(TEXT("answered nine"), S->GetNumberField(TEXT("answered")), 9.0);
	TestFalse(TEXT("not complete"), S->GetBoolField(TEXT("complete")));

	const FGitsInstrument& Imi = GitsInstruments::Imi();
	TestEqual(TEXT("22 IMI items"), Imi.Items.Num(), 22);
	FGitsResponses R2;
	for (const FGitsLikertItem& I : Imi.Items) { R2.Numbers.Add(I.Id, I.Dimension == TEXT("pressure/tension") ? (I.bReversed ? 7.0 : 1.0) : (I.bReversed ? 2.0 : 6.0)); }
	TSharedPtr<FJsonObject> S2 = Imi.Score(R2);
	const TArray<TSharedPtr<FJsonValue>>& Dims = S2->GetArrayField(TEXT("dimensions"));
	TestEqual(TEXT("four subscales"), Dims.Num(), 4);
	TestEqual(TEXT("enjoyment mean"), Dims[0]->AsObject()->GetNumberField(TEXT("mean")), 6.0);
	TestEqual(TEXT("pressure mean, flipped, is 1"), Dims[3]->AsObject()->GetNumberField(TEXT("mean")), 1.0);
	TestTrue(TEXT("no overall for IMI"), !S2->HasTypedField<EJson::Number>(TEXT("overall")));

	const FGitsInstrument& Meega = GitsInstruments::MeegaPlus();
	TestEqual(TEXT("33 MEEGA+ items"), Meega.Items.Num(), 33);
	TestEqual(TEXT("thirteen dimensions"), Meega.Dimensions.Num(), 13);
	FGitsResponses R3;
	for (const FGitsLikertItem& I : Meega.Items) { R3.Numbers.Add(I.Id, I.bReversed ? -2.0 : 2.0); }
	TestEqual(TEXT("MEEGA+ median of all agree"), Meega.Score(R3)->GetNumberField(TEXT("overall")), 2.0);
	return true;
}

#endif
