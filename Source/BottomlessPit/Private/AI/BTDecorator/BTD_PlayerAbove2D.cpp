// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BTDecorator/BTD_PlayerAbove2D.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"

UBTD_PlayerAbove2D::UBTD_PlayerAbove2D()
{
	NodeName = TEXT("PlayerIsAbove");
}

bool UBTD_PlayerAbove2D::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	const AAIController* AIC = OwnerComp.GetAIOwner();
	if (!AIC) return false;

	const APawn* SelfPawn = AIC->GetPawn();
	const UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	const AActor* Target = BB ? Cast<AActor>(BB->GetValueAsObject(TargetKey.SelectedKeyName)) : nullptr;
	if (!SelfPawn || !Target) return false;

	const FVector S = SelfPawn->GetActorLocation();
	const FVector T = Target->GetActorLocation();

	const float dz = T.Z - S.Z;
	const float dx = FMath::Abs(T.X - S.X);

	if (dz <= ThresholdUp) return false;
	if (MaxHorizontal > 0.f && dx > MaxHorizontal) return false;
	return true;
}



