// Copyright Epic Games, Inc. All Rights Reserved.

#include "TinyverseCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EngineUtils.h"
#include "InputActionValue.h"
#include "Tinyverse.h"
#include "TinyverseGravityPlanet.h"
#include "TinyverseHealthComponent.h"

ATinyverseCharacter::ATinyverseCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	
	HealthComponent =
		CreateDefaultSubobject<UTinyverseHealthComponent>(
			TEXT("HealthComponent"));
	
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = false;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = false;
	CameraBoom->SetAbsolute(false, true, false);

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

void ATinyverseCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (bEnablePlanetGravity)
	{
		for (TActorIterator<ATinyverseGravityPlanet> PlanetIterator(GetWorld()); PlanetIterator; ++PlanetIterator)
		{
			if (PlanetIterator->IsLocationWithinInfluence(GetActorLocation()))
			{
				RegisterGravityPlanet(*PlanetIterator);
			}
		}

		UpdatePlanetGravity(0.0f);
	}

	InitializeGravityAlignedCamera();
}

void ATinyverseCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdatePlanetGravity(DeltaSeconds);

	const FVector LocalUp = GetLocalUp();
	AlignCameraFrameToUp(LocalUp);
	UpdateCharacterFacing(DeltaSeconds, LocalUp);
	UpdateAutomaticCamera(DeltaSeconds, LocalUp);
	ApplyGravityAlignedCamera();
}

void ATinyverseCharacter::RegisterGravityPlanet(
	ATinyverseGravityPlanet* GravityPlanet)
{
	if (IsValid(GravityPlanet))
	{
		NearbyGravityPlanets.AddUnique(GravityPlanet);
	}
}

void ATinyverseCharacter::UnregisterGravityPlanet(
	ATinyverseGravityPlanet* GravityPlanet)
{
	NearbyGravityPlanets.RemoveSingleSwap(GravityPlanet);

	if (ActiveGravityPlanet == GravityPlanet)
	{
		ActiveGravityPlanet = nullptr;
	}
}

ATinyverseGravityPlanet* ATinyverseCharacter::GetActiveGravityPlanet() const
{
	return ActiveGravityPlanet;
}

void ATinyverseCharacter::CollectCoin()
{
	++CoinCount;
	OnCoinCountChanged.Broadcast(CoinCount);

#if !UE_BUILD_SHIPPING
	UE_LOG(
		LogTinyverse,
		Display,
		TEXT("%s ha raccolto una moneta. Totale: %d"),
		*GetNameSafe(this),
		CoinCount);
#endif

	if (CoinsPerHealthReward <= 0
		|| CoinCount % CoinsPerHealthReward != 0
		|| !IsValid(HealthComponent))
	{
		return;
	}

	const float AppliedReward = HealthComponent->GrantHealthReward(1.0f);

#if !UE_BUILD_SHIPPING
	UE_LOG(
		LogTinyverse,
		Display,
		TEXT("Soglia monete raggiunta: %d. Ricompensa HP: %.0f, salute: %.0f/%.0f"),
		CoinCount,
		AppliedReward,
		HealthComponent->GetCurrentHealth(),
		HealthComponent->GetMaxHealth());
#endif
}

int32 ATinyverseCharacter::GetCoinCount() const
{
	return CoinCount;
}

