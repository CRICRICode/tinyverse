#include "TinyverseGravityPlanet.h"

#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "TinyverseCharacter.h"

ATinyverseGravityPlanet::ATinyverseGravityPlanet()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	GravityVolume = CreateDefaultSubobject<USphereComponent>(TEXT("GravityVolume"));
	GravityVolume->SetupAttachment(SceneRoot);
	GravityVolume->SetAbsolute(false, false, true);
	GravityVolume->InitSphereRadius(InfluenceRadius);
	GravityVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	GravityVolume->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	GravityVolume->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	GravityVolume->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	GravityVolume->SetGenerateOverlapEvents(true);
	GravityVolume->SetCanEverAffectNavigation(false);

	GravityVolume->OnComponentBeginOverlap.AddDynamic(
		this,
		&ATinyverseGravityPlanet::HandleGravityVolumeBeginOverlap);
	GravityVolume->OnComponentEndOverlap.AddDynamic(
		this,
		&ATinyverseGravityPlanet::HandleGravityVolumeEndOverlap);
}

void ATinyverseGravityPlanet::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	DetectedPlanetRadius = CalculatePlanetRadiusFromComponents();

	if (IsValid(GravityVolume))
	{
		GravityVolume->SetRelativeScale3D(FVector::OneVector);
		GravityVolume->SetSphereRadius(GetInfluenceRadius());
	}
}

void ATinyverseGravityPlanet::BeginPlay()
{
	Super::BeginPlay();

	TArray<AActor*> OverlappingActors;
	GravityVolume->GetOverlappingActors(OverlappingActors, ATinyverseCharacter::StaticClass());

	for (AActor* Actor : OverlappingActors)
	{
		if (ATinyverseCharacter* Character = Cast<ATinyverseCharacter>(Actor))
		{
			Character->RegisterGravityPlanet(this);
		}
	}
}

FVector ATinyverseGravityPlanet::GetGravityDirectionAtLocation(
	const FVector& WorldLocation) const
{
	return (GetActorLocation() - WorldLocation).GetSafeNormal();
}

float ATinyverseGravityPlanet::GetGravityStrengthAtLocation(
	const FVector& WorldLocation) const
{
	if (!IsLocationWithinInfluence(WorldLocation))
	{
		return 0.0f;
	}

	const float Radius = GetPlanetRadius();
	const float Distance = FMath::Max(
		FVector::Distance(GetActorLocation(), WorldLocation),
		Radius);
	const float DistanceRatio = Radius / Distance;
	const float GravityMultiplier = bUseGravityFalloff
		? FMath::Pow(DistanceRatio, FMath::Max(0.0f, GravityFalloffExponent))
		: 1.0f;

	return FMath::Max(0.0f, SurfaceGravity)
		* GravityMultiplier;
}

float ATinyverseGravityPlanet::GetGravitySelectionScoreAtLocation(
	const FVector& WorldLocation) const
{
	const float GravityStrength = GetGravityStrengthAtLocation(WorldLocation);

	if (GravityStrength <= KINDA_SMALL_NUMBER)
	{
		return TNumericLimits<float>::Max();
	}

	const float Radius = GetPlanetRadius();
	const float DistanceFromCenter =
		FVector::Distance(GetActorLocation(), WorldLocation);
	const float DistanceFromSurface =
		FMath::Max(0.0f, DistanceFromCenter - Radius);

	return DistanceFromSurface / GravityStrength;
}

float ATinyverseGravityPlanet::GetPlanetRadius() const
{
	if (bAutoDetectPlanetRadius && DetectedPlanetRadius > 1.0f)
	{
		return DetectedPlanetRadius;
	}

	return FMath::Max(1.0f, PlanetRadius)
		* GetActorScale3D().GetAbsMax();
}

float ATinyverseGravityPlanet::GetInfluenceRadius() const
{
	const float Radius = GetPlanetRadius();
	return FMath::Max(Radius, FMath::Max(1.0f, InfluenceRadius));
}

bool ATinyverseGravityPlanet::IsLocationWithinInfluence(
	const FVector& WorldLocation) const
{
	const float ScaledInfluenceRadius = IsValid(GravityVolume)
		                                    ? GravityVolume->GetScaledSphereRadius()
		                                    : GetInfluenceRadius();

	return FVector::DistSquared(GetActorLocation(), WorldLocation)
		<= FMath::Square(ScaledInfluenceRadius);
}

float ATinyverseGravityPlanet::CalculatePlanetRadiusFromComponents() const
{
	float LargestRadius = 0.0f;
	TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents(this);

	for (const UPrimitiveComponent* Component : PrimitiveComponents)
	{
		if (!IsValid(Component) || Component == GravityVolume)
		{
			continue;
		}

		const float RadiusFromPlanetCenter =
			FVector::Distance(GetActorLocation(), Component->Bounds.Origin)
			+ Component->Bounds.SphereRadius;

		LargestRadius = FMath::Max(LargestRadius, RadiusFromPlanetCenter);
	}

	return LargestRadius;
}

void ATinyverseGravityPlanet::HandleGravityVolumeBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (ATinyverseCharacter* Character = Cast<ATinyverseCharacter>(OtherActor))
	{
		Character->RegisterGravityPlanet(this);
	}
}

void ATinyverseGravityPlanet::HandleGravityVolumeEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex)
{
	if (ATinyverseCharacter* Character = Cast<ATinyverseCharacter>(OtherActor))
	{
		if (!GravityVolume->IsOverlappingActor(Character))
		{
			Character->UnregisterGravityPlanet(this);
		}
	}
}
