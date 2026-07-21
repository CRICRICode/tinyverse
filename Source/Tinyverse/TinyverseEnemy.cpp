#include "TinyverseEnemy.h"
#include "TinyverseCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

ATinyverseEnemy::ATinyverseEnemy()
{
	PrimaryActorTick.bCanEverTick = true;
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	StompTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("StompTrigger"));
	StompTrigger->SetupAttachment(GetCapsuleComponent());
	StompTrigger->SetAbsolute(false, false, false);
	StompTrigger->SetMobility(EComponentMobility::Movable);

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetCapsuleComponent()->SetCollisionObjectType(ECollisionChannel::ECC_Pawn);
	GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Block);
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
	
	bIsDefeated = true;
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

	Destroy();
	
	
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
