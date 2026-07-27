
#include "TinyverseHealthComponent.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"

UTinyverseHealthComponent::UTinyverseHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}


// Called when the game starts
void UTinyverseHealthComponent::BeginPlay()
{
	Super::BeginPlay();
	
	MaximumHealthLimit = FMath::Max(MaximumHealthLimit, 1.0f);
	MaxHealth = FMath::Clamp(MaxHealth, 1.0f, MaximumHealthLimit);
	CurrentHealth = MaxHealth;
	DamageCooldownEndTime = 0.0f;
	
	AActor* Owner = GetOwner();

	if (!IsValid(Owner))
	{
		return;
	}
	
	Owner->OnTakeAnyDamage.AddUniqueDynamic(
		this,
		&UTinyverseHealthComponent::HandleOwnerTakeAnyDamage);
	
	OnHealthChanged.Broadcast(
		this,
		CurrentHealth,
		MaxHealth,
		0.0f);	
}

void UTinyverseHealthComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AActor* Owner = GetOwner())
	{
		Owner->OnTakeAnyDamage.RemoveDynamic(
			this,
			&UTinyverseHealthComponent::HandleOwnerTakeAnyDamage);
	}
	Super::EndPlay(EndPlayReason);
}

float UTinyverseHealthComponent::GetCurrentHealth() const
{
	return CurrentHealth;
}

float UTinyverseHealthComponent::GetMaxHealth() const
{
	return MaxHealth;
}

bool UTinyverseHealthComponent::IsDead() const
{
	return CurrentHealth <= 0.0f;
}

bool UTinyverseHealthComponent::IsInvulnerable() const
{
	const UWorld* World = GetWorld();
	
	return IsValid(World)
		&& World->GetTimeSeconds() < DamageCooldownEndTime;
}

float UTinyverseHealthComponent::Heal(float Amount)
{
	if (Amount <= 0.0f || IsDead())
	{
		return 0.0f;
	}
	
	const float PreviousHealth = CurrentHealth;
	
	CurrentHealth = FMath::Clamp(
		CurrentHealth + Amount,
		0.0f,
		MaxHealth);

	float const AppliedHealing = CurrentHealth - PreviousHealth;

	if (AppliedHealing > 0.0f)
	{
		OnHealthChanged.Broadcast(
			this,
			CurrentHealth,
			MaxHealth,
			AppliedHealing);
	}
	return AppliedHealing;
}

float UTinyverseHealthComponent::GrantHealthReward(float Amount)
{
	if (Amount <= 0.0f || IsDead())
	{
		return 0.0f;
	}

	if (CurrentHealth < MaxHealth)
	{
		return Heal(Amount);
	}

	const float PreviousMaxHealth = MaxHealth;
	MaxHealth = FMath::Clamp(
		MaxHealth + Amount,
		1.0f,
		MaximumHealthLimit);

	const float AddedMaximumHealth = MaxHealth - PreviousMaxHealth;

	if (AddedMaximumHealth <= 0.0f)
	{
		return 0.0f;
	}

	CurrentHealth = FMath::Min(CurrentHealth + AddedMaximumHealth, MaxHealth);

	OnHealthChanged.Broadcast(
		this,
		CurrentHealth,
		MaxHealth,
		AddedMaximumHealth);

	return AddedMaximumHealth;
}

void UTinyverseHealthComponent::HandleOwnerTakeAnyDamage(
	AActor* DamagedActor,
	float Damage,
	const UDamageType* DamageType,
	AController* InstigatedBy,
	AActor* DamageCauser)
{
	if (Damage <= 0.0f 
		|| IsDead()
		|| IsInvulnerable()
		|| DamagedActor != GetOwner())
	{
		return;
	}
	
	const float PreviousHealth = CurrentHealth;
	
	CurrentHealth = FMath::Clamp(
		CurrentHealth - Damage,
		0.0f,
		MaxHealth);
	
	float const HealthDelta = CurrentHealth - PreviousHealth;
	
	if (FMath::IsNearlyZero(HealthDelta))
	{
		return;
	}
	
	if (const UWorld* World = GetWorld())
	{
		DamageCooldownEndTime =
			World->GetTimeSeconds() + InvulnerabilityDuration;
	}
	
	OnHealthChanged.Broadcast(
		this,
		CurrentHealth,
		MaxHealth,
		HealthDelta);
	
	if (IsDead())
	{
		OnDeath.Broadcast(this, DamageCauser);
	}
}
