
//BTService SetPlayerTarget Header
#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTS_SetPlayerTarget.generated.h"

/**
 * 
 */
UCLASS()
class BOTTOMLESSPIT_API UBTS_SetPlayerTarget : public UBTService
{
	GENERATED_BODY()
public:
    UBTS_SetPlayerTarget() { NodeName = TEXT("Set Player -> AttackTarget"); Interval = 0.15f; RandomDeviation = 0.05f; }

    UPROPERTY(EditAnywhere, Category = "Blackboard")
    struct FBlackboardKeySelector TargetKey; // AttackTarget

protected:
    virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float) override;
};



