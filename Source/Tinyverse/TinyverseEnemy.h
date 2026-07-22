// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TinyverseEnemy.generated.h"

class UCapsuleComponent;
class UPrimitiveComponent;
struct FHitResult;

UCLASS(Abstract, Blueprintable)
class TINYVERSE_API ATinyverseEnemy : public ACharacter
{
	GENERATED_BODY()
	
public:	
	ATinyverseEnemy();
	
	UFUNCTION(BlueprintCallable, Category="AI|Patrol")
	void MoveAlongPlanet(float DeltaTime);
	
	UFUNCTION(BlueprintCallable, Category="AI|Patrol")
	void ReversePatrolDirection();
	

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	UCapsuleComponent* StompTrigger = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="State")
	bool bIsDefeated = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stomp", meta=(ClampMin="0.0"))
	float StompBounceSpeed = 600.0f;
	
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Gravity")
	TObjectPtr<AActor> GravityPlanet = nullptr;
	
		
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI|Patrol", meta=(ClampMin="0.0"))
	float PatrolSpeed=150.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI|Patrol", meta=(ClampMin="0.1"))
	float PatrolTurnSpeed = 6.0f;
	
	FVector PatrolDirection = FVector::ZeroVector;
	
	void BeginPlay() override;
	void Tick(float DeltaTime) override;
	
	void UpdateGravity();
	
	UFUNCTION()
	void HandleStompTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OverlappedActor,
	                                    UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex,
	                                    bool bFromSweep, const FHitResult& SweepResult);
};


