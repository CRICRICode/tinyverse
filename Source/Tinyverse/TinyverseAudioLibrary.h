#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TinyverseAudioLibrary.generated.h"

UCLASS()
class TINYVERSE_API UTinyverseAudioLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Audio|Tinyverse", meta=(WorldContext="WorldContextObject", DisplayName="Play FMOD UI Event", CPP_Default_EventPath="event:/SFX/Menu/Selection"))
	static bool PlayUIEventByPath(
		UObject* WorldContextObject,
		const FString& EventPath);
};
