// Fill out your copyright notice in the Description page of Project Settings.
//BTService SetPlayerTarget CPP

#include "AI/BTService/BTS_SetPlayerTarget.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Kismet/GameplayStatics.h"

void UBTS_SetPlayerTarget::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float)
{
    if (auto* BB = OwnerComp.GetBlackboardComponent())
        if (APawn* P = UGameplayStatics::GetPlayerPawn(OwnerComp.GetWorld(), 0))
            BB->SetValueAsObject(TargetKey.SelectedKeyName, P);
}



