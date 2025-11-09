// PaperZD Parent CPP


#include "CPP_PaperZDParentCharacter.h"
#include "SharedHeader/RR_MovementTypes.h"
#include "GameFramework/Controller.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"


ACPP_PaperZDParentCharacter::ACPP_PaperZDParentCharacter()
{
    PrimaryActorTick.bCanEverTick = true; // Enable Tick
    bLookingBack = false;
    UseControlRotation = false;

    bUseControllerRotationYaw = true;
    GetCharacterMovement()->bOrientRotationToMovement = false;
    GetCharacterMovement()->RotationRate = FRotator(0.f, 0.f, 500.f); // Fast Z rotation
}

void ACPP_PaperZDParentCharacter::BeginPlay()
{
    Super::BeginPlay();

    MyMovementComp = GetCharacterMovement();
    MyControllerComp = GetController();
}

void ACPP_PaperZDParentCharacter::ChangeMovementState(EE_PlayerMovementState NewState)
{
    if (NewState == CurrentMovementState) return;
    const EE_PlayerMovementState Old = CurrentMovementState;
    CurrentMovementState = NewState;

    // clear any timers if you keep them here (IdleConfirm etc.), or leave to child
    OnMovementStateChanged.Broadcast(Old, CurrentMovementState);
    OnMovementStateChangedNative(Old, CurrentMovementState);
}

void ACPP_PaperZDParentCharacter::ScheduleIdleConfirm()
{
    if (!IsGrounded()) return; // never idle mid-air
    ClearIdleConfirmTimer();
    GetWorldTimerManager().SetTimer(
        IdleConfirmTimer, this, &ACPP_PaperZDParentCharacter::ConfirmIdle,
        IdleConfirmDelay, false
    );
}

void ACPP_PaperZDParentCharacter::ConfirmIdle()
{
    if (!IsGrounded()) return;
    ChangeMovementState(EE_PlayerMovementState::Idle);
}

void ACPP_PaperZDParentCharacter::ClearIdleConfirmTimer()
{
    GetWorldTimerManager().ClearTimer(IdleConfirmTimer);
}

void ACPP_PaperZDParentCharacter::HandleAxisIntent(int32 CombinedAxis)
{
    // CombinedAxis: -1 = left, +1 = right, 0 = neutral/both
    if (CombinedAxis != 0)
    {
        // User is providing directional intent
        ClearIdleConfirmTimer();
        if (IsGrounded() && CurrentMovementState != EE_PlayerMovementState::Walk)
        {
            ChangeMovementState(EE_PlayerMovementState::Walk);
        }
    }
    else
    {
        // No directional intent (or both held) -> schedule idle confirmation
        if (IsGrounded())
        {
            ScheduleIdleConfirm();
        }
    }
}

bool ACPP_PaperZDParentCharacter::IsGrounded() const
{
    const UCharacterMovementComponent* CM = GetCharacterMovement();
    // Conservative: require moving on ground; don't rely on child flags
    return (CM && CM->IsMovingOnGround() && !CM->IsFalling());
}

void ACPP_PaperZDParentCharacter::BroadcastMoveAxis(float Axis, float SpeedX)
{
    OnMoveAxisUpdatedDelegate.Broadcast(Axis, SpeedX);
}



void ACPP_PaperZDParentCharacter::Tick(float DeltaTime)
{

    
    Super::Tick(DeltaTime);
    if (UseControlRotation) {

        if (CustomTimeDilation <= 0.01f || !MyMovementComp)
            return;

        // Check X velocity directly (faster than IsNearlyZero())
        const float XVelocity = MyMovementComp->Velocity.X;
        if (FMath::Abs(XVelocity) > FacingDeadZone) // Deadzone
            {
                SetFacingDirection(XVelocity < 0); // Directly set direction
            }
    }
}


void ACPP_PaperZDParentCharacter::UpdateControllerRotation()
{
    if (CustomTimeDilation > 0.01f)
    {
        const float XVelocity = MyMovementComp->Velocity.X;
        if (FMath::Abs(XVelocity) > FacingDeadZone)
            SetFacingDirection(XVelocity < 0);
    }
    // When not moving, do nothing - keeps last facing direction
}

void ACPP_PaperZDParentCharacter::SetFacingDirection(bool bNewFacingLeft)
{
    if (bLookingBack == bNewFacingLeft) return; // Early out if no change

    bLookingBack = bNewFacingLeft;

    if (MyControllerComp) // Use cached controller
    {
        const FRotator NewRotation(0.f, bLookingBack ? 180.f : 0.f, 0.f);
        MyControllerComp->SetControlRotation(NewRotation);
        SetActorRotation(NewRotation);
    }
}

void ACPP_PaperZDParentCharacter::StartHitStop(float Duration)
{
    CustomTimeDilation = 0.01f; // Freeze this actor only
    GetWorld()->GetTimerManager().SetTimer(
        HitStopTimerHandle,
        this,
        &ACPP_PaperZDParentCharacter::ClearHitStop,
        Duration,
        false
    );
}

void ACPP_PaperZDParentCharacter::ClearHitStop()
{
    CustomTimeDilation = 1.0f; // Restore
}



