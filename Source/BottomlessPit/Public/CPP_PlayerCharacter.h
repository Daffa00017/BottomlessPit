// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CPP_PaperZDParentCharacter.h"
#include "SharedHeader/RR_MovementTypes.h"   
#include "Components/CapsuleComponent.h"
#include "InputTriggers.h"
#include "InputActionValue.h"
#include "Delegates/DelegateCombinations.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "PaperFlipbookComponent.h"
#include "PaperFlipbook.h"
#include "CPP_PlayerCharacter.generated.h"

class UPaperZDAnimationComponent;
class UPaperZDAnimSequence;
class UPaperZDAnimInstance;
class UInputMappingContext;
class UInputAction;
class UEnhancedInputComponent;
class UEnhancedInputLocalPlayerSubsystem;


/**
 * 
 */
UCLASS()
class BOTTOMLESSPIT_API ACPP_PlayerCharacter : public ACPP_PaperZDParentCharacter
{
	GENERATED_BODY()

public:
	ACPP_PlayerCharacter();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	void UpdateRotationBasedOnCursor();
	virtual void Landed(const FHitResult& Hit) override;


	UPROPERTY(EditAnywhere,BlueprintReadWrite, category = "Arm")
	bool UseUpdateArmAim = false;

	// Call every Tick (or only when aiming, up to you)
	UFUNCTION(BlueprintCallable, Category = "Aim")
	void UpdateArmAim();

	// Spawn transform aligned with the aim (muzzle)
	UFUNCTION(BlueprintCallable, Category = "Aim")
	FTransform GetMuzzleSpawnTransform() const;

	// Broadcast after aim computed (AnimBP can listen)
	UPROPERTY(BlueprintAssignable, Category = "Aim|Events")
	FAimUpdatedSignature OnAimUpdated;

	// === Components Arm Rotate===
	// Pivot we rotate (only around Z/Yaw)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arm")
	TObjectPtr<USceneComponent> ArmPivot;

	// Arm/hand flipbook sprite
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arm")
	TObjectPtr<UPaperFlipbookComponent> ArmFlipbook;

	// Muzzle for projectile spawn
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arm")
	TObjectPtr<USceneComponent> ArmMuzzle;

	// === Tunables ===
	// Default flipbook (set in BP or defaults)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Arm|Config")
	TObjectPtr<UPaperFlipbook> DefaultArmFlipbook;

	// Where the arm attaches on the body (local, X right, Z up)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arm|Config")
	FVector ArmPivotOffset = FVector(10.f, 0.f, 40.f);

	// Where bullets come out, relative to pivot (local)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arm|Config")
	FVector MuzzleLocalOffset = FVector(30.f, 0.f, 0.f);

	// Allowed aim window when facing RIGHT (degrees around forward = 0°)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arm|Clamp")
	float ArmMinAngleDeg = -80.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arm|Clamp")
	float ArmMaxAngleDeg = 80.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arm|Clamp")
	bool bClampAim = true;

	// Hide the arm if aiming too far behind
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arm|Clamp", meta = (ClampMin = "0", ClampMax = "179.9"))
	float BehindThresholdDeg = 95.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arm|Clamp")
	bool bHideWhenBehind = true;

	// Drive the arm flipbook via PaperZD
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arm|Anim")
	TObjectPtr<UPaperZDAnimationComponent> ArmAnim;

	// Optional: default anim instance class just for the arm (if you use state machines)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Arm|Anim")
	TSubclassOf<UPaperZDAnimInstance> ArmAnimInstanceClass;

	// Optional: a default PaperZD sequence to play on the arm (idle/aim pose)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Arm|Anim")
	TObjectPtr<UPaperZDAnimSequence> DefaultArmAnimSequence;

	// If you have a socket on the body flipbook, attach the arm there
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arm|Attach")
	FName ArmAttachSocketName = NAME_None; // e.g. "ShoulderSocket"
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arm|Attach")
	FName ArmMuzzleSocketName = FName("Muzzle"); // or NAME_None if you don't use a socket

	// --- Arm setup & tuning ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arm|Setup")
	FVector ArmPivotUserOffset = FVector::ZeroVector;    // extra local offset on top of socket snap

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arm|Setup")
	FVector ArmMuzzleUserOffset = FVector::ZeroVector;   // extra local offset on top of MuzzleLocalOffset

	// Bias the final aim so muzzle lines up with the cursor visually.
	// Convention: +Pitch => tilt DOWN (UE pitch positive is down)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arm|Aim")
	float AimPitchOffsetDeg = 0.f;

	// Optional: if your arm art needs a tiny roll twist
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arm|Aim")
	float AimRollOffsetDeg = 0.f;

	// If you ever want to bias yaw (usually keep 0 for Paper2D)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arm|Aim")
	float AimYawOffsetDeg = 0.f;

	UFUNCTION(BlueprintPure, Category = "Arm|Aim")
	bool GetArmAimDirection(FVector& OutDir) const;

protected:

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UPROPERTY(EditAnywhere, Category = "Player Controls", meta = (DisplayPriority = 1))
	float PlayerDeadZone = 20.0f;

	UPROPERTY(EditAnywhere, Category = "Player Controls", meta = (DisplayPriority = 1))
	float CursorRotationDeadZone = 50.f;

	UPROPERTY()
	FVector2D ViewportSize;
	float ThresholdRatio;

	// --- Arm | Runtime state readable by AnimBP or gameplay ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arm|State")
	float ArmAimAngleDeg = 0.f;          // final pitch we applied (deg)

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arm|State")
	FVector ArmAimDir = FVector::ForwardVector; // forward on XZ from pitch

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arm|State")
	bool bArmIsAiming = false;           // e.g., tie to bAimInput

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arm|State")
	bool bArmFacingLeft = false;         // mirrors your bLookingBack

