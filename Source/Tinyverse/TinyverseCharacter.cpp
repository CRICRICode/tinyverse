// Copyright Epic Games, Inc. All Rights Reserved.

#include "TinyverseCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/GameInstance.h"
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
#include "TinyverseSaveSubsystem.h"

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

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UTinyverseSaveSubsystem* SaveSubsystem =
			GameInstance->GetSubsystem<UTinyverseSaveSubsystem>())
		{
			SaveSubsystem->TryRestorePlayer(this);
		}
	}

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

void ATinyverseCharacter::RestoreCoinCount(int32 SavedCoinCount)
{
	CoinCount = FMath::Max(0, SavedCoinCount);
	OnCoinCountChanged.Broadcast(CoinCount);
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

	UWorld* World = GetWorld();

	if (!IsValid(World))
	{
		return;
	}

	const FVector CharacterLocation = GetActorLocation();
	ATinyverseGravityPlanet* PreviousGravityPlanet = ActiveGravityPlanet.Get();

	// Rebuild the candidates so a missed overlap cannot leave gravity disabled.
	NearbyGravityPlanets.Reset();

	for (TActorIterator<ATinyverseGravityPlanet> PlanetIterator(World);
		PlanetIterator;
		++PlanetIterator)
	{
		ATinyverseGravityPlanet* Planet = *PlanetIterator;

		if (IsValid(Planet)
			&& Planet->IsLocationWithinInfluence(CharacterLocation))
		{
			NearbyGravityPlanets.Add(Planet);
		}
	}

	ATinyverseGravityPlanet* BestPlanet = nullptr;
	float BestScore = TNumericLimits<float>::Max();
	float ActiveScore = TNumericLimits<float>::Max();
	bool bActivePlanetIsCandidate = false;

	for (ATinyverseGravityPlanet* Planet : NearbyGravityPlanets)
	{
		if (!IsValid(Planet))
		{
			continue;
		}

		const float SelectionScore =
			Planet->GetGravitySelectionScoreAtLocation(CharacterLocation);

		if (Planet == ActiveGravityPlanet)
		{
			ActiveScore = SelectionScore;
			bActivePlanetIsCandidate =
				SelectionScore < TNumericLimits<float>::Max();
		}

		if (SelectionScore < BestScore)
		{
			BestScore = SelectionScore;
			BestPlanet = Planet;
		}
	}

	ATinyverseGravityPlanet* GroundPlanet = nullptr;

	if (Movement->IsMovingOnGround())
	{
		GroundPlanet = Cast<ATinyverseGravityPlanet>(
			Movement->CurrentFloor.HitResult.GetActor());
	}

	const bool bGroundedOnGravityPlanet = IsValid(GroundPlanet)
		&& GroundPlanet->IsLocationWithinInfluence(CharacterLocation);

	if (bGroundedOnGravityPlanet)
	{
		NearbyGravityPlanets.AddUnique(GroundPlanet);
		ActiveGravityPlanet = GroundPlanet;
		bActivePlanetIsCandidate = true;
	}

	if (!bGroundedOnGravityPlanet
		&& !bActivePlanetIsCandidate)
	{
		ActiveGravityPlanet = BestPlanet;
	}
	else if (!bGroundedOnGravityPlanet
		&& IsValid(BestPlanet)
		&& BestPlanet != ActiveGravityPlanet
		&& BestScore * FMath::Max(1.0f, GravitySwitchRatio) < ActiveScore)
	{
		ActiveGravityPlanet = BestPlanet;
	}

	const bool bGravitySourceChanged =
		PreviousGravityPlanet != ActiveGravityPlanet.Get();

#if !UE_BUILD_SHIPPING
	if (bShowGravityDebug && bGravitySourceChanged)
	{
		UE_LOG(
			LogTinyverse,
			Display,
			TEXT("Gravity source changed for %s: %s -> %s (best score: %.4f)"),
			*GetNameSafe(this),
			*GetNameSafe(PreviousGravityPlanet),
			*GetNameSafe(ActiveGravityPlanet),
			BestScore);
	}
#endif

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

	Movement->SetGravityDirection(DesiredGravityDirection);

	if (!bHasPlanetGravityDirection)
	{
		VisualGravityDirection = DesiredGravityDirection;
	}
	else if (DeltaSeconds > 0.0f
		&& GravityDirectionRotationSpeed > 0.0f)
	{
		VisualGravityDirection = FMath::VInterpNormalRotationTo(
			VisualGravityDirection.GetSafeNormal(),
			DesiredGravityDirection,
			DeltaSeconds,
			GravityDirectionRotationSpeed);
	}
	else
	{
		VisualGravityDirection = DesiredGravityDirection;
	}

	if (VisualGravityDirection.IsNearlyZero())
	{
		VisualGravityDirection = DesiredGravityDirection;
	}

	VisualGravityDirection.Normalize();

	bHasPlanetGravityDirection = true;

	const float BaseGravity = FMath::Abs(World->GetGravityZ());

	Movement->GravityScale = BaseGravity > KINDA_SMALL_NUMBER
		                         ? CurrentGravityStrength / BaseGravity
		                         : 0.0f;

	if (bGravitySourceChanged
		&& Movement->IsMovingOnGround()
		&& !bGroundedOnGravityPlanet)
	{
		Movement->SetMovementMode(MOVE_Falling);
	}

#if !UE_BUILD_SHIPPING
	if (bShowGravityDebug)
	{
		const FVector ArrowStart = CharacterLocation;
		const FVector ArrowEnd = ArrowStart + DesiredGravityDirection * 180.0f;

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
	if (bHasPlanetGravityDirection && !VisualGravityDirection.IsNearlyZero())
	{
		return -VisualGravityDirection.GetSafeNormal();
	}

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
