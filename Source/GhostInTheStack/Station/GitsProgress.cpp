#include "GitsProgress.h"
#include "Engine/World.h"
#include "Misc/PackageName.h"

FString UGitsProgressSubsystem::MapNameOf(const UWorld* World)
{
	if (!World) { return FString(); }
	// PIE worlds are named UEDPIE_0_L_Sector1_Airlock; the sector is the map's short name.
	FString Name = World->GetOutermost()->GetName();
	Name = FPackageName::GetShortName(Name);
	return Normalise(Name);
}

FString UGitsProgressSubsystem::Normalise(const FString& MapName)
{
	FString Name = FPackageName::GetShortName(MapName);
	const int32 Pie = Name.Find(TEXT("UEDPIE_"));
	if (Pie == 0)
	{
		// UEDPIE_<n>_<map>
		int32 Underscore = INDEX_NONE;
		if (Name.RightChop(7).FindChar(TEXT('_'), Underscore)) { Name = Name.RightChop(7 + Underscore + 1); }
	}
	return Name;
}
