// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "CPP_HUD.generated.h"

class UCPP_GameplayUI;

UCLASS()
class BOTTOMLESSPIT_API ACPP_HUD : public AHUD
{
	GENERATED_BODY()
	
public:
	ACPP_HUD();

    virtual void Tick(float DeltaTime) override;

protected:
    virtual void BeginPlay() override;

    // Reference to your gameplay UI widget
    UPROPERTY(BlueprintReadOnly, Category = "UI")
    UCPP_GameplayUI* WB_GameplayUIReference;

    // Widget class to create (set this in Blueprint)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
    TSubclassOf<UUserWidget> GameplayWidgetClass;

    // Function to create and show the widget
    UFUNCTION(BlueprintCallable, Category = "UI")
    void CreateGameplayWidget();
private:
};



