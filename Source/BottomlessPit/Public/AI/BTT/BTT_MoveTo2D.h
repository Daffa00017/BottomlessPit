// Fill out your copyright notice in the Description page of Project Settings.
//BTT_MoveTo2D Header
#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTT_MoveTo2D.generated.h"

/**
 * 
 */
UCLASS()
class BOTTOMLESSPIT_API UBTT_MoveTo2D : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UBTT_MoveTo2D();

	/** Blackboard key holding the target actor (e.g., "AttackTarget") */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	struct FBlackboardKeySelector TargetKey;

	/** Succeed when within this 2D radius (XZ) */
	UPROPERTY(EditAnywhere, Category = "Move")
	float AcceptRadius = 120.f;

	/** Safety timeout (seconds) */
	UPROPERTY(EditAnywhere, Category = "Move")
	float MaxTime = 10.f;

	/** Keep pawn glued to ground with a short downward trace */
	UPROPERTY(EditAnywhere, Category = "Ground")
	bool bSnapToGround = true;

	/** How far down to search for ground each tick */
	UPROPERTY(EditAnywhere, Category = "Ground", meta = (EditCondition = "bSnapToGround"))
	float GroundTraceDistance = 200.f;

	/** Height above capsule bottom to place pawn when we hit ground (small offset) */
	UPROPERTY(EditAnywhere, Category = "Ground", meta = (EditCondition = "bSnapToGround"))
	float GroundOffset = 0.f;

	/** Channel for ground check (Visibility or a custom one) */
	UPROPERTY(EditAnywhere, Category = "Ground", meta = (EditCondition = "bSnapToGround"))
	TEnumAsByte<ECollisionChannel> GroundTraceChannel = ECC_Visibility;

	/** Flip sprite by X scale when changing horizontal direction */
	UPROPERTY(EditAnywhere, Category = "Facing")
	bool bFlipSpriteX = true;

protected:
	virtual EBTNodeResult::Type ExecuteTask(class UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(class UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual EBTNodeResult::Type AbortTask(class UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

private:
	TWeakObjectPtr<class APawn> CachedPawn;
	TWeakObjectPtr<class AActor> CachedTarget;
	float StartTime = 0.f;

	// Helpers
	static float Dist2D_XZ(const FVector& A, const FVector& B);
	void MoveHorizontalToward(APawn* Pawn, const FVector& TargetLoc);
	void SnapDownToGround(APawn* Pawn);
	void UpdateFlip(APawn* Pawn, float DeltaX);
};



