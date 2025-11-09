// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PaperZDCharacter.h"
#include "InputTriggers.h"
#include "InputActionValue.h"
#include "SharedHeader/RR_MovementTypes.h"
#include "CPP_PaperZDParentCharacter.generated.h"

/**
 * 
 */
UCLASS()
class BOTTOMLESSPIT_API ACPP_PaperZDParentCharacter : public APaperZDCharacter
{
	GENERATED_BODY()


public:
    ACPP_PaperZDParentCharacter();

    // Function to update controller rotation based on movement
    UFUNCTION(BlueprintCallable, Category = "Movement") // Added UFUNCTION
        void UpdateControllerRotation();

    UFUNCTION(BlueprintCallable, Category = "Movement") // Added UFUNCTION
        void SetFacingDirection(bool bNewFacingLeft);

    UFUNCTION(BlueprintCallable, Category = "Combat|Effects")
    void StartHitStop(float Duration);

    UFUNCTION(BlueprintCallable, Category = "Combat|Effects")
    void ClearHitStop();

    UCharacterMovementComponent* MyMovementComp;
    AController* MyControllerComp;

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    // === Shared movement state (parent owns it) ===
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    EE_PlayerMovementState CurrentMovementState = EE_PlayerMovementState::Idle;

    UPROPERTY(BlueprintAssignable, Category = "Movement|Delegates")
    FOnMovementStateChanged OnMovementStateChanged;

    UPROPERTY(BlueprintAssignable, Category = "Anim|Delegates")
    FOnMoveAxisUpdated OnMoveAxisUpdatedDelegate;

    // Optional: put the action delegates here too so all children reuse them
    UPROPERTY(BlueprintAssignable, Category = "Input|Delegates") FOnDashStarted      OnDashStartedEvent;
    UPROPERTY(BlueprintAssignable, Category = "Input|Delegates") FOnDashCompleted    OnDashCompletedEvent;
    UPROPERTY(BlueprintAssignable, Category = "Input|Delegates") FOnJumpStarted      OnJumpStartedEvent;

    UPROPERTY(BlueprintAssignable, Category = "Input|Delegates") FOnJumpTriggered      OnJumpTriggeredEvent;

    UPROPERTY(BlueprintAssignable, Category = "Input|Delegates") FOnJumpCompleted    OnJumpCompletedEvent;
    UPROPERTY(BlueprintAssignable, Category = "Input|Delegates") FOnShootStarted     OnShootStartedEvent;
    UPROPERTY(BlueprintAssignable, Category = "Input|Delegates") FOnShootCompleted   OnShootCompletedEvent;
    UPROPERTY(BlueprintAssignable, Category = "Input|Delegates") FOnSlideRollStarted OnSlideRollStartedEvent;
    UPROPERTY(BlueprintAssignable, Category = "Input|Delegates") FOnSlideRollCompleted OnSlideRollCompletedEvent;
    UPROPERTY(BlueprintAssignable, Category = "Input|Delegates") FOnAimStarted       OnAimStartedEvent;
    UPROPERTY(BlueprintAssignable, Category = "Input|Delegates") FOnAimCompleted     OnAimCompletedEvent;

    UFUNCTION(BlueprintCallable, Category = "Movement")
    void ChangeMovementState(EE_PlayerMovementState NewState);

    // Helpers your children can call to broadcast axis/speed
    void BroadcastMoveAxis(float Axis, float SpeedX);

    // ===== Tunables shared by all characters =====
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Tuning")
    float IdleConfirmDelay = 0.08f;

    /** If AnimSpeedX > this, we keep/enter Walk even if inputs flicker */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Tuning")
    float WalkSpeedThreshold = 50.f;

    /** Epsilon for Axis change before broadcasting OnMoveAxisUpdated */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim|Tuning")
    float AxisEpsilon = 0.001f;

    /** Epsilon for Speed change before broadcasting (if you add that logic) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim|Tuning")
    float SpeedEpsilon = 2.0f;

    /** Simple helper: -1/0/+1 axis intent -> manages Walk/Idle timers */
    UFUNCTION(BlueprintCallable, Category = "Movement")
    void HandleAxisIntent(int32 CombinedAxis /* -1,0,+1 */);

    /** Ground check shared by all characters */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Movement")
    virtual bool IsGrounded() const;



protected:

    float LastRotationUpdateTime = -1.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Controls", meta = (DisplayPriority = 1))
    float RotationUpdateInterval = 0.5f;

    // Variable to track if we're looking back
    UPROPERTY(BlueprintReadWrite, Category = "Movement")
    bool bLookingBack;

    UPROPERTY(BlueprintReadWrite, Category = "Movement")
    bool UseControlRotation;

    // HitStop
    FTimerHandle HitStopTimerHandle;

    UPROPERTY(EditAnywhere, Category = "Movement")
    float FacingDeadZone = 10.0f;

    uint8 ebLookingBack : 1;

    // Sprite Shake
    FTimerHandle ShakeTimerHandle;
    FVector OriginalSpriteLocation;
    float CurrentShakeIntensity;
    float ShakeTimeRemaining;

    // If children need a hook on state changes without binding to the multicast:
    virtual void OnMovementStateChangedNative(EE_PlayerMovementState OldState, EE_PlayerMovementState NewState) {}

    // Idle confirm lifecycle
    UFUNCTION(BlueprintCallable, Category = "Movement|Idle")
    void ScheduleIdleConfirm();

    UFUNCTION(BlueprintCallable, Category = "Movement|Idle")
    void ConfirmIdle();

    UFUNCTION(BlueprintCallable, Category = "Movement|Idle")
    void ClearIdleConfirmTimer();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|State")
    int MaxJumpCount = 2;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|State", meta = (ClampMin = "0", ClampMax = "5", DisplayPriority = "1"))
    int CurrentJumpCount;

    //Input Variable
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Input|State")
    bool bLeftHeld = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Input|State")
    bool bRightHeld = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Input|State")
    bool bWasBothHeld = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Input|State")
    bool bDashInput = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Input|State")
    bool bJumpInput = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Input|State")
    bool bShootInput = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Input|State")
    bool bSlideRollInput = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Input|State")
    bool bAimInput = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|State")
    bool bIsFalling = false;

    static float GetScalar01(const FInputActionValue& Value)
    {
        // Bool ? 0/1, 1D Axis ? normalized, others ? magnitude
        if (Value.GetValueType() == EInputActionValueType::Boolean)
            return Value.Get<bool>() ? 1.f : 0.f;
        if (Value.GetValueType() == EInputActionValueType::Axis1D)
            return Value.Get<float>();
        if (Value.GetValueType() == EInputActionValueType::Axis2D)
            return Value.Get<FVector2D>().Size();
        if (Value.GetValueType() == EInputActionValueType::Axis3D)
            return Value.Get<FVector>().Size();
        return 0.f;
    }


private:
    FTimerHandle IdleConfirmTimer;

    

};


