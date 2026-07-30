#include "TinyverseEnemy.h"
#include "TinyverseCharacter.h"
#include "TinyverseHealthComponent.h"
#include "TinyverseSaveIdentityComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"


ATinyverseEnemy::ATinyverseEnemy()
{
	PrimaryActorTick.bCanEverTick = true;
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	
	EnemyHealthComponent =
		CreateDefaultSubobject<UTinyverseHealthComponent>(
			TEXT("HealthComponent"));
	
	SaveIdentityComponent =
		CreateDefaultSubobject<UTinyverseSaveIdentityComponent>(
				TEXT("SaveIdentityComponent"));
	
	StompTrigger = CreateDefaultSubobject<UCapsuleComponent>(TEXT("StompTrigger"));
	StompTrigger->SetupAttachment(GetCapsuleComponent());
	StompTrigger->SetAbsolute(false, false, false);
	StompTrigger->SetMobility(EComponentMobility::Movable);
	StompTrigger->SetCanEverAffectNavigation(false);
	StompTrigger->CanCharacterStepUpOn = ECB_No;

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetCapsuleComponent()->SetCollisionObjectType(ECollisionChannel::ECC_Pawn);
	GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic,
	                                                     ECollisionResponse::ECR_Block);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic,
	                                                     ECollisionResponse::ECR_Block);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Block);
	GetCapsuleComponent()->SetGenerateOverlapEvents(false);

	StompTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	StompTrigger->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	StompTrigger->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	StompTrigger->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	StompTrigger->SetGenerateOverlapEvents(true);

	StompTrigger->OnComponentBeginOverlap.AddDynamic(
		this,
		&ATinyverseEnemy::HandleStompTriggerBeginOverlap);

	DamageTrigger = CreateDefaultSubobject<USphereComponent>(TEXT("DamageTrigger"));
	DamageTrigger->SetupAttachment(GetCapsuleComponent());
	DamageTrigger->InitSphereRadius(36.0f);
	DamageTrigger->SetAbsolute(false, false, false);
	DamageTrigger->SetMobility(EComponentMobility::Movable);
	DamageTrigger->SetCanEverAffectNavigation(false);
	DamageTrigger->CanCharacterStepUpOn = ECB_No;
	DamageTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DamageTrigger->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	DamageTrigger->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	DamageTrigger->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	DamageTrigger->SetGenerateOverlapEvents(true);
	DamageTrigger->OnComponentBeginOverlap.AddDynamic(
		this,
		&ATinyverseEnemy::HandleDamageTriggerBeginOverlap);
}

void ATinyverseEnemy::HandleStompTriggerBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	ATinyverseCharacter* PlayerCharacter = Cast<ATinyverseCharacter>(OtherActor);
	if (!IsValid(PlayerCharacter) || bIsDefeated)
	{
		return;
	}

	UCharacterMovementComponent* PlayerMovement = PlayerCharacter->GetCharacterMovement();

	if (IsValid(PlayerMovement))
	{
		FVector LocalUp =
			-PlayerMovement->GetGravityDirection().GetSafeNormal();

		if (LocalUp.IsNearlyZero())
		{
			LocalUp = PlayerCharacter->GetActorUpVector();
		}

		const FVector TangentVelocity = FVector::VectorPlaneProject(
			PlayerCharacter->GetVelocity(),
			LocalUp);

		const FVector BounceVelocity =
			TangentVelocity + LocalUp * StompBounceSpeed;

		PlayerCharacter->LaunchCharacter(
			BounceVelocity,
			true,
			true);
	}
	
	if (ChargePhase != ETinyverseEnemyChargePhase::Inactive)
	{
		EnterChargePhase(
			ETinyverseEnemyChargePhase::Recovery);
	}

