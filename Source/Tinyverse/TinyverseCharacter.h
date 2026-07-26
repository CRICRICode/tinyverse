// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "TinyverseCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class ATinyverseGravityPlanet;
struct FInputActionValue;
class UTinyverseHealthComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 *  A simple player-controllable third person character
 *  Implements a controllable orbiting camera
 */
UCLASS(abstract)
class ATinyverseCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UTinyverseHealthComponent* HealthComponent;
	
protected:

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MouseLookAction;
	
	/**  Debug gravity arrow*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Debug|Gravity")
	bool bShowGravityDebug=false;

public:

	/** Constructor */
	ATinyverseCharacter();

	void RegisterGravityPlanet(ATinyverseGravityPlanet* GravityPlanet);
	void UnregisterGravityPlanet(ATinyverseGravityPlanet* GravityPlanet);

	UFUNCTION(BlueprintPure, Category="Movement|Planet|Gravity")
	ATinyverseGravityPlanet* GetActiveGravityPlanet() const;

protected:

	virtual void BeginPlay() override;

	virtual void Tick(float DeltaSeconds) override;

	/** Initialize input action bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called when movement input ends */
	void StopMove(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

public:

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles look inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoLook(float Yaw, float Pitch);

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();

public:

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	FORCEINLINE class UTinyverseHealthComponent* GetHealthComponent() const { return HealthComponent; }
private:
	void UpdatePlanetGravity(float DeltaSeconds);

	FVector GetLocalUp() const;

	void InitializeGravityAlignedCamera();

	void AlignCameraFrameToUp(const FVector& LocalUp);

	void UpdateAutomaticCamera(float DeltaSeconds, const FVector& LocalUp);

	void UpdateCharacterFacing(float DeltaSeconds, const FVector& LocalUp);

	void ApplyGravityAlignedCamera();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Planet", meta=(AllowPrivateAccess="true", ClampMin="0.01"))
	float CharacterTurnSpeed = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Planet|Gravity", meta=(AllowPrivateAccess="true"))
	bool bEnablePlanetGravity = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Planet|Gravity", meta=(AllowPrivateAccess="true", ClampMin="1.0"))
	float GravitySwitchRatio = 1.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Planet|Gravity", meta=(AllowPrivateAccess="true", ClampMin="0.0", Units="cm"))
	float GravitySurfaceCaptureDistance = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Planet|Gravity", meta=(AllowPrivateAccess="true", ClampMin="0.0", Units="deg/s"))
	float GravityDirectionRotationSpeed = 540.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Planet|Gravity", meta=(AllowPrivateAccess="true"))
	bool bLockGravitySourceWhileGrounded = true;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Movement|Planet|Gravity", meta=(AllowPrivateAccess="true"))
	TObjectPtr<ATinyverseGravityPlanet> ActiveGravityPlanet;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Movement|Planet|Gravity", meta=(AllowPrivateAccess="true"))
	float CurrentGravityStrength = 0.0f;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ATinyverseGravityPlanet>> NearbyGravityPlanets;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera|Planet", meta=(AllowPrivateAccess="true", ClampMin="0.01"))
	float CameraYawSensitivity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera|Planet", meta=(AllowPrivateAccess="true", ClampMin="0.01"))
	float CameraPitchSensitivity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera|Planet|Input", meta=(AllowPrivateAccess="true"))
	bool bInvertHorizontalCamera = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera|Planet|Input", meta=(AllowPrivateAccess="true"))
	bool bInvertVerticalCamera = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera|Planet", meta=(AllowPrivateAccess="true", ClampMin="-89.0", ClampMax="89.0"))
	float MinimumCameraPitch = -70.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera|Planet", meta=(AllowPrivateAccess="true", ClampMin="-89.0", ClampMax="89.0"))
	float MaximumCameraPitch = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera|Planet", meta=(AllowPrivateAccess="true", ClampMin="-89.0", ClampMax="89.0"))
	float InitialCameraPitch = -20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera|Planet|Automatic", meta=(AllowPrivateAccess="true"))
	bool bEnableAutomaticCamera = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera|Planet|Automatic", meta=(AllowPrivateAccess="true", ClampMin="0.0"))
	float AutomaticCameraDelay = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera|Planet|Automatic", meta=(AllowPrivateAccess="true", ClampMin="0.01"))
	float AutomaticCameraYawSpeed = 3.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera|Planet|Automatic", meta=(AllowPrivateAccess="true", ClampMin="0.01"))
	float AutomaticCameraPitchSpeed = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera|Planet|Automatic", meta=(AllowPrivateAccess="true", ClampMin="-89.0", ClampMax="89.0"))
	float AutomaticCameraPitch = -20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera|Planet|Automatic", meta=(AllowPrivateAccess="true", ClampMin="0.0"))
	float AutomaticCameraMinimumSpeed = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera|Planet|Composition", meta=(AllowPrivateAccess="true"))
	float CameraTargetHeight = 60.0f;

	FVector CameraYawForward = FVector::ForwardVector;

	FVector DesiredFacingDirection = FVector::ZeroVector;

	FVector PreviousLocalUp = FVector::UpVector;

	float CameraPitch = 0.0f;

	float TimeSinceLastCameraInput = 0.0f;

	float ForwardMovementInput = 0.0f;

	bool bGravityAlignedCameraInitialized = false;

	bool bHasPlanetGravityDirection = false;
};

