// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Utility/DA_ShakePreset.h"
#include "BFL_ShakeLibrary.generated.h"

class UCameraShakeBase;
class APlayerController;

UCLASS()
class BOTTOMLESSPIT_API UBFL_ShakeLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category = "Camera|Shake")
    static UCameraShakeBase* PlayPerlinShake(APlayerController* PC,
        TSubclassOf<UCameraShakeBase> BasePerlinShakeClass,
        UDA_ShakePreset* Preset, float ExtraScale = 1.f, bool bStopSameClass = false);
};
