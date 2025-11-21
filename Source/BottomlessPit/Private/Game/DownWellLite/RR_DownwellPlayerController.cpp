// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/DownWellLite/RR_DownwellPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Character/CPP_DownwellLiteCharacter.h"

void ARR_DownwellPlayerController::BeginPlay()
{
    Super::BeginPlay();
    if (ULocalPlayer* LP = GetLocalPlayer())
        if (auto* Sub = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LP))
            if (IMC_DownwellLite) Sub->AddMappingContext(IMC_DownwellLite, 1);
}

void ARR_DownwellPlayerController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}



void ARR_DownwellPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    if (auto* EIC = Cast<UEnhancedInputComponent>(InputComponent))
    {
        if (IA_MoveLeft)
        {
            EIC->BindAction(IA_MoveLeft, ETriggerEvent::Started, this, &ARR_DownwellPlayerController::OnLeftStarted);
            EIC->BindAction(IA_MoveLeft, ETriggerEvent::Triggered, this, &ARR_DownwellPlayerController::OnLeftTriggered);
            EIC->BindAction(IA_MoveLeft, ETriggerEvent::Completed, this, &ARR_DownwellPlayerController::OnLeftCompleted);
        }
        if (IA_MoveRight)
        {
            EIC->BindAction(IA_MoveRight, ETriggerEvent::Started, this, &ARR_DownwellPlayerController::OnRightStarted);
            EIC->BindAction(IA_MoveRight, ETriggerEvent::Triggered, this, &ARR_DownwellPlayerController::OnRightTriggered);
            EIC->BindAction(IA_MoveRight, ETriggerEvent::Completed, this, &ARR_DownwellPlayerController::OnRightCompleted);
        }

        if (IA_MoveX)
        {
            EIC->BindAction(IA_MoveX, ETriggerEvent::Started, this, &ThisClass::OnMoveX_Started);
            EIC->BindAction(IA_MoveX, ETriggerEvent::Triggered, this, &ThisClass::OnMoveX_Triggered);
            EIC->BindAction(IA_MoveX, ETriggerEvent::Completed, this, &ThisClass::OnMoveX_Completed);
        }

        if (IA_Jump)
        {
            EIC->BindAction(IA_Jump, ETriggerEvent::Started, this, &ARR_DownwellPlayerController::OnJumpStarted);
            EIC->BindAction(IA_Jump, ETriggerEvent::Triggered, this, &ARR_DownwellPlayerController::OnJumpTriggered);
            EIC->BindAction(IA_Jump, ETriggerEvent::Completed, this, &ARR_DownwellPlayerController::OnJumpCompleted);
        }

        if (IA_MoveDown)
        {
            EIC->BindAction(IA_MoveDown, ETriggerEvent::Started, this, &ARR_DownwellPlayerController::OnDownStarted);
            EIC->BindAction(IA_MoveDown, ETriggerEvent::Triggered, this, &ARR_DownwellPlayerController::OnDownTriggered);
            EIC->BindAction(IA_MoveDown, ETriggerEvent::Completed, this, &ARR_DownwellPlayerController::OnDownCompleted);
        }

        if (IA_Escape)
        {
            EIC->BindAction(IA_Escape, ETriggerEvent::Started, this, &ARR_DownwellPlayerController::OnEscapeStarted);
            EIC->BindAction(IA_Escape, ETriggerEvent::Completed, this, &ARR_DownwellPlayerController::OnEscapeCompleted);
        }

        if (IA_Enter)
        {
            EIC->BindAction(IA_Enter, ETriggerEvent::Started, this, &ARR_DownwellPlayerController::OnEnterStarted);
            EIC->BindAction(IA_Enter, ETriggerEvent::Completed, this, &ARR_DownwellPlayerController::OnEnterCompleted);
        }
    }
}

void ARR_DownwellPlayerController::ApplyArduinoAccelVector(FVector2D RawXY)
{
    ApplyArduinoAccelInput(RawXY.X, RawXY.Y);
}

