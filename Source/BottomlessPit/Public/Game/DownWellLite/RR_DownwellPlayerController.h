// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Game/CPP_PC_BottomlessPit.h"
#include "RR_DownwellPlayerController.generated.h"


class UInputMappingContext;
class UInputAction;
class ACPP_DownwellLiteCharacter;


UCLASS()
class BOTTOMLESSPIT_API ARR_DownwellPlayerController : public ACPP_PC_BottomlessPit
{
	GENERATED_BODY()
	
public:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void SetupInputComponent() override;

    UPROPERTY(EditAnywhere, Category = "Input|IMC") UInputMappingContext* IMC_DownwellLite = nullptr;
    UPROPERTY(EditAnywhere, Category = "Input|IA")  UInputAction* IA_MoveLeft = nullptr;
    UPROPERTY(EditAnywhere, Category = "Input|IA")  UInputAction* IA_MoveRight = nullptr;
    UPROPERTY(EditAnywhere, Category = "Input|IA")  UInputAction* IA_MoveDown = nullptr;
    UPROPERTY(EditAnywhere, Category = "Input|IA")  UInputAction* IA_Jump = nullptr;
    UPROPERTY(EditDefaultsOnly, Category = "Input|IA") UInputAction* IA_MoveX = nullptr;
    UPROPERTY(EditDefaultsOnly, Category = "Input|IA") UInputAction* IA_Steer = nullptr;
    UPROPERTY(EditDefaultsOnly, Category = "Input|IA") UInputAction* IA_Escape = nullptr;
    UPROPERTY(EditDefaultsOnly, Category = "Input|IA") UInputAction* IA_Enter = nullptr;

    UPROPERTY(BlueprintReadWrite,EditAnywhere, category = "Input|Variable")
    bool IsInMainMenu= false;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, category = "Input|Variable")
    bool IsInQuitGame = false;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, category = "Input|Variable")
    bool IsPlayerDead = false;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, category = "Input|Variable")
    bool ConfirmToExit = false;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, category = "Input|Variable")
    float AcceptNextFirstInputDur = 0.1f;

    UFUNCTION(BlueprintImplementableEvent)
    void OnFirstInput();

    UFUNCTION(BlueprintImplementableEvent)
    void OnEscapeInput();

    UFUNCTION(BlueprintImplementableEvent)
    void OnQuitGame();

    UFUNCTION(BlueprintCallable, Category = "Input|Arduino")
    void ApplyArduinoAccelVector(FVector2D RawXY);

    UFUNCTION(BlueprintCallable, Category = "Input|Arduino")
    void ApplyArduinoAccelInput(float RawX, float RawY);

    // Tuning params
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Arduino")
    float ArduinoMaxAbsInput = 1.0f;
    // If your Arduino sends -500..500 or -1023..1023, set this to 500 or 1023.

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Arduino")
    float ArduinoDeadzone = 0.15f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Arduino")
    float ArduinoSensitivity = 1.0f;
    // Multiplier after normalization.

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Arduino")
    float ArduinoSmoothing = 0.15f;
    // 0 = no smoothing, 0.1~0.3 feels nice.

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Arduino")
    bool bArduinoUseYForJumpDown = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Arduino", meta = (EditCondition = "bArduinoUseYForJumpDown"))
    float ArduinoJumpThreshold = 0.55f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Arduino", meta = (EditCondition = "bArduinoUseYForJumpDown"))
    float ArduinoDownThreshold = -0.55f;


private:
    // held state (mirrors your character code, but lives in the controller)
    bool bLeftHeld = false;
    bool bRightHeld = false;
    bool bIsFirstInputCooldownActive = false;
    FTimerHandle TimerHandle_FirstInputCooldown;

    void OnLeftStarted(const struct FInputActionValue&);    
    void OnLeftTriggered(const struct FInputActionValue&);  
    void OnLeftCompleted(const struct FInputActionValue&);  

    void OnRightStarted(const struct FInputActionValue&);
    void OnRightTriggered(const struct FInputActionValue&);
    void OnRightCompleted(const struct FInputActionValue&);

    void OnDownStarted(const struct FInputActionValue&);
    void OnDownTriggered(const struct FInputActionValue&);
    void OnDownCompleted(const struct FInputActionValue&);

    void OnJumpStarted(const struct FInputActionValue&);
    void OnJumpTriggered(const struct FInputActionValue&);
    void OnJumpCompleted(const struct FInputActionValue&);
    
    void OnMoveX_Started(const struct FInputActionValue& Value);
    void OnMoveX_Triggered(const struct FInputActionValue& Value);
    void OnMoveX_Completed(const struct FInputActionValue& Value);

    void OnEscapeStarted(const struct FInputActionValue&);
    void OnEscapeCompleted(const struct FInputActionValue&);

    void OnEnterStarted(const struct FInputActionValue&);
    void OnEnterCompleted(const struct FInputActionValue&);

    UFUNCTION(BlueprintCallable)
    void SendCombinedAxis(); // compute (-1,0,+1) and forward

    ACPP_DownwellLiteCharacter* GetDWChar() const;

    void TryTriggerFirstInput();
    void ResetFirstInputCooldown();

    float ArduinoSmoothedX = 0.f;
    bool bArduinoJumpHeld = false;
    bool bArduinoDownHeld = false;

    void HandleArduinoYActions(float NormY);
};



