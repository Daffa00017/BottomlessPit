// Fill out your copyright notice in the Description page of Project Settings.
//BTT_FindPlayer Header
#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTT_FindPlayerCPP.generated.h"

/**
 * 
 */
UCLASS()
class BOTTOMLESSPIT_API UBTT_FindPlayerCPP : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTT_FindPlayerCPP();

	UPROPERTY(EditAnywhere)
	FBlackboardKeySelector PlayerKey;

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

};



