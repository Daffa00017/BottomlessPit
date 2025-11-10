// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widget/CPP_GameplayUI.h"
#include "Kismet/GameplayStatics.h"

void UCPP_GameplayUI::TickWhilePaused(float DeltaTime)
{
	// PUT ALL YOUR WIDGET TICK LOGIC (animations, updates, etc.) IN HERE
}

void UCPP_GameplayUI::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// When game is NOT paused, we call our main logic function
	if (!UGameplayStatics::IsGamePaused(GetWorld()))
	{
		TickWhilePaused(InDeltaTime);
	}
}
