//BlpEnemies Implementation

#include "Pawn/Enemy/CPP_BlpEnemies.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetSystemLibrary.h"

ACPP_BlpEnemies::ACPP_BlpEnemies()
{
    PrimaryActorTick.bCanEverTick = true;
}

void ACPP_BlpEnemies::BeginPlay()
{
    Super::BeginPlay();
}

void ACPP_BlpEnemies::ConfigureCollisionForOverlapPlayer()
{
    if (UCapsuleComponent* Cap = FindComponentByClass<UCapsuleComponent>())
    {
        Cap->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        //Cap->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);   // player overlaps (damage)
        Cap->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
        Cap->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
        // if you have a custom Player channel, set it to Overlap here too
    }
}

static float BlpCapsuleHalfHeight(const ACPP_BlpEnemies* Self)
{
    if (const UCapsuleComponent* Cap = Self->FindComponentByClass<UCapsuleComponent>())
    {
        return Cap->GetScaledCapsuleHalfHeight();
    }
    return 44.f; // safe fallback
}

void ACPP_BlpEnemies::ActivateFromPool(const FVector& WorldPos)
{
    Super::ActivateFromPool(WorldPos);

    ConfigureCollisionForOverlapPlayer();

    AxisVec = (LaneAxis == EBlpLaneAxis::X) ? FVector(1, 0, 0) : FVector(0, 1, 0);

    if (UFloatingPawnMovement* Mv = FindComponentByClass<UFloatingPawnMovement>())
    {
        Mv->MaxSpeed = MoveSpeed;
        Mv->StopMovementImmediately();
    }

    // Snap once if we’re a walker
    if (Kind == EBPEnemyKind::Walker)
    {
        FVector G;
        if (WalkableGroundAt(GetActorLocation(), GroundDownUU, G))
        {
            FVector L = GetActorLocation();
            L.Z = G.Z + BlpCapsuleHalfHeight(this) + 0.5f;
            SetActorLocation(L, false);
        }
    }

    BaseZ = GetActorLocation().Z;
    FlipCooldown = 0.f;
    bAIOn = true;
}

void ACPP_BlpEnemies::DeactivateToPool()
{
    Super::DeactivateToPool();

    bAIOn = false;
    if (UFloatingPawnMovement* Mv = FindComponentByClass<UFloatingPawnMovement>())
    {
        Mv->StopMovementImmediately();
    }
}


void ACPP_BlpEnemies::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!IsActive() || !bAIOn) return;

    FlipCooldown = FMath::Max(0.f, FlipCooldown - DeltaSeconds);

    switch (Kind)
    {
    case EBPEnemyKind::Walker: WalkerTick(DeltaSeconds); break;
    case EBPEnemyKind::Flyer:  FlyerTick(DeltaSeconds);  break;
    }
}

bool ACPP_BlpEnemies::WallAhead() const
{
    const FVector Start = GetActorLocation() + FVector(0, 0, 8.f);
    const FVector End = Start + AxisVec * (float)Dir * WallLookAheadUU;

    FHitResult Hit;
    FCollisionQueryParams P(TEXT("Blp_WallAhead"), false, this);
    P.AddIgnoredActor(this);

    const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, GroundChannel, P);
    const bool bWall = bHit && Hit.bBlockingHit && FMath::Abs(Hit.ImpactNormal.Z) < 0.4f;

    if (bDebugAI)
    {
        DrawDebugLine(GetWorld(), Start, End, bWall ? FColor::Red : FColor::Green,
            false, DebugLineTime, 0, 1.f);
        if (bHit)
        {
            DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 5.f, 10, FColor::Cyan,
                false, DebugLineTime);
            // normal
            DrawDebugLine(GetWorld(), Hit.ImpactPoint, Hit.ImpactPoint + Hit.ImpactNormal * 25.f,
                FColor::Cyan, false, DebugLineTime, 0, 1.f);
        }
    }

    return bWall;
}

bool ACPP_BlpEnemies::WalkableGroundAt(const FVector& XY, float DownDist, FVector& OutHitPoint) const
{
    const FVector Start = FVector(XY.X, XY.Y, GetActorLocation().Z + 80.f);
    const FVector End = Start + FVector(0, 0, -DownDist - 120.f);

    FHitResult Hit;
    FCollisionQueryParams P(TEXT("Blp_Ground"), false, this);
    P.AddIgnoredActor(this);

    const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, GroundChannel, P);
    const bool bWalkable = bHit && Hit.bBlockingHit && Hit.ImpactNormal.Z >= WalkableMinNormalZ;

    if (bDebugAI)
    {
        DrawDebugLine(GetWorld(), Start, End, bWalkable ? FColor::Green : FColor::Red,
            false, DebugLineTime, 0, 1.f);
        if (bHit)
        {
            DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 5.f, 10,
                bWalkable ? FColor::Green : FColor::Yellow, false, DebugLineTime);
        }
    }

    if (bWalkable)
    {
        OutHitPoint = Hit.ImpactPoint;
        return true;
    }
    return false;
}

