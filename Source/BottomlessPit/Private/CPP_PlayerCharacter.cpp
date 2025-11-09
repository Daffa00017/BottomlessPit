// Fill out your copyright notice in the Description page of Project Settings.

//CPP_PlayerCharacter
#include "CPP_PlayerCharacter.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "Kismet/KismetMathLibrary.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/SceneComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "PaperZDAnimationComponent.h"
//#include "PaperZDAnimSequence.h"
#include "PaperZDAnimInstance.h"
#include "PaperFlipbookComponent.h"
#include "GameFramework/PlayerController.h"

ACPP_PlayerCharacter::ACPP_PlayerCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    ViewportSize = FVector2D::ZeroVector;

    USceneComponent* const AttachParent =
        GetCapsuleComponent() ? static_cast<USceneComponent*>(GetCapsuleComponent())
        : GetRootComponent();

    // --- ArmPivot ---
    ArmPivot = CreateDefaultSubobject<USceneComponent>(TEXT("PC_ArmPivot"));
    ArmPivot->SetupAttachment(AttachParent);
    ArmPivot->SetRelativeLocation(ArmPivotOffset);

    // --- ArmFlipbook ---
    ArmFlipbook = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("PC_ArmFlipbook"));
    ArmFlipbook->SetupAttachment(ArmPivot);
    ArmFlipbook->SetRelativeLocation(FVector::ZeroVector);
    ArmFlipbook->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ArmFlipbook->SetGenerateOverlapEvents(false);

    // --- ArmMuzzle ---
    ArmMuzzle = CreateDefaultSubobject<USceneComponent>(TEXT("PC_ArmMuzzle"));
    ArmMuzzle->SetupAttachment(ArmPivot);
    ArmMuzzle->SetRelativeLocation(MuzzleLocalOffset);

    // --- PaperZD anim component that will drive ArmFlipbook ---
    ArmAnim = CreateDefaultSubobject<UPaperZDAnimationComponent>(TEXT("PC_ArmAnim"));
    // NOTE: we set its RenderComponent in BeginPlay/PostInit, once all components are fully created.
}

void ACPP_PlayerCharacter::BeginPlay()
{
    Super::BeginPlay();

    // --- your existing input & viewport setup ---
    CachedPlayerController = GetWorld()->GetFirstPlayerController();
    if (CachedPlayerController)
    {
        int32 ViewportWidth = 0, ViewportHeight = 0;
        CachedPlayerController->GetViewportSize(ViewportWidth, ViewportHeight);

        ViewportSize = FVector2D((float)ViewportWidth, (float)ViewportHeight);

        if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
            ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(CachedPlayerController->GetLocalPlayer()))
        {
            if (DefaultMappingContext)
            {
                Subsystem->AddMappingContext(DefaultMappingContext, 0);
            }
        }
    }

    ThresholdRatio = CursorRotationThreshold / 1920.f;

    // --- find the main/body flipbook created by your PaperZD parent ---
    UPaperFlipbookComponent* BodyFlipbook = nullptr;

    // If your parent exposes a getter like GetSprite()/GetRenderComponent(), prefer that.
    // BodyFlipbook = GetSprite();

    if (!BodyFlipbook)
    {
        TArray<UPaperFlipbookComponent*> Flipbooks;
        GetComponents<UPaperFlipbookComponent>(Flipbooks);
        for (UPaperFlipbookComponent* FB : Flipbooks)
        {
            if (FB && FB != ArmFlipbook) { BodyFlipbook = FB; break; }
        }
    }

    // --- reattach ArmPivot under the body flipbook (optionally to a socket) ---
    if (BodyFlipbook && ArmPivot)
    {
        const FName ShoulderSocket = ArmAttachSocketName; // can be NAME_None

        // Snap so we don't keep old offsets
        if (ShoulderSocket != NAME_None)
        {
            ArmPivot->AttachToComponent(
                BodyFlipbook,
                FAttachmentTransformRules::SnapToTargetNotIncludingScale,
                ShoulderSocket
            );
        }
        else
        {
            ArmPivot->AttachToComponent(
                BodyFlipbook,
                FAttachmentTransformRules::SnapToTargetNotIncludingScale
            );
        }

        // Force local zero if you want exact (0,0,0) relative to the parent/socket
        ArmPivot->SetRelativeLocation(FVector::ZeroVector);
        ArmPivot->SetRelativeRotation(FRotator::ZeroRotator);
        // keep parent scale; pivot scale should usually be 1
        ArmPivot->SetRelativeScale3D(FVector::OneVector);

        // If you still want an offset later, set it here instead of constructor:
        // ArmPivot->AddRelativeLocation(ArmPivotOffset);
        }

    if (ArmMuzzle && ArmFlipbook)
    {
        if (ArmMuzzleSocketName != NAME_None)
        {
            // Snap to the flipbook's socket
            ArmMuzzle->AttachToComponent(
                ArmFlipbook,
                FAttachmentTransformRules::SnapToTargetNotIncludingScale,
                ArmMuzzleSocketName
            );
            // ensure clean local xform
            ArmMuzzle->SetRelativeLocation(FVector::ZeroVector);
            ArmMuzzle->SetRelativeRotation(FRotator::ZeroRotator);
            ArmMuzzle->SetRelativeScale3D(FVector::OneVector);
        }
        else
        {
            // No socket: just attach to the flipbook and use your offset
            ArmMuzzle->AttachToComponent(
                ArmFlipbook,
                FAttachmentTransformRules::KeepRelativeTransform
            );
            ArmMuzzle->SetRelativeLocation(MuzzleLocalOffset);
            ArmMuzzle->SetRelativeRotation(FRotator::ZeroRotator);
        }
    }
    if (ArmPivot)
    {
        // world rotation won’t be affected by parent negative scale flips
        ArmPivot->SetUsingAbsoluteRotation(true);  // public API; don’t touch bAbsoluteRotation directly
    }

    ApplyArmAttachmentOffsets();
}