void ARR_DownwellPlayerController::ApplyArduinoAccelInput(float RawX, float RawY)
{
    if (IsPlayerDead || ConfirmToExit)
    {
        return;
    }

    // Normalize to [-1, 1] using ArduinoMaxAbsInput
    const float SafeMax = FMath::Max(ArduinoMaxAbsInput, 0.001f);

    float NormX = FMath::Clamp(RawX / SafeMax, -1.f, 1.f);
    float NormY = FMath::Clamp(RawY / SafeMax, -1.f, 1.f);

    // Deadzone
    if (FMath::Abs(NormX) < ArduinoDeadzone) NormX = 0.f;
    if (FMath::Abs(NormY) < ArduinoDeadzone) NormY = 0.f;

    // Sensitivity
    NormX = FMath::Clamp(NormX * ArduinoSensitivity, -1.f, 1.f);
    NormY = FMath::Clamp(NormY * ArduinoSensitivity, -1.f, 1.f);

    // Optional smoothing on X (simple exponential)
    if (ArduinoSmoothing > 0.f)
    {
        ArduinoSmoothedX = FMath::Lerp(ArduinoSmoothedX, NormX, FMath::Clamp(ArduinoSmoothing, 0.f, 1.f));
        NormX = ArduinoSmoothedX;
    }

    // Forward horizontal axis to character
    if (ACPP_DownwellLiteCharacter* C = GetDWChar())
    {
        C->SetMoveAxis(NormX);
    }

    // Optionally map Y to jump/down
    if (bArduinoUseYForJumpDown)
    {
        HandleArduinoYActions(NormY);
    }
}

ACPP_DownwellLiteCharacter* ARR_DownwellPlayerController::GetDWChar() const
{
    return Cast<ACPP_DownwellLiteCharacter>(GetPawn());
}

void ARR_DownwellPlayerController::TryTriggerFirstInput()
{
    if (bIsFirstInputCooldownActive)
    {
        return; 
    }
    bIsFirstInputCooldownActive = true;
    OnFirstInput();
    GetWorld()->GetTimerManager().SetTimer(
        TimerHandle_FirstInputCooldown,
        this,
        &ARR_DownwellPlayerController::ResetFirstInputCooldown,
        AcceptNextFirstInputDur,
        false);
}

void ARR_DownwellPlayerController::ResetFirstInputCooldown()
{
    bIsFirstInputCooldownActive = false;
}

void ARR_DownwellPlayerController::HandleArduinoYActions(float NormY)
{
    ACPP_DownwellLiteCharacter* C = GetDWChar();
    if (!C) return;

    // Jump when tilted up past threshold
    const bool bShouldJump = (NormY >= ArduinoJumpThreshold);
    if (bShouldJump && !bArduinoJumpHeld)
    {
        bArduinoJumpHeld = true;
        OnFirstInput();
        C->InputJumpPressed();
    }
    else if (!bShouldJump && bArduinoJumpHeld)
    {
        bArduinoJumpHeld = false;
        C->InputJumpReleased();
    }

    // Down when tilted down past threshold
    const bool bShouldDown = (NormY <= ArduinoDownThreshold);
    if (bShouldDown && !bArduinoDownHeld)
    {
        bArduinoDownHeld = true;
        OnFirstInput();
        C->InputDownPressed();
    }
    else if (!bShouldDown && bArduinoDownHeld)
    {
        bArduinoDownHeld = false;
        C->InputDownReleased();
    }
}

void ARR_DownwellPlayerController::OnLeftStarted(const FInputActionValue&)
{
    bLeftHeld = true;
    OnFirstInput();
    if (!IsPlayerDead && !ConfirmToExit)
    {
        SendCombinedAxis();
    }
}

void ARR_DownwellPlayerController::OnLeftTriggered(const FInputActionValue&)
{
    bLeftHeld = true;
    if (!IsPlayerDead && !ConfirmToExit)
    {
        SendCombinedAxis();
    }
}

void ARR_DownwellPlayerController::OnLeftCompleted(const FInputActionValue&)
{
    bLeftHeld = false;
    OnFirstInput();
    if (!IsPlayerDead && !ConfirmToExit)
    {
        SendCombinedAxis();
    }
}

void ARR_DownwellPlayerController::OnRightStarted(const FInputActionValue&)
{
    bRightHeld = true;  
    OnFirstInput();
    if (!IsPlayerDead && !ConfirmToExit)
    {
        SendCombinedAxis();
    }
}

