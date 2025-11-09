// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/CPP_DownwellLiteCharacter.h"
#include "Kismet/KismetMathLibrary.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/SceneComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"

ACPP_DownwellLiteCharacter::ACPP_DownwellLiteCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    // Downwell defaults if needed
    GetCharacterMovement()->GravityScale = 1.0f;   // make it fall faster
    GetCharacterMovement()->AirControl = 1.0f;   // more air mobility
    GetCharacterMovement()->JumpZVelocity = JumpHeight;
}

void ACPP_DownwellLiteCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    const float CurrentTime = GetWorld()->GetTimeSeconds();
    const bool bShouldUpdate = (CurrentTime - LastRotationUpdateTime) >= RotationUpdateInterval;

    if (IsPlayerControlled() && UseControlRotation)
    {
        LastRotationUpdateTime = CurrentTime; // Update timestamp
        UpdateControllerRotation();
    }
}

void ACPP_DownwellLiteCharacter::JumpFunction()
{
    if (IsDead)
    {

    }
    else if (CurrentAmmoCount >= 0) {
        bIsFalling = true;
        ClearIdleConfirmTimer();
        if (IsGrounded())
        {
            GetCharacterMovement()->JumpZVelocity = JumpHeight;
        }
        else
        {
            StopJumping();
            if (IsDebugMode){

            }
            else{
            CurrentAmmoCount--;
            }
            GetCharacterMovement()->JumpZVelocity = JumpHeightMidAir;
        }
        Jump();
        ChangeMovementState(EE_PlayerMovementState::Jump);
        //UE_LOG(LogTemp, Warning, TEXT("CurrentAmmoCount: %d"), CurrentAmmoCount);
    

    }
}

void ACPP_DownwellLiteCharacter::SetMoveAxis(float InAxis)
{
    if (IsDead) 
        {

        }
    else
        {
            MoveAxis = FMath::Clamp(InAxis, -1.f, 1.f);

            // movement every frame because Triggered calls this continuously
            if (!FMath::IsNearlyZero(MoveAxis))
                {
                    AddMovementInput(FVector(1.f, 0.f, 0.f), MoveAxis);
                }

                const bool bAxisZero = FMath::IsNearlyZero(MoveAxis);
                const bool bWasZero = FMath::IsNearlyZero(PrevAxisForIdle);

                if (IsGrounded())
                    {
                        if (!bAxisZero)
                            {
                                ClearIdleConfirmTimer();
                                if (CurrentMovementState != EE_PlayerMovementState::Walk)
                                ChangeMovementState(EE_PlayerMovementState::Walk);
                            }
                        else if (!bWasZero) // only when transitioning to zero
                            {
                                ScheduleIdleConfirm();
                            }
                    }
                else
                    {
                        ClearIdleConfirmTimer();
                        if (bJumpInput && bIsFalling && CurrentMovementState != EE_PlayerMovementState::Jump)
                        ChangeMovementState(EE_PlayerMovementState::Jump);
                    }

            PrevAxisForIdle = MoveAxis;

        }
}

void ACPP_DownwellLiteCharacter::InputJumpPressed()
{
    bJumpInput = true;
    //JumpFunction();
    OnJumpStartedEvent.Broadcast(GetScalar01(1));
}

void ACPP_DownwellLiteCharacter::InputJumpTriggered()
{
    bJumpInput = true;
    OnJumpTriggeredEvent.Broadcast();
}

void ACPP_DownwellLiteCharacter::CustomEventOnLanded_Implementation(FHitResult HitResult)
{
    // Optional: default behavior
    UE_LOG(LogTemp, Warning, TEXT("CustomEventOnLanded called in C++"));
}

void ACPP_DownwellLiteCharacter::Landed(const FHitResult& Hit)
{
    Super::Landed(Hit);


    GetCharacterMovement()->GravityScale = OriGravityScale;
    CurrentAmmoCount = MaxAmmoCount ;
    bIsFalling = false;
    IsUpdatingGravity = false;
    ClearIdleConfirmTimer();
    CustomEventOnLanded(Hit);

    // resolve to ground locomotion
    if (bLeftHeld || bRightHeld)
        ChangeMovementState(EE_PlayerMovementState::Walk);
    else
        ChangeMovementState(EE_PlayerMovementState::Idle);
}

void ACPP_DownwellLiteCharacter::BeginPlay()
{
    Super::BeginPlay();


}

void ACPP_DownwellLiteCharacter::InputJumpReleased()
{
    bJumpInput = false;
    OnJumpCompletedEvent.Broadcast();
    // Optional: StopJumping(); for variable height
    StopJumping();
}

void ACPP_DownwellLiteCharacter::InputDownPressed()
{
    if (!IsGrounded())
    {
        OnDownSmashStartedEvent.Broadcast(GetScalar01(1));
        //UE_LOG(LogTemp, Warning, TEXT("Smash Ground"));
        IsSmashingDown = true;
    }
}

void ACPP_DownwellLiteCharacter::InputDownReleased()
{
    OnDownSmashCompletedEvent.Broadcast();
}