void ACPP_PlayerCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    const float CurrentTime = GetWorld()->GetTimeSeconds();
    const bool bShouldUpdate = (CurrentTime - LastRotationUpdateTime) >= RotationUpdateInterval;

    if (IsPlayerControlled() && UseUpdateArmAim)
    {
        UpdateRotationBasedOnCursor();
        UpdateArmAim();
    }

    if (IsPlayerControlled() && UseControlRotation)
    {
        LastRotationUpdateTime = CurrentTime; // Update timestamp
        UpdateControllerRotation();
    }
}

void ACPP_PlayerCharacter::UpdateRotationBasedOnCursor()
{
    if (!CachedPlayerController) return;

    // Get mouse and character positions in screen space
    FVector2D MousePos;
    FVector2D CharScreenPos;

    if (!CachedPlayerController->GetMousePosition(MousePos.X, MousePos.Y) ||
        !CachedPlayerController->ProjectWorldLocationToScreen(
            GetActorLocation(),  // Using simple actor location instead of capsule
            CharScreenPos,
            true))
    {
        return;
    }

    // Simple X comparison - no viewport size needed
    const bool bNewDirection = (MousePos.X < CharScreenPos.X);

    if (bLookingBack != bNewDirection)
    {
        SetFacingDirection(bNewDirection);
    
    }
}

bool ACPP_PlayerCharacter::GetArmAimDirection(FVector& OutDir) const
{
    if (!ArmPivot) return false;

    // Read the rotation you already set in UpdateArmAim() (mirror/offset/clamp applied)
    FVector Dir = ArmPivot->GetComponentRotation().Vector();
    Dir.Y = 0.f; // lock to side-scroller plane
    if (!Dir.Normalize()) return false;

    OutDir = Dir;
    return true;
}

