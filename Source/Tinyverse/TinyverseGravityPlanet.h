#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TinyverseGravityPlanet.generated.h"

class UPrimitiveComponent;
class USceneComponent;
class USphereComponent;
struct FHitResult;

UCLASS(Blueprintable)
class TINYVERSE_API ATinyverseGravityPlanet : public AActor
{
	GENERATED_BODY()

public:
	ATinyverseGravityPlanet();

	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintPure, Category="Gravity")
	FVector GetGravityDirectionAtLocation(const FVector& WorldLocation) const;

	UFUNCTION(BlueprintPure, Category="Gravity", meta=(ToolTip="Returns the gravity acceleration at this location in cm/s^2, including optional distance falloff."))
	float GetGravityStrengthAtLocation(const FVector& WorldLocation) const;

	UFUNCTION(BlueprintPure, Category="Gravity", meta=(ToolTip="Returns distance from the surface divided by local gravity strength. The character selects the planet with the lowest score."))
	float GetGravitySelectionScoreAtLocation(const FVector& WorldLocation) const;

	UFUNCTION(BlueprintPure, Category="Gravity")
	float GetPlanetRadius() const;

	UFUNCTION(BlueprintPure, Category="Gravity")
	float GetInfluenceRadius() const;

	UFUNCTION(BlueprintPure, Category="Gravity")
	bool IsLocationWithinInfluence(const FVector& WorldLocation) const;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USphereComponent> GravityVolume;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gravity|Dimensions", meta=(ToolTip="Automatically derives the physical planet radius from its primitive components."))
	bool bAutoDetectPlanetRadius = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gravity|Dimensions", meta=(EditCondition="!bAutoDetectPlanetRadius", EditConditionHides, ClampMin="1.0", Units="cm", ToolTip="Physical planet radius used when automatic detection is disabled."))
	float PlanetRadius = 1000.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Gravity|Dimensions", meta=(Units="cm"))
	float DetectedPlanetRadius = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gravity", meta=(ClampMin="0.0", Units="cm/s^2", ToolTip="Gravity acceleration at the planet surface. This drives both character acceleration and planet selection."))
	float SurfaceGravity = 980.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gravity|Influence", meta=(ClampMin="1.0", Units="cm", ToolTip="Maximum distance from the planet center at which this planet can affect characters."))
	float InfluenceRadius = 3000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gravity|Influence", meta=(ToolTip="When enabled, gravity becomes weaker with distance from the surface. When disabled, Surface Gravity remains constant throughout the influence volume."))
	bool bUseGravityFalloff = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gravity|Influence", meta=(EditCondition="bUseGravityFalloff", EditConditionHides, ClampMin="0.0", ToolTip="Controls how quickly gravity weakens with distance. 0 keeps it constant; 2 approximates an inverse-square falloff."))
	float GravityFalloffExponent = 0.0f;

private:
	float CalculatePlanetRadiusFromComponents() const;

	UFUNCTION()
	void HandleGravityVolumeBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void HandleGravityVolumeEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex);
};
