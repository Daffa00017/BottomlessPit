// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTT_FindJumpSpot2D.generated.h"

/**
 * 
 */
UCLASS()
class BOTTOMLESSPIT_API UBTT_FindJumpSpot2D : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTT_FindJumpSpot2D();

	/** Reads player from this key (AttackTarget) to estimate needed jump height */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	struct FBlackboardKeySelector TargetKey;

	/** Writes found location here */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	struct FBlackboardKeySelector JumpSpotKey;

	/** Writes success bool here */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	struct FBlackboardKeySelector HasJumpSpotKey;

	/** How far to search left/right from current X */
	UPROPERTY(EditAnywhere, Category = "Search")
	float MaxSearchDistance = 1200.f;

	/** Step size when scanning */
	UPROPERTY(EditAnywhere, Category = "Search")
	float Step = 60.f;

	/** Downward trace length to find ground */
	UPROPERTY(EditAnywhere, Category = "Ground")
	float GroundTrace = 400.f;

	/** Clearance height (how much vertical room we need to jump) */
	UPROPERTY(EditAnywhere, Category = "Ceiling")
	float RequiredHeadroom = 240.f;

	/** Trace channel for ground and ceiling checks */
	UPROPERTY(EditAnywhere, Category = "Trace")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	/** Extra margin for capsule top clearance */
	UPROPERTY(EditAnywhere, Category = "Ceiling")
	float TopMargin = 10.f;

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

private:
	bool FindCandidate(UWorld* W, const FVector& Origin, const float DirSign, FVector& OutSpot, float NeedHeadroomZ) const;
	bool GroundAt(UWorld* W, const FVector& From, FVector& OutGround) const;
	bool HeadroomClear(UWorld* W, const FVector& GroundLoc, float Height) const;
	
};



