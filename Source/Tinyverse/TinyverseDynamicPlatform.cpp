// Fill out your copyright notice in the Description page of Project Settings.


#include "TinyverseDynamicPlatform.h"
#include "Components/StaticMeshComponent.h"


// Sets default values
ATinyverseDynamicPlatform::ATinyverseDynamicPlatform()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	Platform = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Platform"));
	SetRootComponent(Platform);
	Platform->SetMobility(EComponentMobility::Movable);
	Platform->SetRelativeScale3D(FVector(2.0f, 2.0f, 2.0f));
}

// Called when the game starts or when spawned
void ATinyverseDynamicPlatform::BeginPlay()
{
	Super::BeginPlay();

	StartLocation = GetActorLocation();
	EndLocation = StartLocation	+ GetActorTransform().TransformVectorNoScale(MovementOffset);
}

// Called every frame
void ATinyverseDynamicPlatform::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	PlatformSlide(DeltaTime);
}

void ATinyverseDynamicPlatform::PlatformSlide(float DeltaTime)
{
	if (MovementSpeed <= 0.0f)
	{
		return;
	}
	
	const FVector TargetLocation = bIsMovingToEnd ? EndLocation : StartLocation;

	const FVector NewLocation = FMath::VInterpConstantTo(
		GetActorLocation(),
		TargetLocation,
		DeltaTime,
		MovementSpeed);

	SetActorLocation(NewLocation);
	
	if (GetActorLocation().Equals(TargetLocation, 1.0f))
	{
		bIsMovingToEnd = !bIsMovingToEnd;
	}
	
	
}