void ATinyverseCharacter::UpdatePlanetGravity(float DeltaSeconds)
{
	if (!bEnablePlanetGravity)
	{
		return;
	}

	UCharacterMovementComponent* Movement = GetCharacterMovement();

	if (!IsValid(Movement))
	{
		return;
	}

	const FVector CharacterLocation = GetActorLocation();

	NearbyGravityPlanets.RemoveAllSwap(
		[CharacterLocation](const TObjectPtr<ATinyverseGravityPlanet>& Planet)
		{
			return !IsValid(Planet)
				|| !Planet->IsLocationWithinInfluence(CharacterLocation);
		});

	ATinyverseGravityPlanet* StrongestPlanet = nullptr;
	float StrongestGravity = 0.0f;
	float ActivePriority = 0.0f;
	bool bActivePlanetIsCandidate = false;
	ATinyverseGravityPlanet* ClosestSurfacePlanet = nullptr;
	float ClosestSurfaceDistance = TNumericLimits<float>::Max();

	for (ATinyverseGravityPlanet* Planet : NearbyGravityPlanets)
	{
		if (!IsValid(Planet))
		{
			continue;
		}

		const float GravityPriority =
			Planet->GetGravityPriorityAtLocation(CharacterLocation);
		const float DistanceFromSurface = FMath::Max(
			0.0f,
			FVector::Distance(Planet->GetActorLocation(), CharacterLocation)
			- Planet->GetPlanetRadius());

		if (DistanceFromSurface <= GravitySurfaceCaptureDistance
			&& DistanceFromSurface < ClosestSurfaceDistance)
		{
			ClosestSurfaceDistance = DistanceFromSurface;
			ClosestSurfacePlanet = Planet;
		}

		if (Planet == ActiveGravityPlanet)
		{
			ActivePriority = GravityPriority;
			bActivePlanetIsCandidate = GravityPriority > 0.0f;
		}

		if (GravityPriority > StrongestGravity)
		{
			StrongestGravity = GravityPriority;
			StrongestPlanet = Planet;
		}
	}

	ATinyverseGravityPlanet* GroundPlanet = nullptr;

	if (Movement->IsMovingOnGround())
	{
		GroundPlanet = Cast<ATinyverseGravityPlanet>(
			Movement->CurrentFloor.HitResult.GetActor());
	}

	if (IsValid(ClosestSurfacePlanet))
	{
		ActiveGravityPlanet = ClosestSurfacePlanet;
		bActivePlanetIsCandidate = true;
	}
	else if (IsValid(GroundPlanet)
		&& GroundPlanet->IsLocationWithinInfluence(CharacterLocation))
	{
		NearbyGravityPlanets.AddUnique(GroundPlanet);
		ActiveGravityPlanet = GroundPlanet;
		bActivePlanetIsCandidate = true;
	}

	const bool bKeepGroundedSource = bLockGravitySourceWhileGrounded
		&& Movement->IsMovingOnGround()
		&& !IsValid(GroundPlanet)
		&& bActivePlanetIsCandidate;

	if (!IsValid(ClosestSurfacePlanet)
		&& !IsValid(GroundPlanet)
		&& !bActivePlanetIsCandidate)
	{
		ActiveGravityPlanet = StrongestPlanet;
	}
	else if (!IsValid(ClosestSurfacePlanet)
		&& !IsValid(GroundPlanet)
		&& !bKeepGroundedSource
		&& StrongestPlanet != ActiveGravityPlanet
		&& StrongestGravity > ActivePriority * FMath::Max(1.0f, GravitySwitchRatio))
	{
		ActiveGravityPlanet = StrongestPlanet;
	}

	if (!IsValid(ActiveGravityPlanet))
	{
		CurrentGravityStrength = 0.0f;
		Movement->GravityScale = 0.0f;
		return;
	}

	CurrentGravityStrength =
		ActiveGravityPlanet->GetGravityStrengthAtLocation(CharacterLocation);

	const FVector DesiredGravityDirection =
		ActiveGravityPlanet->GetGravityDirectionAtLocation(CharacterLocation);

	if (DesiredGravityDirection.IsNearlyZero())
	{
		Movement->GravityScale = 0.0f;
		return;
	}

	FVector NewGravityDirection = DesiredGravityDirection;

	if (bHasPlanetGravityDirection
		&& DeltaSeconds > 0.0f
		&& GravityDirectionRotationSpeed > 0.0f)
	{
		NewGravityDirection = FMath::VInterpNormalRotationTo(
			Movement->GetGravityDirection().GetSafeNormal(),
			DesiredGravityDirection,
			DeltaSeconds,
			GravityDirectionRotationSpeed);
	}

	Movement->SetGravityDirection(NewGravityDirection);
	bHasPlanetGravityDirection = true;

	const UWorld* World = GetWorld();
	const float BaseGravity = IsValid(World)
		                          ? FMath::Abs(World->GetGravityZ())
		                          : 0.0f;

	Movement->GravityScale = BaseGravity > KINDA_SMALL_NUMBER
		                         ? CurrentGravityStrength / BaseGravity
		                         : 0.0f;

#if !UE_BUILD_SHIPPING
	if (bShowGravityDebug)
	{
		const FVector ArrowStart = CharacterLocation;
		const FVector ArrowEnd = ArrowStart + NewGravityDirection * 180.0f;

		DrawDebugDirectionalArrow(
			GetWorld(),
			ArrowStart,
			ArrowEnd,
			40.0f,
			FColor::Red,
			false,
			0.0f,
			0,
			3.0f);
	}
#endif
}

void ATinyverseCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ATinyverseCharacter::Move);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &ATinyverseCharacter::StopMove);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Canceled, this, &ATinyverseCharacter::StopMove);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ATinyverseCharacter::Look);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ATinyverseCharacter::Look);
	}
	else
	{
		UE_LOG(LogTinyverse, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void ATinyverseCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	// route the input
	DoMove(MovementVector.X, MovementVector.Y);
}

void ATinyverseCharacter::StopMove(const FInputActionValue&)
{
	const bool bWasMovingBackward = ForwardMovementInput < -KINDA_SMALL_NUMBER;
	ForwardMovementInput = 0.0f;

	if (bWasMovingBackward)
	{
		TimeSinceLastCameraInput = 0.0f;
	}
}

void ATinyverseCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// route the input
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void ATinyverseCharacter::DoMove(float Right, float Forward)
{
	ForwardMovementInput = Forward;

	if (GetController() == nullptr)
	{
		return;
	}

	if (!bGravityAlignedCameraInitialized)
	{
		InitializeGravityAlignedCamera();
	}

	const FVector LocalUp = GetLocalUp();
	FVector ForwardDirection = FVector::VectorPlaneProject(CameraYawForward, LocalUp).GetSafeNormal();

	if (ForwardDirection.IsNearlyZero())
	{
		ForwardDirection = FVector::VectorPlaneProject(GetActorForwardVector(), LocalUp).GetSafeNormal();
	}

	const FVector RightDirection = FVector::CrossProduct(LocalUp, ForwardDirection).GetSafeNormal();

	if (!ForwardDirection.IsNearlyZero())
	{
		AddMovementInput(ForwardDirection, Forward);
	}

	if (!RightDirection.IsNearlyZero())
	{
		AddMovementInput(RightDirection, Right);
	}

	const FVector DesiredMoveDirection = (ForwardDirection * Forward + RightDirection * Right).GetSafeNormal();

	if (!DesiredMoveDirection.IsNearlyZero())
	{
		DesiredFacingDirection = DesiredMoveDirection;
	}
}

void ATinyverseCharacter::DoLook(float Yaw, float Pitch)
{
	if (GetController() == nullptr)
	{
		return;
	}

	if (!bGravityAlignedCameraInitialized)
	{
		InitializeGravityAlignedCamera();
	}

	const FVector LocalUp = GetLocalUp();
	AlignCameraFrameToUp(LocalUp);

	if (!FMath::IsNearlyZero(Yaw) || !FMath::IsNearlyZero(Pitch))
	{
		TimeSinceLastCameraInput = 0.0f;

		const float NaturalYaw = bInvertHorizontalCamera ? -Yaw : Yaw;
		const float NaturalPitch = bInvertVerticalCamera ? Pitch : -Pitch;
		const float YawAngle = FMath::DegreesToRadians(NaturalYaw * CameraYawSensitivity);
		CameraYawForward = FQuat(LocalUp, YawAngle).RotateVector(CameraYawForward).GetSafeNormal();
		CameraPitch = FMath::Clamp(
			CameraPitch + NaturalPitch * CameraPitchSensitivity,
			MinimumCameraPitch,
			MaximumCameraPitch);
	}

	ApplyGravityAlignedCamera();
}

void ATinyverseCharacter::DoJumpStart()
{
	// signal the character to jump
	Jump();
}

void ATinyverseCharacter::DoJumpEnd()
{
	// signal the character to stop jumping
	StopJumping();
}

FVector ATinyverseCharacter::GetLocalUp() const
{
	const FVector GravityDirection = GetCharacterMovement()->GetGravityDirection();

	if (GravityDirection.IsNearlyZero())
	{
		return GetActorUpVector().GetSafeNormal();
	}

	return -GravityDirection.GetSafeNormal();
}

void ATinyverseCharacter::InitializeGravityAlignedCamera()
{
	CameraBoom->bUsePawnControlRotation = false;
	CameraBoom->SetAbsolute(false, true, false);

	const FVector LocalUp = GetLocalUp();
	CameraYawForward = FVector::VectorPlaneProject(GetActorForwardVector(), LocalUp).GetSafeNormal();

	if (CameraYawForward.IsNearlyZero())
	{
		FVector CameraRight;
		LocalUp.FindBestAxisVectors(CameraYawForward, CameraRight);
	}

	PreviousLocalUp = LocalUp;
	CameraPitch = FMath::Clamp(InitialCameraPitch, MinimumCameraPitch, MaximumCameraPitch);
	bGravityAlignedCameraInitialized = true;

	ApplyGravityAlignedCamera();
}

void ATinyverseCharacter::AlignCameraFrameToUp(const FVector& LocalUp)
{
	if (!bGravityAlignedCameraInitialized)
	{
		InitializeGravityAlignedCamera();
		return;
	}

	const FQuat UpDelta = FQuat::FindBetweenNormals(PreviousLocalUp, LocalUp);
	CameraYawForward = UpDelta.RotateVector(CameraYawForward);
	CameraYawForward = FVector::VectorPlaneProject(CameraYawForward, LocalUp).GetSafeNormal();

	if (CameraYawForward.IsNearlyZero())
	{
		CameraYawForward = FVector::VectorPlaneProject(GetActorForwardVector(), LocalUp).GetSafeNormal();
	}

	if (CameraYawForward.IsNearlyZero())
	{
		FVector CameraRight;
		LocalUp.FindBestAxisVectors(CameraYawForward, CameraRight);
	}

	PreviousLocalUp = LocalUp;
}

void ATinyverseCharacter::UpdateAutomaticCamera(float DeltaSeconds, const FVector& LocalUp)
{
	TimeSinceLastCameraInput += DeltaSeconds;

	if (!bEnableAutomaticCamera || TimeSinceLastCameraInput < AutomaticCameraDelay)
	{
		return;
	}

	const FVector TangentVelocity = FVector::VectorPlaneProject(GetVelocity(), LocalUp);
	const bool bAllowAutomaticYaw = ForwardMovementInput >= -KINDA_SMALL_NUMBER;

	if (bAllowAutomaticYaw && TangentVelocity.SizeSquared() >= FMath::Square(AutomaticCameraMinimumSpeed))
	{
		const FVector DesiredForward = FVector::VectorPlaneProject(GetActorForwardVector(), LocalUp).GetSafeNormal();
		const float ForwardDot = FMath::Clamp(
			FVector::DotProduct(CameraYawForward, DesiredForward),
			-1.0f,
			1.0f);
		const float SignedAngle = FMath::Atan2(
			FVector::DotProduct(FVector::CrossProduct(CameraYawForward, DesiredForward), LocalUp),
			ForwardDot);
		const float YawAlpha = 1.0f - FMath::Exp(-AutomaticCameraYawSpeed * DeltaSeconds);

		CameraYawForward = FQuat(LocalUp, SignedAngle * YawAlpha)
			.RotateVector(CameraYawForward)
			.GetSafeNormal();
	}

	CameraPitch = FMath::FInterpTo(
		CameraPitch,
		FMath::Clamp(AutomaticCameraPitch, MinimumCameraPitch, MaximumCameraPitch),
		DeltaSeconds,
		AutomaticCameraPitchSpeed);
}

void ATinyverseCharacter::UpdateCharacterFacing(float DeltaSeconds, const FVector& LocalUp)
{
	FVector TangentFacing = FVector::VectorPlaneProject(DesiredFacingDirection, LocalUp).GetSafeNormal();

	if (TangentFacing.IsNearlyZero())
	{
		TangentFacing = FVector::VectorPlaneProject(GetActorForwardVector(), LocalUp).GetSafeNormal();
	}

	if (TangentFacing.IsNearlyZero())
	{
		return;
	}

	DesiredFacingDirection = TangentFacing;

	const FQuat TargetRotation = FRotationMatrix::MakeFromXZ(TangentFacing, LocalUp).ToQuat();
	const float TurnAlpha = 1.0f - FMath::Exp(-CharacterTurnSpeed * DeltaSeconds);
	const FQuat SmoothedRotation = FQuat::Slerp(GetActorQuat(), TargetRotation, TurnAlpha).GetNormalized();

	SetActorRotation(SmoothedRotation);
}

void ATinyverseCharacter::ApplyGravityAlignedCamera()
{
	if (CameraBoom == nullptr || !bGravityAlignedCameraInitialized)
	{
		return;
	}

	const FVector LocalUp = GetLocalUp();
	const FVector TangentForward = FVector::VectorPlaneProject(CameraYawForward, LocalUp).GetSafeNormal();
	const FQuat SurfaceRotation = FRotationMatrix::MakeFromXZ(TangentForward, LocalUp).ToQuat();
	const FQuat PitchRotation = FRotator(CameraPitch, 0.0f, 0.0f).Quaternion();

	CameraBoom->TargetOffset = LocalUp * CameraTargetHeight;
	CameraBoom->SetWorldRotation(SurfaceRotation * PitchRotation);
}