#if !UE_BUILD_SHIPPING
	const FString EnemyName = GetNameSafe(this);
	const FString PlayerName = GetNameSafe(PlayerCharacter);
	const float PreviousHealth = IsValid(EnemyHealthComponent)
		                             ? EnemyHealthComponent->GetCurrentHealth()
		                             : 0.0f;
	const float MaxHealth = IsValid(EnemyHealthComponent)
		                        ? EnemyHealthComponent->GetMaxHealth()
		                        : 0.0f;
	const bool bWasInvulnerable = IsValid(EnemyHealthComponent)
		                              && EnemyHealthComponent->IsInvulnerable();
	const bool bCouldBeDamaged = CanBeDamaged();
#endif
	
	UGameplayStatics::ApplyDamage(
		this,
		StompDamage,
		PlayerCharacter->GetController(),
		PlayerCharacter,
		UDamageType::StaticClass());

#if !UE_BUILD_SHIPPING
	const float CurrentHealth = EnemyHealthComponent != nullptr
		                            ? EnemyHealthComponent->GetCurrentHealth()
		                            : PreviousHealth;
	const float AppliedDamage = PreviousHealth - CurrentHealth;

	UE_LOG(
		LogTemp,
		Display,
		TEXT("[STOMP] %s su %s | %s | richiesto: %.1f, applicato: %.1f, HP: %.1f/%.1f -> %.1f/%.1f, CanBeDamaged: %s, InvulnerableBefore: %s"),
		*PlayerName,
		*EnemyName,
		AppliedDamage > 0.0f ? TEXT("VALIDO") : TEXT("IGNORATO"),
		StompDamage,
		AppliedDamage,
		PreviousHealth,
		MaxHealth,
		CurrentHealth,
		MaxHealth,
		bCouldBeDamaged ? TEXT("true") : TEXT("false"),
		bWasInvulnerable ? TEXT("true") : TEXT("false"));
#endif
}


void ATinyverseEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateGravity();
}

void ATinyverseEnemy::UpdateGravity()
{
	if (!IsValid(GravityPlanet))
	{
		return;
	}

	const FVector GravityDirection =
		(GravityPlanet->GetActorLocation() - GetActorLocation())
		.GetSafeNormal();

	if (GravityDirection.IsNearlyZero())
	{
		return;
	}

	GetCharacterMovement()->SetGravityDirection(GravityDirection);

	const FVector LocalUp = -GravityDirection;

	FVector TangentForward = FVector::VectorPlaneProject(
		GetActorForwardVector(),
		LocalUp).GetSafeNormal();

	if (TangentForward.IsNearlyZero())
	{
		FVector TangentRight;
		LocalUp.FindBestAxisVectors(TangentForward, TangentRight);
	}

	const FQuat TargetRotation =
		FRotationMatrix::MakeFromXZ(TangentForward, LocalUp).ToQuat();

	SetActorRotation(TargetRotation);
}

void ATinyverseEnemy::BeginPlay()
{
	Super::BeginPlay();
	
	if (IsValid(SaveIdentityComponent) && SaveIdentityComponent->IsOwnerMarkedAsRemoved())
	{
		Destroy();
		return;
	}

	if (IsValid(EnemyHealthComponent))
	{
		EnemyHealthComponent->OnDeath.AddUniqueDynamic(
			this,
			&ATinyverseEnemy::HandleDeath);
	}

	UpdateGravity();

	UCharacterMovementComponent* Movement = GetCharacterMovement();

	if (!IsValid(Movement))
	{
		return;
	}

	Movement->MaxWalkSpeed = PatrolSpeed;
	Movement->bOrientRotationToMovement = false;

	const FVector GravityDirection = Movement->GetGravityDirection().GetSafeNormal();

	const FVector LocalUp = GravityDirection.IsNearlyZero()
		                        ? GetActorUpVector().GetSafeNormal()
		                        : -GravityDirection;

	PatrolDirection = FVector::VectorPlaneProject(GetActorForwardVector(),
	                                              LocalUp.GetSafeNormal());

	if (PatrolDirection.IsNearlyZero())
	{
		FVector TangentRight;
		
		LocalUp.FindBestAxisVectors(
			PatrolDirection,
			TangentRight);
	}
}

