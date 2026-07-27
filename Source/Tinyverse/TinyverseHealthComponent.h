// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TinyverseHealthComponent.generated.h"

class UTinyverseHealthComponent;
class UDamageType;
class AController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
	FTinyverseHealthChangedSignature,
	UTinyverseHealthComponent*, HealthComponent,
	float, CurrentHealth,
	float, MaxHealth,
	float, HealthDelta);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FTinyverseDeathSignature,
	UTinyverseHealthComponent*, HealthComponent,
	AActor*, DamageCauser);


UCLASS(ClassGroup=(Tinyverse), meta=(BlueprintSpawnableComponent))
class TINYVERSE_API UTinyverseHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UTinyverseHealthComponent();
	
	UPROPERTY(BlueprintAssignable, Category = "Health|Events")
	FTinyverseHealthChangedSignature OnHealthChanged;
	
	UPROPERTY(BlueprintAssignable, Category = "Health|Events")
	FTinyverseDeathSignature OnDeath;
	
	UFUNCTION(BlueprintPure, Category="Health")
	float GetCurrentHealth() const;

	UFUNCTION(BlueprintPure, Category="Health")
	float GetMaxHealth() const;

	UFUNCTION(BlueprintPure, Category="Health")
	bool IsDead() const;

	UFUNCTION(BlueprintPure, Category="Health")
	bool IsInvulnerable() const;

	UFUNCTION(BlueprintCallable, Category="Health")
	float Heal(float Amount);

	UFUNCTION(BlueprintCallable, Category="Health")
	float GrantHealthReward(float Amount = 1.0f);
	
protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health", meta=(ClampMin="1.0"))
	float MaxHealth = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health", meta=(ClampMin="1.0"))
	float MaximumHealthLimit = 5.0f;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Health")
	float CurrentHealth = 0.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health|Damage", meta=(ClampMin="0.0"))
	float InvulnerabilityDuration = 0.75f;
	
	UFUNCTION()
	void HandleOwnerTakeAnyDamage(
		AActor* DamagedActor,
		float Damage,
		const UDamageType* DamageType,
		AController* InstigatedBy,
		AActor* DamageCauser);
	
	double DamageCooldownEndTime = 0.0;
		
};
