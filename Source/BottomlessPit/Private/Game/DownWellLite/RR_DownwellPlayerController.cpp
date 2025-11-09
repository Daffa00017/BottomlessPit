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
    static const FKey WheelAxisKey(TEXT("RawInput.Axis0")); // replace with your key
    const float v = GetInputAnalogKeyState(WheelAxisKey);

    GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Cyan,
        FString::Printf(TEXT("Wheel = %.3f"), v));
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
            EIC->BindAction(IA_MoveX, ETriggerEvent::Triggered, this, &ThisClass::OnMoveX_Triggered);
            EIC->BindAction(IA_MoveX, ETriggerEvent::Completed, this, &ThisClass::OnMoveX_Completed);
        }

        if (IA_Steer)
        {
            EIC->BindAction(IA_Steer, ETriggerEvent::Triggered, this, &ThisClass::OnSteer_Triggered);
            EIC->BindAction(IA_Steer, ETriggerEvent::Completed, this, &ThisClass::OnSteer_Completed);
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
    }
}

ACPP_DownwellLiteCharacter* ARR_DownwellPlayerController::GetDWChar() const
{
    return Cast<ACPP_DownwellLiteCharacter>(GetPawn());
}

void ARR_DownwellPlayerController::OnLeftStarted(const FInputActionValue&)
{
    bLeftHeld = true;  
    SendCombinedAxis();
}

void ARR_DownwellPlayerController::OnLeftTriggered(const FInputActionValue&)
{
    bLeftHeld = true;
    SendCombinedAxis();
}

void ARR_DownwellPlayerController::OnLeftCompleted(const FInputActionValue&)
{
    bLeftHeld = false;
    SendCombinedAxis();
}

void ARR_DownwellPlayerController::OnRightStarted(const FInputActionValue&)
{
    bRightHeld = true;  
    SendCombinedAxis();
}

void ARR_DownwellPlayerController::OnRightTriggered(const FInputActionValue&)
{
    bRightHeld = true;
    SendCombinedAxis();
}

void ARR_DownwellPlayerController::OnRightCompleted(const FInputActionValue&)
{
    bRightHeld = false;  
    SendCombinedAxis();
}

void ARR_DownwellPlayerController::OnDownStarted(const FInputActionValue&)
{
    if (auto* C = GetDWChar())
        C->InputDownPressed();
}

void ARR_DownwellPlayerController::OnDownTriggered(const FInputActionValue&)
{
}

void ARR_DownwellPlayerController::OnDownCompleted(const FInputActionValue&)
{
    if (auto* C = GetDWChar())
        C->InputDownReleased();
}

void ARR_DownwellPlayerController::SendCombinedAxis()
{
    // Right = +1, Left = -1, Both = 0 (your desired behavior)
        const int Axis = (bRightHeld ? 1 : 0) - (bLeftHeld ? 1 : 0);
    if (auto* C = GetDWChar())
    {
        C->SetMoveAxis(static_cast<float>(Axis));
        // If you want your Walk/Idle transitions like before, you can also:
        // if (Axis != 0) C->CommitWalkIfStillDirected(); else C->ScheduleIdleConfirm();
    }
}

void ARR_DownwellPlayerController::OnJumpStarted(const FInputActionValue&) 
{ 
    if (auto* C = GetDWChar()) 
        C->InputJumpPressed(); 
}

void ARR_DownwellPlayerController::OnJumpTriggered(const FInputActionValue&)
{
    if (auto* C = GetDWChar())
        C->InputJumpTriggered();
}

void ARR_DownwellPlayerController::OnJumpCompleted(const FInputActionValue&) 
{ 
    if (auto* C = GetDWChar()) 
        C->InputJumpReleased(); 
}

void ARR_DownwellPlayerController::OnMoveX_Triggered(const FInputActionValue& Value)
{
    float Axis = FMath::Clamp(Value.Get<float>(), -1.f, 1.f);
    // Safety deadzone (optional; IMC deadzone should already handle it)
    if (FMath::Abs(Axis) < 0.2f) Axis = 0.f;

    if (auto* C = GetDWChar())
        C->SetMoveAxis(Axis);
}

void ARR_DownwellPlayerController::OnMoveX_Completed(const FInputActionValue& Value)
{
    if (auto* C = GetDWChar())
        C->SetMoveAxis(0.f);
}

void ARR_DownwellPlayerController::OnSteer_Triggered(const FInputActionValue& Value)
{
    float Axis = FMath::Clamp(Value.Get<float>(), -1.f, 1.f);
    // Optional safety deadzone if your IMC deadzone is low:
    if (FMath::Abs(Axis) < 0.06f) Axis = 0.f;
    const float x = Value.Get<float>();
    UE_LOG(LogTemp, Log, TEXT("Steering = %.3f"), x);


    if (auto* C = GetDWChar())
        C->SetMoveAxis(Axis);

}

void ARR_DownwellPlayerController::OnSteer_Completed(const FInputActionValue& Value)
{
    if (auto* C = GetDWChar())
        C->SetMoveAxis(0.f);
}



