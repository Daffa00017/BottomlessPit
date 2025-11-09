// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTT_JumpUp2D.generated.h"

/**
 * 
 */
UCLASS()
class BOTTOMLESSPIT_API UBTT_JumpUp2D : public UBTTaskNode
{
	GENERATED_BODY()
public:
	UBTT_JumpUp2D();

	/** Read target (player) for desired height estimate; optional */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	struct FBlackboardKeySelector TargetKey;

	/** If >0, override jump height (world units). If <=0, compute from player Z delta */
	UPROPERTY(EditAnywhere, Category = "Jump")
	float DesiredHeight = 0.f;

	/** Gravity (units/s^2). UE default ~980. Go higher for snappier 2D jumps. */
	UPROPERTY(EditAnywhere, Category = "Jump")
	float Gravity = 1200.f;

	/** Max initial jump speed clamp */
	UPROPERTY(EditAnywhere, Category = "Jump")
	float MaxJumpSpeed = 1800.f;

	/** Safety timeout */
	UPROPERTY(EditAnywhere, Category = "Jump")
	float MaxTime = 2.0f;

	/** Ground check channel */
	UPROPERTY(EditAnywhere, Category = "Jump")
	TEnumAsByte<ECollisionChannel> GroundTraceChannel = ECC_Visibility;

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

private:
	TWeakObjectPtr<APawn> Pawn;
	float Vz = 0.f;
	float StartTime = 0.f;
	float StartZ = 0.f;

	bool IsGrounded(APawn* P) const;
};



