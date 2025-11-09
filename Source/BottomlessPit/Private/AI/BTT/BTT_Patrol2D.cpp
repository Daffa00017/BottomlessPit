// BTT_Patrol2D Implementation

#include "AI/BTT/BTT_Patrol2D.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"
#include "Enum/AIMovementState.h"
#include "Pawn/Enemy/CPP_EnemyParent.h" 

UBTT_Patrol2D::UBTT_Patrol2D()
{
	NodeName = TEXT("Pick Patrol Location (2D)");
	bNotifyTick = false;
	PatrolProbability = 0.75f;
	MinPatrolDistance = 200.f;
	MaxPatrolDistance = 600.f;
	MaxAttempts = 6;
	GroundTraceDistance = 400.f;
	bRequireGround = true;
	SameFloorTolerance = 24.f;
	bPrimeWalkOnPick = true;
    bDebugTraceLoc = true;
}

EBTNodeResult::Type UBTT_Patrol2D::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* /*NodeMemory*/)
{
    APawn* ControlledPawn = OwnerComp.GetAIOwner() ? OwnerComp.GetAIOwner()->GetPawn() : nullptr;
    if (!ControlledPawn) return EBTNodeResult::Failed;

    UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
    if (!BB) return EBTNodeResult::Failed;

    UWorld* World = ControlledPawn->GetWorld();
    if (!World) return EBTNodeResult::Failed;

    FVector PawnLoc = ControlledPawn->GetActorLocation();
    FVector PatrolLoc = PawnLoc; // Default: stay in place

    // Probability: maybe don't patrol at all
    if (FMath::FRand() > PatrolProbability)
    {
        UE_LOG(LogTemp, Log, TEXT("[Patrol2D] Staying put, no patrol triggered."));
        BB->SetValueAsVector(PatrolLocationKey.SelectedKeyName, PawnLoc);
        return EBTNodeResult::Succeeded;
    }

    bool bFoundValid = false;
    FVector CandidateGround = PawnLoc;

    // Try multiple times to find a valid patrol point
    for (int i = 0; i < MaxAttempts; i++)
    {
        float OffsetX = FMath::FRandRange(MinPatrolDistance, MaxPatrolDistance);
        OffsetX *= FMath::RandBool() ? 1.f : -1.f; // Random left/right

        FVector CandidateLoc = PawnLoc + FVector(OffsetX, 0.f, 0.f);

        // Use your helper function to find ground
        bool bHitGround = FindGroundAtPoint(World, CandidateLoc.X, CandidateLoc.Y, PawnLoc.Z, CandidateGround);

        if (bRequireGround && !bHitGround)
        {
            UE_LOG(LogTemp, Warning, TEXT("[Patrol2D] Attempt %d: No ground hit at %s"), i + 1, *CandidateLoc.ToString());
            continue;
        }

        // Optional: Check if Z height difference is small enough to be same floor
        if (FMath::Abs(CandidateGround.Z - PawnLoc.Z) > SameFloorTolerance)
        {
            UE_LOG(LogTemp, Warning, TEXT("[Patrol2D] Attempt %d: Too big Z diff at %s"), i + 1, *CandidateLoc.ToString());
            continue;
        }

        // Valid patrol point found
        PatrolLoc = CandidateGround;
        bFoundValid = true;
        break;
    }

    // If no valid point found, stay put
    if (!bFoundValid)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Patrol2D] No valid patrol point after %d attempts, staying put."), MaxAttempts);
        PatrolLoc = PawnLoc;
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("[Patrol2D] Patrol point set to: %s"), *PatrolLoc.ToString());
    }

    BB->SetValueAsVector(PatrolLocationKey.SelectedKeyName, PatrolLoc);
    return EBTNodeResult::Succeeded;
}


bool UBTT_Patrol2D::FindGroundAtPoint(UWorld* World, float X, float Y, float StartZ, FVector& OutGround) const
{
    if (!World) return false;

    const FVector Start = FVector(X, Y, StartZ);
    const FVector End = FVector(X, Y, StartZ - GroundTraceDistance);

    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(Patrol_FindGround), false, nullptr);

    // Debug line every trace
    if (bDebugTraceLoc)
    {
        DrawDebugLine(World, Start, End, FColor::Yellow, false, 2.f, 0, 1.5f);
    }

    const bool bHit = World->LineTraceSingleByChannel(Hit, Start, End, GroundChannel, Params);

    if (bHit)
    {
        OutGround = Hit.ImpactPoint;

        if (bDebugTraceLoc)
        {
            DrawDebugPoint(World, Hit.ImpactPoint, 10.f, FColor::Green, false, 2.f);
        }

        return true;
    }
    else
    {
        if (bDebugTraceLoc)
        {
            DrawDebugPoint(World, End, 10.f, FColor::Red, false, 2.f);
        }

        // fallback so AI doesn't freeze forever
        OutGround = FVector(X, Y, StartZ);
        return false;
    }
}



