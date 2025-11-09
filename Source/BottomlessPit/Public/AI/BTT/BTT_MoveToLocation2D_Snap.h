// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pawn/Enemy/CPP_EnemyParent.h"      
#include "Enum/AIMovementState.h"   
#include "BehaviorTree/BTTaskNode.h"
#include "BTT_MoveToLocation2D_Snap.generated.h"

/**
 * 
 */
UCLASS()
class BOTTOMLESSPIT_API UBTT_MoveToLocation2D_Snap : public UBTTaskNode
{
	GENERATED_BODY()
public:
	UBTT_MoveToLocation2D_Snap();

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	struct FBlackboardKeySelector LocationKey; // <- JumpSpot (Vector)

	UPROPERTY(EditAnywhere, Category = "Move") float AcceptRadius = 100.f;
	UPROPERTY(EditAnywhere, Category = "Move") float MaxTime = 8.f;

	UPROPERTY(EditAnywhere, Category = "Ground") bool bSnapToGround = true;
	UPROPERTY(EditAnywhere, Category = "Ground", meta = (EditCondition = "bSnapToGround")) float GroundTraceDistance = 200.f;
	UPROPERTY(EditAnywhere, Category = "Ground", meta = (EditCondition = "bSnapToGround")) TEnumAsByte<ECollisionChannel> GroundTraceChannel = ECC_Visibility;

	UPROPERTY(EditAnywhere, Category = "Facing") bool bFlipSpriteX = true;

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

private:
	TWeakObjectPtr<APawn> Pawn;
	FVector TargetLoc;
	float StartTime = 0.f;

	static float Dist2D_XZ(const FVector& A, const FVector& B);
	void MoveHorizontalToward(APawn* P, const FVector& To);
	void SnapDown(APawn* P);
	void Flip(APawn* P, float DX);
	
	static FORCEINLINE void SetStateIfEnemy(APawn* P, EAIMovementState S)
	{
		if (ACPP_EnemyParent* E = Cast<ACPP_EnemyParent>(P))
		{
			E->SetAIMoveState(S);
		}
	}

};



