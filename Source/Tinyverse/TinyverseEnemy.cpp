#include "TinyverseEnemy.h"
#include "TinyverseCharacter.h"
#include "TinyverseHealthComponent.h"
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
	if (!IsValid(PlayerCharacter) || IsDefeated)
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
	
	UGameplayStatics::ApplyDamage(
		this,
		StompDamage,
		PlayerCharacter->GetController(),
		PlayerCharacter,
		UDamageType::StaticClass());
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
	if (IsDefeated || DeltaTime <= 0.0f)
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
	if (IsDefeated || ContactDamage <= 0.0f)
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

	UTinyverseHealthComponent* PlayerHealthComponent =
		PlayerCharacter->GetHealthComponent();
	
	const float PreviousHealth = IsValid(PlayerHealthComponent)
		                             ? PlayerHealthComponent->GetCurrentHealth()
		                             : 0.0f;

	UGameplayStatics::ApplyDamage(
		PlayerCharacter,
		ContactDamage,
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

	UE_LOG(
		LogTemp,
		Display,
		TEXT("%s ha inflitto %.0f danni a %s (HP: %.0f)"),
		*GetNameSafe(this),
		PreviousHealth - PlayerHealthComponent->GetCurrentHealth(),
		*GetNameSafe(PlayerCharacter),
		PlayerHealthComponent->GetCurrentHealth());
}

void ATinyverseEnemy::HandleDeath(
	UTinyverseHealthComponent* DeadHealthComponent,
	AActor* DamageCauser)
{
	if (IsDefeated || DeadHealthComponent != EnemyHealthComponent)
	{
		return;
	}
	
	IsDefeated = true;
	
	UE_LOG(
		LogTemp,
		Display,
		TEXT("%s sconfitto da %s"),
		*GetNameSafe(this),
		*GetNameSafe(DamageCauser));

	Destroy();
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