void ACPP_BlpEnemies::WalkerTick(float dt)
{
    UFloatingPawnMovement* Mv = FindComponentByClass<UFloatingPawnMovement>();
    if (!Mv) return;

    const float HH = BlpCapsuleHalfHeight(this);
    const float Skin = 0.5f;

    // 1) face travel dir for visuals (logic uses AxisVec so this is cosmetic)
    const float TargetYaw = (LaneAxis == EBlpLaneAxis::X)
        ? ((Dir > 0) ? 0.f : 180.f)
        : ((Dir > 0) ? 90.f : -90.f);
    FRotator R = GetActorRotation();
    if (!FMath::IsNearlyEqual(R.Yaw, TargetYaw, 0.1f)) { R.Yaw = TargetYaw; SetActorRotation(R); }

    // 2) snap to ground at current XY (place capsule center HH above the surface)
    FVector GroundPoint;
    if (WalkableGroundAt(GetActorLocation(), GroundDownUU, GroundPoint))
    {
        FVector L = GetActorLocation();
        L.Z = GroundPoint.Z + HH + Skin;   // <-- key change (was +1)
        SetActorLocation(L, false);
    }

    // 3) anticipate: edge OR wall ahead -> flip (debounced)
    const FVector Ahead = GetActorLocation() + AxisVec * (float)Dir * GroundAheadUU;
    FVector GroundAheadHit;
    const bool bGroundAhead = WalkableGroundAt(Ahead, GroundDownUU, GroundAheadHit);
    const bool bWall = WallAhead();

    if (FlipCooldown <= 0.f && (!bGroundAhead || bWall))
    {
        Dir *= -1;
        FlipCooldown = FlipCooldownSeconds;
        // small nudge along new dir so we don't sample the same wall next tick
        SetActorLocation(GetActorLocation() + AxisVec * (float)Dir * 2.f, false);
    }

    // 4) move via FloatingPawnMovement (like your BT task)
    Mv->AddInputVector(AxisVec * (float)Dir);

    // 5) HUD (reuse variables; do NOT redeclare)
    if (bDebugAI && GEngine)
    {
        const FString ChanStr = UEnum::GetValueAsString(GroundChannel.GetValue());
        const FString AxisStr = (LaneAxis == EBlpLaneAxis::X) ? TEXT("X") : TEXT("Y");
        const FString Msg = FString::Printf(
            TEXT("[Walker] Dir=%d  Wall=%s  GroundAhead=%s  Axis=%s  Chan=%s"),
            Dir, bWall ? TEXT("Y") : TEXT("N"), bGroundAhead ? TEXT("Y") : TEXT("N"),
            *AxisStr, *ChanStr
        );
        GEngine->AddOnScreenDebugMessage(
            (int32)((uintptr_t)this & 0xFFFFFF), 0.f, FColor::Yellow, Msg, false);
    }
}

void ACPP_BlpEnemies::FlyerTick(float dt)
{
    UFloatingPawnMovement* Mv = FindComponentByClass<UFloatingPawnMovement>();
    if (!Mv) return;

    // only flip on wall ahead (debounced)
    if (FlipCooldown <= 0.f && WallAhead())
    {
        Dir *= -1;
        FlipCooldown = FlipCooldownSeconds;
        SetActorLocation(GetActorLocation() + AxisVec * (float)Dir * 2.f, false); // tiny push-off
    }

    // horizontal movement
    Mv->AddInputVector(AxisVec * (float)Dir);

    // hold altitude with a small hover (like your BT intent)
    if (HoverAmp > 0.f && HoverHz > 0.f)
    {
        FVector L = GetActorLocation();
        const float T = GetWorld()->TimeSeconds;
        L.Z = BaseZ + HoverAmp * FMath::Sin(2.f * PI * HoverHz * T);
        SetActorLocation(L, false);
    }

    // visuals
    const float TargetYaw = (LaneAxis == EBlpLaneAxis::X)
        ? ((Dir > 0) ? 0.f : 180.f)
        : ((Dir > 0) ? 90.f : -90.f);
    FRotator R = GetActorRotation();
    if (!FMath::IsNearlyEqual(R.Yaw, TargetYaw, 0.1f))
    {
        R.Yaw = TargetYaw;
        SetActorRotation(R);
    }
}