	// --- Arm | Tuning ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arm|Clamp")
	bool bInvertAimPitch = true;         // makes mouse up => arm up



	// Distance from character screen position to trigger rotation
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Controls",
		meta = (ClampMin = "0", ClampMax = "1000", DisplayPriority = "1"))
	float CursorRotationThreshold = 100.f;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Player Controls", meta = (DisplayPriority = 1))
	APlayerController* CachedPlayerController;

	UPROPERTY(EditAnywhere, Category = "Movement|Tuning")
	float WalkCommitDelay = 0.03f;

	UFUNCTION(BlueprintNativeEvent, Category = "Movement")
	void CustomEventOnLanded(FHitResult HitResult);

	bool bIsChangingState = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
	float MoveAxis = 0.f;                

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim", meta = (AllowPrivateAccess = "true"))
	float AnimSpeedX = 0.f;       

	float LastAxisSent = 9999.f;     // sentinel
	float LastSpeedXSent = -1.f;

	bool GetCursorWorldOnCharacterPlane(FVector& OutWorld) const;

	// Facing-aware clamp/mirror
	float GetFinalArmAngleDegrees(float RawAngleDeg, bool bFacingLeft) const;

	// If you want to make these live-editable in editor
	virtual void OnConstruction(const FTransform& Transform) override;

	// Helper to re-apply offsets cleanly (constructor/BeginPlay/OnConstruction)
	void ApplyArmAttachmentOffsets();
	

private:
	FORCEINLINE bool GetCharacterScreenPosition(FVector2D& OutPos) const;
	UEnhancedInputComponent* CachedEIC = nullptr;

	//Input Variable

	float TimeSinceInputReleased = 0.f;
	float LastNonZeroAxis = 0.f;
	double LastInputPressedTime = 0.0;
	FTimerHandle IdleConfirmTimer;
	FTimerHandle WalkCommitTimer;

	//UPROPERTY()


	//Input Asset
	UPROPERTY(EditAnywhere, Category = "Input", meta = (DisplayPriority = 1))
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, Category = "Input", meta = (DisplayPriority = 1))
	UInputAction* MoveLeftAction = nullptr;

	UPROPERTY(EditAnywhere, Category = "Input", meta = (DisplayPriority = 0))
	UInputAction* MoveRightAction = nullptr;

	UPROPERTY(EditAnywhere, Category = "Input", meta = (DisplayPriority = 0))
	UInputAction* DashAction = nullptr;

	UPROPERTY(EditAnywhere, Category = "Input", meta = (DisplayPriority = 0))
	UInputAction* JumpAction = nullptr;

	UPROPERTY(EditAnywhere, Category = "Input", meta = (DisplayPriority = 0))
	UInputAction* ShootAction = nullptr;

	UPROPERTY(EditAnywhere, Category = "Input", meta = (DisplayPriority = 0))
	UInputAction* SlideRollAction = nullptr;

	UPROPERTY(EditAnywhere, Category = "Input", meta = (DisplayPriority = 0))
	UInputAction* AimAction = nullptr;

	EE_PlayerMovementState LastMovementState = EE_PlayerMovementState::Idle;

	// cached XZ unit vector (for muzzle rotation)
	FVector LastAimDir = FVector::ForwardVector;


	//Input Function

	//MoveAction (A & D) Input
	void OnMoveLeftTriggered(const FInputActionValue& Value);
	void OnMoveRightTriggered(const FInputActionValue& Value);
	void OnMoveLeftStarted(const FInputActionValue& Value);
	void OnMoveRightStarted(const FInputActionValue& Value);
	void OnMoveLeftCompleted(const FInputActionValue& Value);
	void OnMoveRightCompleted(const FInputActionValue& Value);
		//MoveAction Function To move the player
		void RecomputeAxisAndSpeed();

	// Dash
	void OnDashStarted(const FInputActionValue& Value);
	void OnDashTriggered(const FInputActionValue& Value);
	void OnDashCompleted(const FInputActionValue& Value);

	// Jump
	void OnJumpStarted(const FInputActionValue& Value);
	void OnJumpTriggered(const FInputActionValue& Value);
	void OnJumpCompleted(const FInputActionValue& Value);

	// Shoot
	void OnShootStarted(const FInputActionValue& Value);
	void OnShootTriggered(const FInputActionValue& Value);
	void OnShootCompleted(const FInputActionValue& Value);

	// Slide/Roll
	void OnSlideRollStarted(const FInputActionValue& Value);
	void OnSlideRollTriggered(const FInputActionValue& Value);
	void OnSlideRollCompleted(const FInputActionValue& Value);

	// Aim (usually a hold-to-aim)
	void OnAimStarted(const FInputActionValue& Value);
	void OnAimTriggered(const FInputActionValue& Value);
	void OnAimCompleted(const FInputActionValue& Value);

	void HandleMove(bool bIsLeftKey, ETriggerEvent Phase, const FInputActionValue& Value);

	FORCEINLINE int GetCombinedAxis() const
	{
		const int L = bLeftHeld ? 1 : 0;
		const int R = bRightHeld ? 1 : 0;
		return R - L;   // -1 if Left, +1 if Right, 0 if none or both
	}

	static FORCEINLINE bool RR_IsAirState(EE_PlayerMovementState S)
	{
		return S == EE_PlayerMovementState::Jump
			|| S == EE_PlayerMovementState::WallSlide
			|| S == EE_PlayerMovementState::Dash; // add other air states if needed
	}

};