void ATinyverseEnemy::MoveAlongPlanet(float DeltaTime)
{
	if (bIsDefeated 
		|| DeltaTime <= 0.0f
		|| ChargePhase != ETinyverseEnemyChargePhase::Inactive)
	{
		return;
	}
	
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	
	if (!IsValid(Movement))
	{
		return;
	}
	
	const FVector GravityDirection = Movement->GetGravityDirection().GetSafeNormal();
	
	if (GravityDirection.IsNearlyZero())
	{
		return;
	}
	
	const FVector LocalUp = - GravityDirection;
	
	PatrolDirection = FVector::VectorPlaneProject(
		PatrolDirection,
		LocalUp).GetSafeNormal();
	
	if (PatrolDirection.IsNearlyZero())
	{
		PatrolDirection = FVector::VectorPlaneProject(
			GetActorForwardVector(),
			LocalUp).GetSafeNormal();
	}
	
	if (PatrolDirection.IsNearlyZero())
	{
		FVector TangentRight;
		LocalUp.FindBestAxisVectors(
			PatrolDirection,
			TangentRight);
	}
	
	AddMovementInput(PatrolDirection, 1.0f);
	
	FVector CurrentForward = FVector::VectorPlaneProject(
		GetActorForwardVector(),
		LocalUp).GetSafeNormal();

	if (CurrentForward.IsNearlyZero())
	{
		CurrentForward = PatrolDirection;
	}

	const float ForwardDot = FMath::Clamp(
		FVector::DotProduct(CurrentForward, PatrolDirection),
		-1.0f,
		1.0f);

	const float SignedAngle = FMath::Atan2(
		FVector::DotProduct(
			FVector::CrossProduct(CurrentForward, PatrolDirection),
			LocalUp),
		ForwardDot);
	
	const float TurnAlpha = 1.0f - FMath::Exp(-PatrolTurnSpeed * DeltaTime);
	
	const FVector SmoothedForward = FQuat(
		LocalUp,
		SignedAngle * TurnAlpha)
		.RotateVector(CurrentForward)
		.GetSafeNormal();

	const FQuat SmoothedRotation = FRotationMatrix::MakeFromXZ(
		SmoothedForward,
		LocalUp).ToQuat();
	
	SetActorRotation(SmoothedRotation);
}

void ATinyverseEnemy::ReversePatrolDirection()
{
	if (PatrolDirection.IsNearlyZero())
	{
		PatrolDirection = -GetActorForwardVector();
		return;
	}
	
	PatrolDirection *= -1.0f;
}

void ATinyverseEnemy::HandleDamageTriggerBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (bIsDefeated)
	{
		return;
	}

	ATinyverseCharacter* PlayerCharacter = Cast<ATinyverseCharacter>(OtherActor);

	if (!IsValid(PlayerCharacter))
	{
		return;
	}

	if (StompTrigger->IsOverlappingActor(PlayerCharacter))
	{
		return;
	}
	
	const float DamageAmount =
	ChargePhase == ETinyverseEnemyChargePhase::Charging
		? ChargeDamage
		: ContactDamage;

	if (bIsDefeated || DamageAmount <= 0.0f)
	{
		return;
	}

	UTinyverseHealthComponent* PlayerHealthComponent =
		PlayerCharacter->GetHealthComponent();
	
	const float PreviousHealth = IsValid(PlayerHealthComponent)
		                             ? PlayerHealthComponent->GetCurrentHealth()
		                             : 0.0f;

	if (ChargePhase != ETinyverseEnemyChargePhase::Inactive)
	{
		EnterChargePhase(
			ETinyverseEnemyChargePhase::Recovery);
	}
	
	UGameplayStatics::ApplyDamage(
		PlayerCharacter,
		DamageAmount,
		GetController(),
		this,
		UDamageType::StaticClass());

	const bool bDamageWasApplied = IsValid(PlayerHealthComponent)
		                               && PlayerHealthComponent->GetCurrentHealth() < PreviousHealth;

	if (!bDamageWasApplied)
	{
		return;
	}

	UCharacterMovementComponent* PlayerMovement = PlayerCharacter->GetCharacterMovement();
	FVector LocalUp = IsValid(PlayerMovement)
		                  ? -PlayerMovement->GetGravityDirection().GetSafeNormal()
		                  : FVector::ZeroVector;

	if (LocalUp.IsNearlyZero())
	{
		LocalUp = PlayerCharacter->GetActorUpVector().GetSafeNormal();
	}

	FVector AwayDirection = FVector::VectorPlaneProject(
		PlayerCharacter->GetActorLocation() - GetActorLocation(),
		LocalUp).GetSafeNormal();

	if (AwayDirection.IsNearlyZero())
	{
		AwayDirection = FVector::VectorPlaneProject(
			-GetActorForwardVector(),
			LocalUp).GetSafeNormal();
	}

	const FVector KnockbackVelocity =
		AwayDirection * ContactKnockbackSpeed
		+ LocalUp * ContactKnockbackLiftSpeed;

	PlayerCharacter->LaunchCharacter(KnockbackVelocity, true, true);