void ACPP_PlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    CachedEIC = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);

    // bind both actions on Started & Completed so we track held state
    if (MoveLeftAction)
    {
        CachedEIC->BindAction(MoveLeftAction, ETriggerEvent::Started, this, &ACPP_PlayerCharacter::OnMoveLeftStarted);
        CachedEIC->BindAction(MoveLeftAction, ETriggerEvent::Triggered, this, &ACPP_PlayerCharacter::OnMoveLeftTriggered);
        CachedEIC->BindAction(MoveLeftAction, ETriggerEvent::Completed, this, &ACPP_PlayerCharacter::OnMoveLeftCompleted);
    }
    if (MoveRightAction)
    {
        CachedEIC->BindAction(MoveRightAction, ETriggerEvent::Started, this, &ACPP_PlayerCharacter::OnMoveRightStarted);
        CachedEIC->BindAction(MoveRightAction, ETriggerEvent::Triggered, this, &ACPP_PlayerCharacter::OnMoveRightTriggered);
        CachedEIC->BindAction(MoveRightAction, ETriggerEvent::Completed, this, &ACPP_PlayerCharacter::OnMoveRightCompleted);
    }

    // DASH
    if (DashAction)
    {
        CachedEIC->BindAction(DashAction, ETriggerEvent::Started, this, &ThisClass::OnDashStarted);
        CachedEIC->BindAction(DashAction, ETriggerEvent::Triggered, this, &ThisClass::OnDashTriggered);
        CachedEIC->BindAction(DashAction, ETriggerEvent::Completed, this, &ThisClass::OnDashCompleted);
    }

    // JUMP
    if (JumpAction)
    {
        CachedEIC->BindAction(JumpAction, ETriggerEvent::Started, this, &ThisClass::OnJumpStarted);
        CachedEIC->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ThisClass::OnJumpTriggered);
        CachedEIC->BindAction(JumpAction, ETriggerEvent::Completed, this, &ThisClass::OnJumpCompleted);
    }

    // SHOOT
    if (ShootAction)
    {
        CachedEIC->BindAction(ShootAction, ETriggerEvent::Started, this, &ThisClass::OnShootStarted);
        CachedEIC->BindAction(ShootAction, ETriggerEvent::Triggered, this, &ThisClass::OnShootTriggered);
        CachedEIC->BindAction(ShootAction, ETriggerEvent::Completed, this, &ThisClass::OnShootCompleted);
    }

    // SLIDE/ROLL
    if (SlideRollAction)
    {
        CachedEIC->BindAction(SlideRollAction, ETriggerEvent::Started, this, &ThisClass::OnSlideRollStarted);
        CachedEIC->BindAction(SlideRollAction, ETriggerEvent::Triggered, this, &ThisClass::OnSlideRollTriggered);
        CachedEIC->BindAction(SlideRollAction, ETriggerEvent::Completed, this, &ThisClass::OnSlideRollCompleted);
    }

    // AIM (hold to aim)
    if (AimAction)
    {
        CachedEIC->BindAction(AimAction, ETriggerEvent::Started, this, &ThisClass::OnAimStarted);
        CachedEIC->BindAction(AimAction, ETriggerEvent::Triggered, this, &ThisClass::OnAimTriggered);
        CachedEIC->BindAction(AimAction, ETriggerEvent::Completed, this, &ThisClass::OnAimCompleted);
    }
}

void ACPP_PlayerCharacter::Landed(const FHitResult& Hit)
{
    CurrentJumpCount = 0;
    Super::Landed(Hit);
    bIsFalling = false;
    ClearIdleConfirmTimer();
    CustomEventOnLanded(Hit);

    // resolve to ground locomotion
    if (bLeftHeld || bRightHeld)
        ChangeMovementState(EE_PlayerMovementState::Walk);
    else
        ChangeMovementState(EE_PlayerMovementState::Idle);
}

