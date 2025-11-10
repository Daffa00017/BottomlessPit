// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/CPP_HUD.h"
#include "Blueprint/UserWidget.h"
#include "UI/Widget/CPP_GameplayUI.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

ACPP_HUD::ACPP_HUD()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bTickEvenWhenPaused = true;
}

void ACPP_HUD::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void ACPP_HUD::BeginPlay()
{
	Super::BeginPlay();
	CreateGameplayWidget();
}

void ACPP_HUD::CreateGameplayWidget()
{
    if (GameplayWidgetClass && GetOwningPlayerController())
    {
        WB_GameplayUIReference = CreateWidget<UCPP_GameplayUI>(
            GetOwningPlayerController(),
            GameplayWidgetClass
        );

        if (WB_GameplayUIReference)
        {
            WB_GameplayUIReference->AddToViewport();
        }
    }
}