#if !UE_BUILD_SHIPPING
	UE_LOG(
		LogTemp,
		Display,
		TEXT("%s ha inflitto %.0f danni a %s (HP: %.0f)"),
		*GetNameSafe(this),
		PreviousHealth - PlayerHealthComponent->GetCurrentHealth(),
		*GetNameSafe(PlayerCharacter),
		PlayerHealthComponent->GetCurrentHealth());
#endif
}

void ATinyverseEnemy::HandleDeath(
	UTinyverseHealthComponent* DeadHealthComponent,
	AActor* DamageCauser)
{
	if (bIsDefeated || DeadHealthComponent != EnemyHealthComponent)
	{
		return;
	}
	
	CancelCharge();
	bIsDefeated = true;
	
#if !UE_BUILD_SHIPPING
	UE_LOG(
		LogTemp,
		Display,
		TEXT("%s sconfitto da %s"),
		*GetNameSafe(this),
		*GetNameSafe(DamageCauser));
#endif
	
	if (IsValid(SaveIdentityComponent))
	{
		SaveIdentityComponent->MarkOwnerAsRemoved();
	}
	
	Destroy();
}


float ATinyverseEnemy::GetDetectionRadius() const
{
	return FMath::Max(0.0f, DetectionRadius);
}

float ATinyverseEnemy::GetLoseTargetRadius() const
{
	return FMath::Max(GetDetectionRadius(), LoseTargetRadius);
}

float ATinyverseEnemy::GetChargeActivationRadius() const
{
	return FMath::Clamp(
		ChargeActivationRadius,
		0.0f,
		GetDetectionRadius());
}

bool ATinyverseEnemy::TrackTargetAlongPlanet(
	AActor* TargetActor,
	float DeltaTime)
{
	if (!IsValid(TargetActor)
		|| TargetActor == this
		|| bIsDefeated
		|| DeltaTime <= 0.0f
		|| ChargePhase != ETinyverseEnemyChargePhase::Inactive)
	{
		return false;
	}

	UCharacterMovementComponent* Movement = GetCharacterMovement();

	if (!IsValid(Movement))
	{
		return false;
	}

	const FVector LocalUp = GetLocalUp();
	const FVector DesiredDirection = GetTangentDirectionTo(TargetActor);

	if (LocalUp.IsNearlyZero() || DesiredDirection.IsNearlyZero())
	{
		return false;
	}

	Movement->MaxWalkSpeed = FMath::Max(0.0f, TrackingSpeed);
	AddMovementInput(DesiredDirection, 1.0f);

	FVector CurrentForward = FVector::VectorPlaneProject(
		GetActorForwardVector(),
		LocalUp).GetSafeNormal();

	if (CurrentForward.IsNearlyZero())
	{
		CurrentForward = DesiredDirection;
	}

	const float ForwardDot = FMath::Clamp(
		FVector::DotProduct(CurrentForward, DesiredDirection),
		-1.0f,
		1.0f);

	const float SignedAngle = FMath::Atan2(
		FVector::DotProduct(
			FVector::CrossProduct(CurrentForward, DesiredDirection),
			LocalUp),
		ForwardDot);

	const float TurnAlpha = 1.0f - FMath::Exp(
		-FMath::Max(0.1f, TrackingTurnSpeed) * DeltaTime);

	const FVector SmoothedForward = FQuat(
		LocalUp,
		SignedAngle * TurnAlpha)
		.RotateVector(CurrentForward)
		.GetSafeNormal();

	SetActorRotation(
		FRotationMatrix::MakeFromXZ(
			SmoothedForward,
			LocalUp).ToQuat());

	return true;
}