void ARR_DownwellPlayerController::OnRightTriggered(const FInputActionValue&)
{
    bRightHeld = true;
    if (!IsPlayerDead && !ConfirmToExit)
    {
        SendCombinedAxis();
    }
}

void ARR_DownwellPlayerController::OnRightCompleted(const FInputActionValue&)
{
    bRightHeld = false; 
    if (!IsPlayerDead && !ConfirmToExit)
    {
        SendCombinedAxis();
    }
}

void ARR_DownwellPlayerController::OnDownStarted(const FInputActionValue&)
{
    OnFirstInput();
    if (!IsPlayerDead && !ConfirmToExit)
    {
        if (auto* C = GetDWChar())
            C->InputDownPressed();
    }
}

void ARR_DownwellPlayerController::OnDownTriggered(const FInputActionValue&)
{
}

void ARR_DownwellPlayerController::OnDownCompleted(const FInputActionValue&)
{
    if (!IsPlayerDead && !ConfirmToExit)
    {
        if (auto* C = GetDWChar())
            C->InputDownReleased();
    }
}

void ARR_DownwellPlayerController::SendCombinedAxis()
{
    if (!IsPlayerDead && !ConfirmToExit)
    {
            const int Axis = (bRightHeld ? 1 : 0) - (bLeftHeld ? 1 : 0);
        if (auto* C = GetDWChar())
        {
            C->SetMoveAxis(static_cast<float>(Axis));
        }
    }
}

void ARR_DownwellPlayerController::OnJumpStarted(const FInputActionValue&) 
{ 
        OnFirstInput();
        if (!IsPlayerDead && !ConfirmToExit)
        {
            if (auto* C = GetDWChar())
                C->InputJumpPressed();
        }
}

void ARR_DownwellPlayerController::OnJumpTriggered(const FInputActionValue&)
{
    if (!IsPlayerDead&&!ConfirmToExit)
    {
        if (auto* C = GetDWChar())
            C->InputJumpTriggered();
    }
}

void ARR_DownwellPlayerController::OnJumpCompleted(const FInputActionValue&) 
{ 
    if (!IsPlayerDead && !ConfirmToExit)
    {
        if (auto* C = GetDWChar())
            C->InputJumpReleased();
    }
}

void ARR_DownwellPlayerController::OnMoveX_Started(const FInputActionValue& Value)
{
    OnFirstInput();
    if (!IsPlayerDead && !ConfirmToExit)
    {
        float Axis = FMath::Clamp(Value.Get<float>(), -1.f, 1.f);
        // Safety deadzone (optional; IMC deadzone should already handle it)
        if (FMath::Abs(Axis) < 0.2f) Axis = 0.f;

        if (auto* C = GetDWChar())
            C->SetMoveAxis(Axis);
    }
}

void ARR_DownwellPlayerController::OnMoveX_Triggered(const FInputActionValue& Value)
{
    if (!IsPlayerDead && !ConfirmToExit)
    {
        float Axis = FMath::Clamp(Value.Get<float>(), -1.f, 1.f);
        // Safety deadzone (optional; IMC deadzone should already handle it)
        if (FMath::Abs(Axis) < 0.2f) Axis = 0.f;

        if (auto* C = GetDWChar())
            C->SetMoveAxis(Axis);
    }
}

void ARR_DownwellPlayerController::OnMoveX_Completed(const FInputActionValue& Value)
{
    if (!IsPlayerDead && !ConfirmToExit)
    {
        if (auto* C = GetDWChar())
            C->SetMoveAxis(0.f);
    }
}

void ARR_DownwellPlayerController::OnEscapeStarted(const FInputActionValue&)
{
    if (!IsPlayerDead)
    {
        OnEscapeInput();
    }
}

void ARR_DownwellPlayerController::OnEscapeCompleted(const FInputActionValue&)
{
    if (!IsPlayerDead)
    {

    }
}

void ARR_DownwellPlayerController::OnEnterStarted(const FInputActionValue&)
{
    if (ConfirmToExit)
    {
        OnQuitGame();
    } else {
    
    }
}

void ARR_DownwellPlayerController::OnEnterCompleted(const FInputActionValue&)
{

}





