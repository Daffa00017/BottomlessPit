// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTS_SenseVertical2D.generated.h"

/**
 * 
 */
UCLASS()
class BOTTOMLESSPIT_API UBTS_SenseVertical2D : public UBTService
{
	GENERATED_BODY()
public:
	UBTS_SenseVertical2D();

	/** BB key with the player/target actor (e.g., AttackTarget) */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	struct FBlackboardKeySelector TargetKey;

	/** BB bool to write: true if target Z is sufficiently above me */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	struct FBlackboardKeySelector PlayerAboveKey;

	/** Consider “above” when Target.Z - Me.Z > ThresholdUp */
	UPROPERTY(EditAnywhere, Category = "Sense")
	float ThresholdUp = 80.f;

	/** Also consider horizontal window; if <=0, ignore */
	UPROPERTY(EditAnywhere, Category = "Sense")
	float MaxHorizontal = 1200.f;

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	
};