void ATinyverseEnemy::StopTracking()
{
	if (ChargePhase != ETinyverseEnemyChargePhase::Inactive)
	{
		return;
	}

	UCharacterMovementComponent* Movement = GetCharacterMovement();

	if (!IsValid(Movement))
	{
		return;
	}

	Movement->StopMovementImmediately();
	Movement->MaxWalkSpeed = PatrolSpeed;
}

FVector ATinyverseEnemy::GetLocalUp() const
{
	const UCharacterMovementComponent* Movement = GetCharacterMovement();
	
	if (IsValid(Movement))
	{
		const FVector GravityDirection = Movement->GetGravityDirection().GetSafeNormal();
		
		if (!GravityDirection.IsNearlyZero())
		{
			return -GravityDirection;
		}
	}
	
	return GetActorUpVector().GetSafeNormal();
}

FVector ATinyverseEnemy::GetTangentDirectionTo(const AActor* TargetActor) const
{
	if (!IsValid(TargetActor))
	{
		return FVector::ZeroVector;
	}
	
	return FVector::VectorPlaneProject(
		TargetActor->GetActorLocation() - GetActorLocation(),
		GetLocalUp()).GetSafeNormal();
}

bool ATinyverseEnemy::StartCharge(AActor* TargetActor)
{
	const FVector InitialDirection =
	GetTangentDirectionTo(TargetActor);
	
	if (!IsValid(TargetActor) ||
		TargetActor == this
		|| bIsDefeated
		|| ChargePhase != ETinyverseEnemyChargePhase::Inactive
		|| InitialDirection.IsNearlyZero())
	{
		return false;
	}
	
	ChargeTarget = TargetActor;
	ChargeDirection = InitialDirection;
	
	EnterChargePhase(ETinyverseEnemyChargePhase::Windup);
	
	return true;
}

void ATinyverseEnemy::EnterChargePhase(ETinyverseEnemyChargePhase NewPhase)
{
	ChargePhase = NewPhase;
	ChargePhaseElapsedTime = 0.0f;
	
	if (ChargePhase == ETinyverseEnemyChargePhase::Inactive)
	{
		ChargeTarget = nullptr;
		ChargeDirection = FVector::ZeroVector;
	}
	else if (ChargePhase == ETinyverseEnemyChargePhase::Charging)
	{
		ChargeDirection = FVector::VectorPlaneProject(
			GetActorForwardVector(),
			GetLocalUp()).GetSafeNormal();
		
		if (ChargeDirection.IsNearlyZero())
		{
			ChargeDirection = GetTangentDirectionTo(ChargeTarget);
		}
	}
	
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	
	if (!IsValid(Movement))
	{
		return;
	}
	
	switch (ChargePhase)
	{
	case ETinyverseEnemyChargePhase::Inactive:
		Movement->StopMovementImmediately();
		Movement->MaxWalkSpeed = PatrolSpeed;
		break;

	case ETinyverseEnemyChargePhase::Windup:
		Movement->StopMovementImmediately();
		Movement->MaxWalkSpeed = 0.0f;
		break;

	case ETinyverseEnemyChargePhase::Charging:
		Movement->MaxWalkSpeed = ChargeSpeed;
		break;

	case ETinyverseEnemyChargePhase::Recovery:
		Movement->StopMovementImmediately();
		Movement->MaxWalkSpeed = 0.0f;
		break;
	}
}