void ACPP_PlayerCharacter::UpdateArmAim()
{
    if (!ArmPivot) return;

    // 1) Cursor → intersection on side-scroller plane (Y = Actor.Y)
    FVector HitOnPlane;
    if (!GetCursorWorldOnCharacterPlane(HitOnPlane))
        return;

    // 2) EXACTLY like your BP: Start = ActorLocation (not pivot)
    const FVector ActorLoc = GetActorLocation();
    const FRotator Look = UKismetMathLibrary::FindLookAtRotation(ActorLoc, HitOnPlane);

    // 3) Build the same relative rot: Pitch from Look.Pitch, Yaw=0, Roll from Look.Roll
    float Pitch = Look.Pitch;
    float Roll = Look.Roll;

    // Keep BP feel: when facing left, mirror ONLY pitch so mouse up = arm up
    if (bLookingBack)
    {
        Pitch = -Pitch;
        // If your asset needs roll mirrored too, uncomment:
        // Roll = -Roll;
    }

    // 3.5) Apply your editor biases to line muzzle/cursor up
    Pitch += bLookingBack ? -AimPitchOffsetDeg : AimPitchOffsetDeg;
    const float Yaw = AimYawOffsetDeg;      // stays 0 for pure 2D setups
    Roll += AimRollOffsetDeg;

    // Optional clamp (after bias)
    if (bClampAim)
    {
        Pitch = FMath::Clamp(Pitch, ArmMinAngleDeg, ArmMaxAngleDeg);
    }

    // 4) Apply RELATIVE rotation (same as K2_SetRelativeRotation in your BP)
    const FRotator RelativeRot(/*Pitch*/ Pitch, /*Yaw*/ Yaw, /*Roll*/ Roll);
    ArmPivot->SetRelativeRotation(RelativeRot);

    // 5) Keep sprite flip purely visual (no double-rotation)
    if (ArmFlipbook)
    {
        ArmFlipbook->SetRelativeScale3D(FVector(bLookingBack ? -1.f : 1.f, 1.f, 1.f));
    }
}

FTransform ACPP_PlayerCharacter::GetMuzzleSpawnTransform() const
{
    const FVector SpawnLoc = ArmMuzzle
        ? ArmMuzzle->GetComponentLocation()
        : (ArmPivot ? ArmPivot->GetComponentLocation() : GetActorLocation());

    // Forward from the current pitch on Y axis (same basis as UpdateArmAim)
    const FRotator PivotRel = ArmPivot ? ArmPivot->GetRelativeRotation() : FRotator::ZeroRotator;
    FVector Forward = FRotationMatrix(FRotator(PivotRel.Pitch, 0.f, 0.f)).GetUnitAxis(EAxis::X);
    Forward.Y = 0.f; // keep on XZ plane
    Forward.Normalize();

    const FRotator AimRot = FRotationMatrix::MakeFromXZ(Forward, FVector::UpVector).Rotator();
    return FTransform(AimRot, SpawnLoc, FVector(1.f));
}

void ACPP_PlayerCharacter::CustomEventOnLanded_Implementation(FHitResult HitResult)
{
    // Optional: default behavior
    UE_LOG(LogTemp, Warning, TEXT("CustomEventOnLanded called in C++"));
}

bool ACPP_PlayerCharacter::GetCursorWorldOnCharacterPlane(FVector& OutWorld) const
{
    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (!PC) return false;

    FVector WL, WD;
    if (!PC->DeprojectMousePositionToWorld(WL, WD))
        return false;

    // Intersect with plane Y = Actor.Y, exactly as your BP math did
    const double ActorY = (double)GetActorLocation().Y;
    const double DeltaY = ActorY - (double)WL.Y;
    const double DirY = (double)WD.Y;

    // BP used SelectFloat( (DirY == 0) ? DeltaY : (DeltaY / DirY) )
    const double t = FMath::IsNearlyZero(DirY) ? DeltaY : (DeltaY / DirY);

    OutWorld = WL + WD * (float)t;
    return true;
}

float ACPP_PlayerCharacter::GetFinalArmAngleDegrees(float RawAngleDeg, bool bFacingLeft) const
{
    float Pitch = RawAngleDeg;

    // If your visuals are mirrored when facing left, invert pitch for “natural” wrist
    // (this matches what usually happens when scaling X to -1 for left).
    if (bFacingLeft)
    {
        Pitch = -Pitch;
    }

    // Optional clamp window, e.g. -80..+80
    if (bClampAim)
    {
        Pitch = FMath::Clamp(Pitch, ArmMinAngleDeg, ArmMaxAngleDeg);
    }

    return Pitch;
}

void ACPP_PlayerCharacter::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    ApplyArmAttachmentOffsets();
}

