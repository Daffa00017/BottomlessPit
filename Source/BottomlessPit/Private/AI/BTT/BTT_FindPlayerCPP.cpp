// Fill out your copyright notice in the Description page of Project Settings.

//BTT_FindPlayer CPP
#include "AI/BTT/BTT_FindPlayerCPP.h"
#include "kismet/GameplayStatics.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTT_FindPlayerCPP::UBTT_FindPlayerCPP()
{
	NodeName = "Find Player";
}
EBTNodeResult::Type UBTT_FindPlayerCPP::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (APawn* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0))
	{
		auto Blackboard = OwnerComp.GetBlackboardComponent();
		Blackboard->SetValueAsObject(PlayerKey.SelectedKeyName, Player);

		return EBTNodeResult::Succeeded;
	}

	return EBTNodeResult::Failed;
}






