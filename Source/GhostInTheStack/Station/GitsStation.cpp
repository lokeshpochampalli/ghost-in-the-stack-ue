#include "GitsStation.h"
#include "GitsStationSubsystem.h"
#include "GitsTerminal.h"
#include "GitsScript.h"
#include "Companion/GitsTags.h"
#include "Engine/World.h"
#include "EngineUtils.h"

DEFINE_LOG_CATEGORY_STATIC(LogGitsStation, Display, All);

void AGitsStation::BeginPlay()
{
	Super::BeginPlay();
	if (UGitsStationSubsystem* S = GetWorld()->GetSubsystem<UGitsStationSubsystem>()) { S->InitialisePower(PowerBudget, ReserveRestore); }
	// The validator's power rules, on every script this level can run (PHASES-3D Phase 5).
	for (TActorIterator<AGitsTerminal> It(GetWorld()); It; ++It)
	{
		if (!It->Script) { continue; }
		for (const FString& Problem : GitsTags::ValidatePower(It->Script, PowerBudget))
		{
			UE_LOG(LogGitsStation, Error, TEXT("%s: %s"), *It->Script->GetName(), *Problem);
		}
	}
}

AGitsStation::AGitsStation()
{
	PrimaryActorTick.bCanEverTick = false;
	// A sensible first level: one airlock sensor holding.
	FGitsSensorSpec Airlock;
	Airlock.Id = TEXT("airlock");
	Airlock.Value = 4.f;
	Sensors.Add(Airlock);
}

FGitsWorldState AGitsStation::InitialWorld() const
{
	FGitsWorldState World;
	for (const auto& P : InitialSwitches) { World.Add(P.Key, FGitsWorldValue::MakeBool(P.Value)); }
	for (const auto& P : InitialLevels) { World.Add(P.Key, FGitsWorldValue::MakeNumber(P.Value)); }
	return World;
}

FGitsValue AGitsStation::Read(const FGitsWorldState& World, const FGitsQuery& Query) const
{
	const FGitsWorldValue* Clock = World.Find(GitsWorld::ClockKey);
	const double Ticks = (Clock && Clock->Kind == FGitsWorldValue::EKind::Number) ? Clock->Number : 0.0;
	for (const FGitsSensorSpec& S : Sensors)
	{
		if (S.Id == Query.Id)
		{
			const double V = (double)S.Value + (double)S.DriftPerTick * Ticks;
			// Whole readings stay whole numbers, as a gauge would show them.
			if (FMath::IsNearlyEqual(V, FMath::RoundToDouble(V))) { return FGitsValue::MakeInt((int64)FMath::RoundToDouble(V)); }
			return FGitsValue::MakeFloat(V);
		}
	}
	return FGitsValue::MakeInt(0);
}