void ACPP_PlayerCharacter::ApplyArmAttachmentOffsets()
{
    // Pivot: apply extra local offset after whatever attachment you already did
    if (ArmPivot)
    {
        // Keep current attach; just ensure local offset is what you want
        ArmPivot->SetRelativeLocation(ArmPivotUserOffset);
    }

    // Muzzle: add user offset on top of your default
    if (ArmMuzzle)
    {
        ArmMuzzle->SetRelativeLocation(MuzzleLocalOffset + ArmMuzzleUserOffset);
    }
}

bool ACPP_PlayerCharacter::GetCharacterScreenPosition(FVector2D& OutPos) const
{
    return GetWorld()->GetFirstPlayerController()->ProjectWorldLocationToScreen(
        GetActorLocation() + FVector(0, 0, 50), // Small height offset
        OutPos
    );
}

void ACPP_PlayerCharacter::OnMoveLeftStarted(const FInputActionValue& Value)
{
    HandleMove(true , ETriggerEvent::Started  , Value);
}

void ACPP_PlayerCharacter::OnMoveLeftTriggered(const FInputActionValue& Value)
{
    HandleMove(true, ETriggerEvent::Triggered, Value);
}

void ACPP_PlayerCharacter::OnMoveLeftCompleted(const FInputActionValue& Value)
{
    HandleMove(true, ETriggerEvent::Completed, Value);
}

// === RIGHT (mirror) ===
void ACPP_PlayerCharacter::OnMoveRightStarted(const FInputActionValue& Value)
{
    HandleMove(false, ETriggerEvent::Started, Value);
}

void ACPP_PlayerCharacter::OnMoveRightTriggered(const FInputActionValue& Value)
{
    HandleMove(false, ETriggerEvent::Triggered, Value);
}

void ACPP_PlayerCharacter::OnMoveRightCompleted(const FInputActionValue& Value)
{
    HandleMove(false, ETriggerEvent::Completed, Value);
}

void ACPP_PlayerCharacter::RecomputeAxisAndSpeed()
{
    const int Combined = GetCombinedAxis();
    MoveAxis = float(Combined);              // -1, 0, +1

    if (Combined != 0)
        AddMovementInput(FVector(1, 0, 0), MoveAxis);

    const float NewSpeedX = FMath::Abs(GetVelocity().X);
    AnimSpeedX = NewSpeedX;

    if (!FMath::IsNearlyEqual(MoveAxis, LastAxisSent, AxisEpsilon))
    {
        LastAxisSent = MoveAxis;
        BroadcastMoveAxis(MoveAxis, AnimSpeedX);
    }
}

// === DASH ===
void ACPP_PlayerCharacter::OnDashStarted(const FInputActionValue& Value)
{
    bDashInput = true;
    OnDashStartedEvent.Broadcast(GetScalar01(Value));
    // TODO: actually start dash (set state, launch, i-frames, etc.)
}

void ACPP_PlayerCharacter::OnDashTriggered(const FInputActionValue& Value)
{
    bDashInput = true;
    // Optional: sustain dash if analog/hold dash
}

void ACPP_PlayerCharacter::OnDashCompleted(const FInputActionValue& /*Value*/)
{
    bDashInput = false;
    OnDashCompletedEvent.Broadcast();
    // TODO: end dash (clear state)
}

// === JUMP ===
void ACPP_PlayerCharacter::OnJumpStarted(const FInputActionValue& Value)
{
    if (CurrentJumpCount < MaxJumpCount) {
        bJumpInput = true;
        bIsFalling = true;
        OnJumpStartedEvent.Broadcast(GetScalar01(Value));
        ClearIdleConfirmTimer();
        // TODO: Call Jump(); or custom jump logic
        CurrentJumpCount = CurrentJumpCount + 1 ;
        Jump();
        ChangeMovementState(EE_PlayerMovementState::Jump);
        //UE_LOG(LogTemp, Warning, TEXT("JumpCount: %d"), CurrentJumpCount);
    }

}

void ACPP_PlayerCharacter::OnJumpTriggered(const FInputActionValue& Value)
{
    bJumpInput = true;
    // Optional: variable jump height (hold duration), coyote time, etc.
}

void ACPP_PlayerCharacter::OnJumpCompleted(const FInputActionValue& /*Value*/)
{
    bJumpInput = false;
    OnJumpCompletedEvent.Broadcast();
    // Optional: StopJumping(); for variable height
    StopJumping();
}

