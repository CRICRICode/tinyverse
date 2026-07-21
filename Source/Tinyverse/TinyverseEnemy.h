// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TinyverseEnemy.generated.h"

class UBoxComponent;
class USceneComponent;
class UPrimitiveComponent;
struct FHitResult;

UCLASS(Abstract, Blueprintable)
class TINYVERSE_API ATinyverseEnemy : public ACharacter
{
	GENERATED_BODY()
	
public:	
	ATinyverseEnemy();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	UBoxComponent* StompTrigger = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="State")
	bool bIsDefeated = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stomp", meta=(ClampMin="0.0"))
	float StompBounceSpeed = 600.0f;
	
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Gravity")
	TObjectPtr<AActor> GravityPlanet = nullptr;
	
	virtual void Tick(float DeltaTime) override;
	
	void UpdateGravity();
	
	UFUNCTION()
	void HandleStompTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OverlappedActor,
	                                    UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex,
	                                    bool bFromSweep, const FHitResult& SweepResult);
};


