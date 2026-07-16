#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TinyverseAnimationLibrary.generated.h"

UCLASS()
class TINYVERSE_API UTinyverseAnimationLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintPure, Category="Animation|Planet", meta=(DisplayName="Calculate Planet Direction"))
	static float CalculatePlanetDirection(const FVector& Velocity, const FRotator& BaseRotation);
};
