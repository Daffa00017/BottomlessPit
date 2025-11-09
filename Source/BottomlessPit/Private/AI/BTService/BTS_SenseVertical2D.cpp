// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BTService/BTS_SenseVertical2D.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"

UBTS_SenseVertical2D::UBTS_SenseVertical2D()
{
	NodeName = TEXT("Sense Vertical (2D)");
	Interval = 0.2f; // 5 Hz
	RandomDeviation = 0.05f;
}

void UBTS_SenseVertical2D::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float)
{
	auto* BB = OwnerComp.GetBlackboardComponent();
	auto* AIC = OwnerComp.GetAIOwner();
	if (!BB || !AIC) return;

	APawn* Me = AIC->GetPawn();
	AActor* Target = Cast<AActor>(BB->GetValueAsObject(TargetKey.SelectedKeyName));
	if (!Me || !Target) return;

	const FVector M = Me->GetActorLocation();
	const FVector T = Target->GetActorLocation();

	const float dz = T.Z - M.Z;
	const float dx = FMath::Abs(T.X - M.X);

	const bool bAbove = (dz > ThresholdUp) && (MaxHorizontal <= 0.f || dx <= MaxHorizontal);
	BB->SetValueAsBool(PlayerAboveKey.SelectedKeyName, bAbove);
}




