#include "TinyverseAudioLibrary.h"

#include "FMODBlueprintStatics.h"
#include "FMODEvent.h"
#include "Tinyverse.h"

bool UTinyverseAudioLibrary::PlayUIEventByPath(
	UObject* WorldContextObject,
	const FString& EventPath)
{
	if (!IsValid(WorldContextObject) || EventPath.IsEmpty())
	{
		return false;
	}

	UFMODEvent* Event = UFMODBlueprintStatics::FindEventByName(EventPath);
	if (!IsValid(Event))
	{
#if !UE_BUILD_SHIPPING
		UE_LOG(
			LogTinyverse,
			Warning,
			TEXT("FMOD UI event not found: %s"),
			*EventPath);
#endif
		return false;
	}

	UFMODBlueprintStatics::PlayEvent2D(
		WorldContextObject,
		Event,
		true);

	return true;
}
