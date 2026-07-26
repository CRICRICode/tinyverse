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

	UFUNCTION(BlueprintPure, Category="Gravity")
	float GetGravityStrengthAtLocation(const FVector& WorldLocation) const;

	UFUNCTION(BlueprintPure, Category="Gravity")
	float GetGravityPriorityAtLocation(const FVector& WorldLocation) const;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gravity|Dimensions")
	bool bAutoDetectPlanetRadius = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gravity|Dimensions", meta=(EditCondition="!bAutoDetectPlanetRadius", EditConditionHides, ClampMin="1.0", Units="cm"))
	float PlanetRadius = 1000.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Gravity|Dimensions", meta=(Units="cm"))
	float DetectedPlanetRadius = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gravity", meta=(ClampMin="0.0", Units="cm/s^2"))
	float SurfaceGravity = 980.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gravity|Influence", meta=(ClampMin="1.0", Units="cm"))
	float InfluenceRadius = 3000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gravity|Influence")
	bool bUseGravityFalloff = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gravity|Influence", meta=(EditCondition="bUseGravityFalloff", EditConditionHides, ClampMin="0.0"))
	float GravityFalloffExponent = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gravity|Influence", meta=(ClampMin="0.0"))
	float GravitySelectionFalloffExponent = 2.0f;

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
