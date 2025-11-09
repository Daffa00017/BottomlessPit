#pragma once
#include "CoreMinimal.h"
#include "AIMovementState.generated.h"

UENUM(BlueprintType)
enum class EAIMovementState : uint8
{
    Idle      UMETA(DisplayName = "Idle"),
    Walk      UMETA(DisplayName = "Walk"),
    Run       UMETA(DisplayName = "Run"),
    Aiming    UMETA(DisplayName = "Aiming"),
    Slide     UMETA(DisplayName = "Slide"),
    Roll      UMETA(DisplayName = "Roll"),
    WallSlide UMETA(DisplayName = "WallSlide"),
    Dash      UMETA(DisplayName = "Dash"),
    Jump      UMETA(DisplayName = "Jump")   // <-- no trailing comma
};