// === SHOOT ===
void ACPP_PlayerCharacter::OnShootStarted(const FInputActionValue& Value)
{
    bShootInput = true;
    OnShootStartedEvent.Broadcast(GetScalar01(Value));
    // TODO: start firing / spawn projectile / play montage
}

void ACPP_PlayerCharacter::OnShootTriggered(const FInputActionValue& Value)
{
    bShootInput = true;
    // Optional: auto-fire if action is set to "Repeat while held"
}

void ACPP_PlayerCharacter::OnShootCompleted(const FInputActionValue& /*Value*/)
{
    bShootInput = false;
    OnShootCompletedEvent.Broadcast();
    // TODO: stop firing, cooldown end, etc.
}

// === SLIDE / ROLL ===
void ACPP_PlayerCharacter::OnSlideRollStarted(const FInputActionValue& Value)
{
    bSlideRollInput = true;
    OnSlideRollStartedEvent.Broadcast(GetScalar01(Value));
    // TODO: start slide/roll
}

void ACPP_PlayerCharacter::OnSlideRollTriggered(const FInputActionValue& Value)
{
    bSlideRollInput = true;
    // Optional: sustain slide if analog
}

void ACPP_PlayerCharacter::OnSlideRollCompleted(const FInputActionValue& /*Value*/)
{
    bSlideRollInput = false;
    OnSlideRollCompletedEvent.Broadcast();
    // TODO: end slide/roll
}

// === AIM (hold to aim) ===
void ACPP_PlayerCharacter::OnAimStarted(const FInputActionValue& Value)
{
    bAimInput = true;
    OnAimStartedEvent.Broadcast(GetScalar01(Value));
    // Optional: bIsAiming = true; adjust FOV/speed/reticle
}

void ACPP_PlayerCharacter::OnAimTriggered(const FInputActionValue& Value)
{
    bAimInput = true;
    // Optional: continuous aim adjustments (e.g., cursor aim move)
}

void ACPP_PlayerCharacter::OnAimCompleted(const FInputActionValue& /*Value*/)
{
    bAimInput = false;
    OnAimCompletedEvent.Broadcast();
    // Optional: bIsAiming = false; restore FOV/speed
}

void ACPP_PlayerCharacter::HandleMove(bool bIsLeftKey, ETriggerEvent Phase, const FInputActionValue& Value)
{
    bool& ThisHeld = bIsLeftKey ? bLeftHeld : bRightHeld;
    bool& OtherHeld = bIsLeftKey ? bRightHeld : bLeftHeld;

    // update held flags + when to recompute
    switch (Phase)
    {
    case ETriggerEvent::Started:
        ThisHeld = true;
        break;

    case ETriggerEvent::Triggered:
        ThisHeld = true;
        RecomputeAxisAndSpeed(); // does AddMovementInput + OnMoveAxisUpdated via parent
        break;

    case ETriggerEvent::Completed:
        ThisHeld = false;
        RecomputeAxisAndSpeed();
        break;

    default:
        break;
    }

    const int Combined = GetCombinedAxis();               // -1,0,+1  (0 includes “both held”)
    const bool bBoth = (bLeftHeld && bRightHeld);

    // promote to Walk only if truly grounded and not currently an air state
    if (Combined != 0 && IsGrounded() && !RR_IsAirState(CurrentMovementState))
    {
        ClearIdleConfirmTimer();
        if (CurrentMovementState != EE_PlayerMovementState::Walk)
        {
            ChangeMovementState(EE_PlayerMovementState::Walk);
        }
    }

    // both-keys bookkeeping (same as before, just centralized)
    if (bBoth && !bWasBothHeld && IsGrounded())
    {
        ScheduleIdleConfirm();
        bWasBothHeld = true;
    }
    if (!bBoth && bWasBothHeld)
    {
        ClearIdleConfirmTimer();
        bWasBothHeld = false;
    }

    // when this key is released and the other isn’t held → consider idling
    if (Phase == ETriggerEvent::Completed && !OtherHeld)
    {
        if (IsGrounded()) ScheduleIdleConfirm();
    }
}






