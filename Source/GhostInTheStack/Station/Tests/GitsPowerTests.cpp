// The power stages and the validator's power rules.
#include "Interpreter/Tests/GitsTestUtil.h"
#include "Station/GitsPower.h"
#include "Station/GitsScript.h"
#include "Companion/GitsTags.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGitsPowerStages, "GhostInTheStack.Station.Power.Stages", GitsTest::TestFlags)
bool FGitsPowerStages::RunTest(const FString&)
{
	using namespace GitsPower;
	TestEqual(TEXT("full"), (int32)StageFor(40, 40), (int32)EStage::Nominal);
	TestEqual(TEXT("just above half"), (int32)StageFor(21, 40), (int32)EStage::Nominal);
	TestEqual(TEXT("half is low"), (int32)StageFor(20, 40), (int32)EStage::Low);
	TestEqual(TEXT("just above a quarter"), (int32)StageFor(11, 40), (int32)EStage::Low);
	TestEqual(TEXT("a quarter is critical"), (int32)StageFor(10, 40), (int32)EStage::Critical);
	TestEqual(TEXT("one left is critical"), (int32)StageFor(1, 40), (int32)EStage::Critical);
	TestEqual(TEXT("zero is out"), (int32)StageFor(0, 40), (int32)EStage::Out);
	TestEqual(TEXT("no budget declared is nominal"), (int32)StageFor(0, 0), (int32)EStage::Nominal);
	TestTrue(TEXT("factors fall"), LightFactor(EStage::Nominal) > LightFactor(EStage::Low) && LightFactor(EStage::Low) > LightFactor(EStage::Critical) && LightFactor(EStage::Critical) > LightFactor(EStage::Out));
	TestTrue(TEXT("out is dark"), LightFactor(EStage::Out) == 0.f);
	TestEqual(TEXT("name"), FString(StageName(EStage::Critical)), FString(TEXT("critical")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGitsPowerValidate, "GhostInTheStack.Station.Power.Validate", GitsTest::TestFlags)
bool FGitsPowerValidate::RunTest(const FString&)
{
	UGitsScript* Script = NewObject<UGitsScript>();
	Script->Source = TEXT("a = 1\n");
	Script->RunCost = 12;
	Script->PredictedRunCost = 4;
	TestEqual(TEXT("valid costs against a budget of 40"), GitsTags::ValidatePower(Script, 40).Num(), 0);
	TestEqual(TEXT("budget below the run cost"), GitsTags::ValidatePower(Script, 10).Num(), 1);
	Script->PredictedRunCost = 12;
	TestEqual(TEXT("predicted must be cheaper"), GitsTags::ValidatePower(Script, 40).Num(), 1);
	Script->PredictedRunCost = 4;
	Script->RunCost = 0;
	TestTrue(TEXT("a run must cost something"), GitsTags::ValidatePower(Script, 40).Num() >= 1);
	Script->RunCost = 12;
	TestTrue(TEXT("the general validator includes the cost rules"), GitsTags::Validate(Script).Num() == 0);
	Script->PredictedRunCost = 20;
	TestTrue(TEXT("general validator catches the discount"), GitsTags::Validate(Script).Num() == 1);
	return true;
}

#endif
