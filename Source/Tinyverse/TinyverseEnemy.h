// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TinyverseEnemy.generated.h"

class UCapsuleComponent;
class USphereComponent;
class UPrimitiveComponent;
struct FHitResult;
class UTinyverseHealthComponent;

UENUM(BlueprintType)
enum class ETinyverseEnemyChargePhase : uint8
{
	Inactive,
	Windup,
	Charging,
	Recovery
};

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
	
	UFUNCTION(BlueprintCallable, Category="AI|Charge", meta=(ReturnDisplayName = "Started"))
	bool StartCharge(AActor* TargetActor);
	
	UFUNCTION(BlueprintCallable, Category="AI|Charge", meta=(ReturnDisplayName="Finished"))
	bool UpdateCharge(float DeltaTime);

	UFUNCTION(BlueprintCallable, Category="AI|Charge")
	void CancelCharge();

	UFUNCTION(BlueprintPure, Category="AI|Charge")
	float GetDetectionRadius() const;

	UFUNCTION(BlueprintPure, Category="AI|Charge")
	float GetLoseTargetRadius() const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	UCapsuleComponent* StompTrigger = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	USphereComponent* DamageTrigger = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	UTinyverseHealthComponent* EnemyHealthComponent = nullptr;
	
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
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AI|Charge", meta=(ClampMin="0.0"))
	float DetectionRadius = 800.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AI|Charge", meta=(ClampMin="0.0"))
	float LoseTargetRadius = 1000.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AI|Charge", meta=(ClampMin="0.0"))
	float ChargeWindupDuration = 0.35f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AI|Charge", meta=(ClampMin="0.0"))
	float ChargeSpeed = 500.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AI|Charge", meta=(ClampMin="0.0"))
	float ChargeDuration = 0.8f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AI|Charge", meta=(ClampMin="0.0"))
	float ChargeRecoveryDuration = 0.6f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AI|Charge", meta=(ClampMin="0.0"))
	float ChargeDamage = 1.0f;
	
	FVector PatrolDirection = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat|Damage", meta=(ClampMin="0.0"))
	float ContactDamage = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat|Stomp", meta=(ClampMin="0.0"))
	float StompDamage = 1.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat|Knockback", meta=(ClampMin="0.0"))
	float ContactKnockbackSpeed = 450.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat|Knockback", meta=(ClampMin="0.0"))
	float ContactKnockbackLiftSpeed = 250.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="AI|Charge")
	ETinyverseEnemyChargePhase ChargePhase = ETinyverseEnemyChargePhase::Inactive;
	
	UPROPERTY(Transient)
	TObjectPtr<AActor> ChargeTarget = nullptr;
	
	float ChargePhaseElapsedTime = 0.0f;
	FVector ChargeDirection = FVector::ZeroVector;
	
	void BeginPlay() override;
	void Tick(float DeltaTime) override;
	void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	void UpdateGravity();
	FVector GetLocalUp() const;
	FVector GetTangentDirectionTo(const AActor* TargetActor) const;
	void EnterChargePhase(ETinyverseEnemyChargePhase NewPhase);
	void UpdateChargeFacing(float DeltaTime);
	void UpdateChargeMovement();
	
	UFUNCTION()
	void HandleStompTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OverlappedActor,
	                                    UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex,
	                                    bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleDamageTriggerBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);
	
	UFUNCTION()
	void HandleDeath(
		UTinyverseHealthComponent* DeadHealthComponent,
		AActor* DamageCauser);
};


