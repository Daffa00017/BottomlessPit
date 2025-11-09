#pragma once

#include "CoreMinimal.h"
#include "RR_MovementTypes.generated.h"

//  Enum: UHT needs "generated.h"
UENUM(BlueprintType)
enum class EE_PlayerMovementState : uint8
{
    Idle      UMETA(DisplayName = "Idle"),
    Walk      UMETA(DisplayName = "Walk"),
    Aiming    UMETA(DisplayName = "Aiming"),
    Slide     UMETA(DisplayName = "Slide"),
    Roll      UMETA(DisplayName = "Roll"),
    WallSlide UMETA(DisplayName = "WallSlide"),
    Dash      UMETA(DisplayName = "Dash"),
    Jump      UMETA(DisplayName = "Jump")
};

// Delegates: pure C++ macros, no generated.h magic needed
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMoveAxisUpdated, float, Axis, float, SpeedX);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMovementStateChanged, EE_PlayerMovementState, OldState, EE_PlayerMovementState, NewState);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDownSmashStarted, float, Value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDownSmashCompleted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDashStarted, float, Value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDashCompleted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnJumpStarted, float, Value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnJumpTriggered);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnJumpCompleted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShootStarted, float, Value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnShootCompleted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSlideRollStarted, float, Value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSlideRollCompleted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAimStarted, float, Value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAimCompleted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAimUpdatedSignature, float, AimAngleDegrees, FVector, AimDirection);



