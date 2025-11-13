// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DA_ShakePreset.generated.h"

/**
 * 
 */
UCLASS()
class BOTTOMLESSPIT_API UDA_ShakePreset : public UDataAsset
{
	GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Duration = 0.12f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float BlendIn = 0.02f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float BlendOut = 0.08f;

    // Base amplitude/frequency (UU and Hz)
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float AmpX = 3.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float AmpZ = 2.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Freq = 14.f;

    // Extras
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float RollAmp = 0.f;   // keep tiny for pixel art
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Scale = 1.f;
};
