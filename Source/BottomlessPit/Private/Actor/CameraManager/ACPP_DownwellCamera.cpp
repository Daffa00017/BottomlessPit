// Fill out your copyright notice in the Description page of Project Settings.


#include "Actor/CameraManager/ACPP_DownwellCamera.h"
#include "GameFramework/Actor.h"

AACPP_DownwellCamera::AACPP_DownwellCamera()
{
    // keep ticking from base
}

FVector AACPP_DownwellCamera::ComputeDesiredLocation(float DeltaSeconds) const
{
    FVector Desired = GetActorLocation();

    // 1) Lock horizontal axis: pure Downwell (no left/right follow)
    Desired.X = WellCenterX;

    // 2) Build a vertical target based on player + auto-scroll
    float TargetZ = Desired.Z; // fallback

    if (FollowTarget)
    {
        const FVector T = FollowTarget->GetActorLocation();
        // Aim below the player by DownLead
        TargetZ = T.Z - DownLead;
    }

    // Apply auto-scrolling (down = decreasing Z)
    TargetZ -= AutoScrollSpeed * DeltaSeconds;

    // Optionally smooth vertical *downward* motion
    float NewZ = (FollowDownLerp >= 1.f)
        ? TargetZ
        : FMath::Lerp(Desired.Z, TargetZ, FMath::Clamp(FollowDownLerp, 0.f, 1.f));

    // 3) Never move camera up: clamp by the lowest Z reached so far
    if (NewZ > LastCameraZ)
    {
        NewZ = LastCameraZ; // trying to go up? stay where we were
    }

    Desired.Z = NewZ;

    // 4) Keep strict 2D if needed
    if (bLockYToZero) Desired.Y = 0.f;

    // 5) Update the “lowest so far”
    LastCameraZ = Desired.Z;

    return Desired;
}

void AACPP_DownwellCamera::NotifyWorldZLoop(float DeltaZ)
{
    // Snap the camera actor by the same world delta (Z only).
    AddActorWorldOffset(FVector(0.f, 0.f, DeltaZ), false, nullptr, ETeleportType::TeleportPhysics);

    // IMPORTANT: raise the upward clamp so camera is allowed to move up again.
    LastCameraZ += DeltaZ;

    // If you cache desired/previous locations for smoothing, reset them here to avoid lerp-back.
    // Example (rename to your vars if you have them):
    // CachedDesired = GetActorLocation();
    // bHasCachedDesired = true;
}

void AACPP_DownwellCamera::BeginPlay()
{
    Super::BeginPlay();
    // initialize “no-go-up” clamp to current Z
    LastCameraZ = GetActorLocation().Z;
    // Apply initial knobs
    SpringArm->TargetArmLength = ArmLength;
    SpringArm->AddLocalOffset(ArmLocalOffset);
    SpringArm->SetRelativeRotation(ArmRotation);
}





