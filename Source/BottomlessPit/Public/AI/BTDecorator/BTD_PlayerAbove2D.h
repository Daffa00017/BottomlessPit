// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTD_PlayerAbove2D.generated.h"

/**
 * 
 */
UCLASS()
class BOTTOMLESSPIT_API UBTD_PlayerAbove2D : public UBTDecorator
{
	GENERATED_BODY()

public:
	UBTD_PlayerAbove2D();

	/** Blackboard actor key for the player/target (e.g., AttackTarget) */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetKey;

	/** Consider “above” when Target.Z - Self.Z > this */
	UPROPERTY(EditAnywhere, Category = "Sense")
	float ThresholdUp = 80.f;

	/** If >0, require |Target.X - Self.X| <= MaxHorizontal */
	UPROPERTY(EditAnywhere, Category = "Sense")
	float MaxHorizontal = 1200.f;

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	
};



