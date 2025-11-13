// Fill out your copyright notice in the Description page of Project Settings.


#include "Utility/FunctionLibrary/BFL_ShakeLibrary.h"
#include "Camera/PlayerCameraManager.h"
#include "Camera/CameraShakeBase.h"
//#include "GameplayCameras/PerlinNoiseCameraShakePattern.h"

UCameraShakeBase* UBFL_ShakeLibrary::PlayPerlinShake(APlayerController* PC,
    TSubclassOf<UCameraShakeBase> BaseClass, UDA_ShakePreset* Preset, float ExtraScale, bool bStopSameClass)
{
    if (APlayerCameraManager* PCM = PC->PlayerCameraManager)
        {
        if (bStopSameClass) { PCM->StopAllInstancesOfCameraShake(BaseClass, /*bImmediate=*/false); }
        const float Scale = (Preset ? Preset->Scale : 1.f) * ExtraScale;
        UCameraShakeBase * Instance = PCM->StartCameraShake(BaseClass, Scale); // simple overload
        return Instance;
        }
    return nullptr;
}
