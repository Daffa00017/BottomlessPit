//Fire if within Line Of Sight Header

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTT_FireIfLOS2D.generated.h"

/**
 * 
 */
UCLASS()
class BOTTOMLESSPIT_API UBTT_FireIfLOS2D : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UBTT_FireIfLOS2D();

	UPROPERTY(EditAnywhere, Category = "Blackboard") FBlackboardKeySelector TargetKey;     // AttackTarget
	UPROPERTY(EditAnywhere, Category = "Blackboard") FBlackboardKeySelector HasLOSKey;     // HasLOS (Bool)
	UPROPERTY(EditAnywhere, Category = "Blackboard") FBlackboardKeySelector LastShotTimeKey; // LastShotTime (Float)

	UPROPERTY(EditAnywhere, Category = "Combat") float AttackRange = 900.f;
	UPROPERTY(EditAnywhere, Category = "Combat") float Cooldown = 3.0f;
	UPROPERTY(EditAnywhere, Category = "Combat") float EyeHeight = 30.f;
	UPROPERTY(EditAnywhere, Category = "Combat") TEnumAsByte<ECollisionChannel> LOSChannel = ECC_Visibility;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector RequestNewStrafeKey;

	// NEW: face the player on fire (left/right only)
	UPROPERTY(EditAnywhere, Category = "Facing")
	bool bFaceTargetOnFire = true;

	// NEW: True = mirror sprite X-scale. False = rotate yaw 0/180.
	UPROPERTY(EditAnywhere, Category = "Facing")
	bool bFlipSpriteX = true;

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

private:
	static bool LOS2D(const APawn* Me, const AActor* T, float EyeZ, float Range, ECollisionChannel Chan);
	// NEW:
	void FaceToward(APawn* Me, const AActor* T) const;
};



