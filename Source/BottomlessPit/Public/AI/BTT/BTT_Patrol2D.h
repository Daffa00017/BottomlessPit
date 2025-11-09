// BTT_Patrol2D Header
#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTT_Patrol2D.generated.h"


UCLASS()
class BOTTOMLESSPIT_API UBTT_Patrol2D : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTT_Patrol2D();

	/** Blackboard Vector key to write patrol point (consumed by MoveToLocation2D_Snap) */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	struct FBlackboardKeySelector PatrolLocationKey;

	/** Probability [0..1] that we will actually choose a patrol point (else stay idle). */
	UPROPERTY(EditAnywhere, Category = "Patrol", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PatrolProbability = 0.75f;

	/** Minimum and maximum horizontal distance (abs on X) from pawn to candidate. */
	UPROPERTY(EditAnywhere, Category = "Patrol", meta = (ClampMin = "0.0"))
	float MinPatrolDistance = 200.f;

	UPROPERTY(EditAnywhere, Category = "Patrol", meta = (ClampMin = "0.0"))
	float MaxPatrolDistance = 600.f;

	/** Number of attempts to find a valid point (try different directions / jitter) */
	UPROPERTY(EditAnywhere, Category = "Patrol", meta = (ClampMin = "1"))
	int32 MaxAttempts = 6;

	/** Downward trace distance to find ground under candidate point */
	UPROPERTY(EditAnywhere, Category = "Patrol", meta = (EditCondition = "bRequireGround"))
	float GroundTraceDistance = 400.f;

	/** Whether to require valid ground below the candidate */
	UPROPERTY(EditAnywhere, Category = "Patrol")
	bool bRequireGround = true;

	/** How much Z difference is allowed to still be considered "same platform" (units in world Z) */
	UPROPERTY(EditAnywhere, Category = "Patrol")
	float SameFloorTolerance = 24.f;

	/** Channel to use for ground check */
	UPROPERTY(EditAnywhere, Category = "Patrol")
	TEnumAsByte<ECollisionChannel> GroundChannel = ECC_Visibility;

	/** If true set the Pawn's AIMoveState to Walk immediately when we pick a patrol spot */
	UPROPERTY(EditAnywhere, Category = "Patrol")
	bool bPrimeWalkOnPick = true;

	/** If true set the Pawn's AIMoveState to Walk immediately when we pick a patrol spot */
	UPROPERTY(EditAnywhere, Category = "Debug")
	bool bDebugTraceLoc = true;

protected:
	virtual EBTNodeResult::Type ExecuteTask(class UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

private:
	/** trace down from (X,Y,StartZ) to find a ground impact; returns true and fills OutGround if hit */
	bool FindGroundAtPoint(UWorld* World, float X, float Y, float StartZ, FVector& OutGround) const;
};



