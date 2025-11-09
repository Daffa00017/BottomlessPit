//Behavior Tree Task Maintain Distance Header
#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "Enum/AIMovementState.h"
#include "Pawn/Enemy/CPP_EnemyParent.h"
#include "BTT_MaintainDistance2D.generated.h"

/**
 * Maintain a horizontal distance band to Target on XZ (side-scroller).
 * - Outside band: move in/out.
 * - Inside band: optionally pick ONE random strafe spot (after Fire requests it).
 * - Ground-snap via short downward trace.
 */
UCLASS()
class BOTTOMLESSPIT_API UBTT_MaintainDistance2D : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTT_MaintainDistance2D();

	/** Player/target actor (Blackboard Object) */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	struct FBlackboardKeySelector TargetKey;

	/** Optional debug: write chosen strafe ground point here (Vector) */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	struct FBlackboardKeySelector StrafeSpotKey;

	/** Fire task sets this Bool after a shot; we consume it to pick ONE new strafe goal */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	struct FBlackboardKeySelector RequestNewStrafeKey;

	// Distance band
	UPROPERTY(EditAnywhere, Category = "Move") float PreferMin = 450.f;
	UPROPERTY(EditAnywhere, Category = "Move") float PreferMax = 550.f;
	UPROPERTY(EditAnywhere, Category = "Move") float BandEpsilon = 40.f;

	// Grounding
	UPROPERTY(EditAnywhere, Category = "Ground") bool bSnapToGround = true;
	UPROPERTY(EditAnywhere, Category = "Ground", meta = (EditCondition = "bSnapToGround"))
	float GroundTrace = 300.f;
	UPROPERTY(EditAnywhere, Category = "Ground", meta = (EditCondition = "bSnapToGround"))
	TEnumAsByte<ECollisionChannel> GroundChannel = ECC_Visibility;

	// Facing
	UPROPERTY(EditAnywhere, Category = "Facing") bool bFlipSpriteX = true;

	// Strafe (inside band)
	UPROPERTY(EditAnywhere, Category = "Strafe") bool  bRandomizeInBand = true;
	UPROPERTY(EditAnywhere, Category = "Strafe") float MinDeltaXFromMe = 200.f;
	UPROPERTY(EditAnywhere, Category = "Strafe") float StrafeJitter = 80.f;
	UPROPERTY(EditAnywhere, Category = "Strafe") float StrafeEpsilon = 50.f;
	UPROPERTY(EditAnywhere, Category = "Strafe") bool  bRequireLOSForStrafe = true;
	UPROPERTY(EditAnywhere, Category = "Strafe") float EyeHeight = 30.f;

protected:
	virtual EBTNodeResult::Type ExecuteTask(class UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(class UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual EBTNodeResult::Type AbortTask(class UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

private:
	TWeakObjectPtr<class APawn>  Pawn;
	TWeakObjectPtr<class AActor> Target;
	float StartTime = 0.f;

	// replaces TOptional
	bool  bHasGoalX = false;
	float GoalX = 0.f;

	// helpers (must match cpp exactly)
	static float DistXZ(const FVector& A, const FVector& B);
	void Snap(class APawn* P);
	void Flip(class APawn* P, float DX);
	bool ChooseNewStrafeGoal(class UWorld* W, const FVector& Me, const FVector& Player, float& OutGoalX);
	bool ProjectGroundAtX(class UWorld* W, float X, float StartZ, float Y, FVector& OutGround) const;
	bool HasLOSFrom(class UWorld* W, const FVector& From, const AActor* To) const;

	static FORCEINLINE void SetAIMoveStateIfEnemy(APawn* P, EAIMovementState NewState)
	{
		if (ACPP_EnemyParent* E = Cast<ACPP_EnemyParent>(P))
		{
			E->SetAIMoveState(NewState);
		}
	}

};