void ATinyverseEnemy::UpdateChargeFacing(float DeltaTime)
{
	if (DeltaTime <= 0.0f || !IsValid(ChargeTarget))
	{
		return;
	}
	
	const FVector LocalUp = GetLocalUp();
	const FVector DesiredForward = GetTangentDirectionTo(ChargeTarget);
	
	FVector CurrentForward = 
		FVector::VectorPlaneProject(
			GetActorForwardVector(),
			LocalUp).GetSafeNormal();
	
	if (CurrentForward.IsNearlyZero() || DesiredForward.IsNearlyZero())
	{
		return;
	}
	
	const float ForwardDot = FMath::Clamp(
		FVector::DotProduct(CurrentForward, DesiredForward),
		-1.0f,
		1.0f);
	
	const float SignedAngle = FMath::Atan2(
		FVector::DotProduct(
			FVector::CrossProduct(CurrentForward, DesiredForward),
			LocalUp),
		ForwardDot);

	
	const float MaxTurnStep =
		ChargeWindupDuration <= KINDA_SMALL_NUMBER
			? FMath::Abs(SignedAngle)
			: PI*DeltaTime/ChargeWindupDuration;

	const float TurnStep = FMath::Clamp(
		SignedAngle,
		-MaxTurnStep,
		MaxTurnStep);

	const FVector NewForward = FQuat(LocalUp, TurnStep)
		.RotateVector(CurrentForward).GetSafeNormal();
	
	SetActorRotation(
		FRotationMatrix::MakeFromXZ(
			NewForward,
			LocalUp).ToQuat());
	
	ChargeDirection = DesiredForward;
	
}

void ATinyverseEnemy::UpdateChargeMovement()
{
	const FVector LocalUp = GetLocalUp();

	ChargeDirection = FVector::VectorPlaneProject(
		ChargeDirection,
		LocalUp).GetSafeNormal();

	if (ChargeDirection.IsNearlyZero())
	{
		return;
	}

	AddMovementInput(ChargeDirection, 1.0f);

	SetActorRotation(
		FRotationMatrix::MakeFromXZ(
			ChargeDirection,
			LocalUp).ToQuat());
}

bool ATinyverseEnemy::UpdateCharge(float DeltaTime)
{
	if (ChargePhase == ETinyverseEnemyChargePhase::Inactive)
	{
		return true;
	}

	if (bIsDefeated)
	{
		CancelCharge();
		return true;
	}

	if (DeltaTime <= 0.0f)
	{
		return false;
	}

	ChargePhaseElapsedTime += DeltaTime;

	switch (ChargePhase)
	{
	case ETinyverseEnemyChargePhase::Windup:
		if (!IsValid(ChargeTarget))
		{
			CancelCharge();
			return true;
		}

		UpdateChargeFacing(DeltaTime);

		if (ChargePhaseElapsedTime >=
			FMath::Max(0.0f, ChargeWindupDuration))
		{
			EnterChargePhase(
				ETinyverseEnemyChargePhase::Charging);
		}
		break;

	case ETinyverseEnemyChargePhase::Charging:
		UpdateChargeMovement();

		if (ChargePhaseElapsedTime >=
			FMath::Max(0.0f, ChargeDuration))
		{
			EnterChargePhase(
				ETinyverseEnemyChargePhase::Recovery);
		}
		break;

	case ETinyverseEnemyChargePhase::Recovery:
		if (ChargePhaseElapsedTime >=
			FMath::Max(0.0f, ChargeRecoveryDuration))
		{
			CancelCharge();
			return true;
		}
		break;

	case ETinyverseEnemyChargePhase::Inactive:
		return true;
	}

	return false;
}

void ATinyverseEnemy::CancelCharge()
{
	if (ChargePhase == ETinyverseEnemyChargePhase::Inactive)
	{
		return;
	}

	EnterChargePhase(
		ETinyverseEnemyChargePhase::Inactive);
}

void ATinyverseEnemy::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(EnemyHealthComponent))
	{
		EnemyHealthComponent->OnDeath.RemoveDynamic(
			this,
			&ATinyverseEnemy::HandleDeath);
	}
	
	Super::EndPlay(EndPlayReason);
}
