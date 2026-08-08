// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "TinyverseCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UFMODEvent;
class ATinyverseGravityPlanet;
struct FInputActionValue;
class UTinyverseHealthComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FTinyverseCoinCountChangedSignature,
	int32,
	CoinCount);

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
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Debug|Gravity", meta=(ToolTip="Shows the active gravity direction and logs every gravity source change. Development builds only."))
	bool bShowGravityDebug=false;

public:

	/** Constructor */
	ATinyverseCharacter();

	UPROPERTY(BlueprintAssignable, Category="Collectibles|Events")
	FTinyverseCoinCountChangedSignature OnCoinCountChanged;

	UFUNCTION(BlueprintCallable, Category="Collectibles")
	void CollectCoin();

	UFUNCTION(BlueprintPure, Category="Collectibles")
	int32 GetCoinCount() const;

	void RestoreCoinCount(int32 SavedCoinCount);

	void RegisterGravityPlanet(ATinyverseGravityPlanet* GravityPlanet);
	void UnregisterGravityPlanet(ATinyverseGravityPlanet* GravityPlanet);

	UFUNCTION(BlueprintPure, Category="Movement|Planet|Gravity")
	ATinyverseGravityPlanet* GetActiveGravityPlanet() const;

	UFUNCTION(BlueprintCallable, Category="Audio|Footsteps")
	void PlayFootstep(FName FootSocketName);

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

	UFMODEvent* ResolveFMODEvent(
		TObjectPtr<UFMODEvent>& EventReference,
		const FString& EventPath);

	void PlayFMODEvent(
		TObjectPtr<UFMODEvent>& EventReference,
		const FString& EventPath,
		const FTransform& EventTransform);

	FVector GetLocalUp() const;

	void InitializeGravityAlignedCamera();

	void AlignCameraFrameToUp(const FVector& LocalUp);

	void UpdateAutomaticCamera(float DeltaSeconds, const FVector& LocalUp);

	void UpdateCharacterFacing(float DeltaSeconds, const FVector& LocalUp);

	void ApplyGravityAlignedCamera();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Planet", meta=(AllowPrivateAccess="true", ClampMin="0.01"))
	float CharacterTurnSpeed = 12.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio|Footsteps", meta=(AllowPrivateAccess="true", ToolTip="FMOD event played by footstep animation notifies. If empty, the event path below is resolved at runtime."))
	TObjectPtr<UFMODEvent> FootstepEvent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio|Footsteps", meta=(AllowPrivateAccess="true", ToolTip="Fallback FMOD Studio path used when Footstep Event is not assigned in the Character Blueprint."))
	FString FootstepEventPath = TEXT("event:/SFX/Player/FootSteps");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio|Footsteps", meta=(AllowPrivateAccess="true", ClampMin="0.0", Units="cm/s", ToolTip="Footstep notifies are ignored below this character speed."))
	float MinimumFootstepSpeed = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio|Player", meta=(AllowPrivateAccess="true", ToolTip="FMOD event played when the character successfully starts a jump."))
	TObjectPtr<UFMODEvent> JumpEvent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio|Player", meta=(AllowPrivateAccess="true", ToolTip="Fallback FMOD Studio path used when Jump Event is not assigned in the Character Blueprint."))
	FString JumpEventPath = TEXT("event:/SFX/Player/Jump");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio|Collectibles", meta=(AllowPrivateAccess="true", ToolTip="FMOD event played whenever a coin is collected."))
	TObjectPtr<UFMODEvent> CoinCollectedEvent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio|Collectibles", meta=(AllowPrivateAccess="true", ToolTip="Fallback FMOD Studio path used when Coin Collected Event is not assigned in the Character Blueprint."))
	FString CoinCollectedEventPath = TEXT("event:/SFX/Collectible/Coin");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio|Player", meta=(AllowPrivateAccess="true", ToolTip="FMOD event played when a coin milestone actually restores or increases health."))
	TObjectPtr<UFMODEvent> HealthRewardEvent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio|Player", meta=(AllowPrivateAccess="true", ToolTip="Fallback FMOD Studio path used when Health Reward Event is not assigned in the Character Blueprint."))
	FString HealthRewardEventPath = TEXT("event:/SFX/Player/Health");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Collectibles", meta=(AllowPrivateAccess="true", ClampMin="1"))
	int32 CoinsPerHealthReward = 10;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Collectibles", meta=(AllowPrivateAccess="true"))
	int32 CoinCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Planet|Gravity", meta=(AllowPrivateAccess="true", ToolTip="Enables the custom planetary gravity system for this character."))
	bool bEnablePlanetGravity = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Planet|Gravity", meta=(AllowPrivateAccess="true", ClampMin="1.0", ToolTip="How much better a new planet's selection score must be before replacing the active planet. Use 1.0 for no hysteresis."))
	float GravitySwitchRatio = 1.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Planet|Gravity", meta=(AllowPrivateAccess="true", ClampMin="0.0", Units="deg/s", ToolTip="Rotation speed used to visually align the character and camera with the active gravity direction. Physical gravity changes immediately."))
	float GravityDirectionRotationSpeed = 540.0f;

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

	FVector VisualGravityDirection = FVector::DownVector;

	bool bGravityAlignedCameraInitialized = false;

	bool bHasPlanetGravityDirection = false;
};

