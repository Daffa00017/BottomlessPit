// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTT_FindVantage2D.generated.h"

/**
 * 
 */
UCLASS()
class BOTTOMLESSPIT_API UBTT_FindVantage2D : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UBTT_FindVantage2D();

	UPROPERTY(EditAnywhere, Category = "Blackboard") FBlackboardKeySelector TargetKey;     // AttackTarget
	UPROPERTY(EditAnywhere, Category = "Blackboard") FBlackboardKeySelector OutVantageKey;// VantageSpot (Vector)
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector LastVantageKey; // Vector (can be None)

	// search params
	UPROPERTY(EditAnywhere, Category = "Search") float MaxDistance = 1600.f;
	UPROPERTY(EditAnywhere, Category = "Search") float Step = 120.f;
	UPROPERTY(EditAnywhere, Category = "Search") float SameFloorTolerance = 80.f;
	UPROPERTY(EditAnywhere, Category = "Search") float EyeHeight = 30.f;

	// traces
	UPROPERTY(EditAnywhere, Category = "Trace") float GroundTrace = 400.f;
	UPROPERTY(EditAnywhere, Category = "Trace") TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	UPROPERTY(EditAnywhere, Category = "Random")
	bool bRandomizeStartSide = true;                 // randomize whether we search right or left first

	UPROPERTY(EditAnywhere, Category = "Random", meta = (ClampMin = "0"))
	float Jitter = 60.f;                             // random +/- X offset around each probe

	UPROPERTY(EditAnywhere, Category = "Random", meta = (ClampMin = "0"))
	float MinDeltaXFromOrigin = 200.f;               // don't pick a vantage that's almost my current X

	UPROPERTY(EditAnywhere, Category = "Random", meta = (ClampMin = "0"))
	float MinDeltaXFromLast = 150.f;

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

private:
	bool GroundAt(UWorld* W, const FVector& FromXZ, FVector& OutGround) const;
	bool LOSFrom(UWorld* W, const FVector& From, const AActor* Target) const;
};



