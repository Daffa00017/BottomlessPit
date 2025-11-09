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

private:
    // held state (mirrors your character code, but lives in the controller)
    bool bLeftHeld = false;
    bool bRightHeld = false;

    void OnLeftStarted(const struct FInputActionValue&);    // A down
    void OnLeftTriggered(const struct FInputActionValue&);  // held repeat
    void OnLeftCompleted(const struct FInputActionValue&);  // A up

    void OnRightStarted(const struct FInputActionValue&);
    void OnRightTriggered(const struct FInputActionValue&);
    void OnRightCompleted(const struct FInputActionValue&);

    void OnDownStarted(const struct FInputActionValue&);
    void OnDownTriggered(const struct FInputActionValue&);
    void OnDownCompleted(const struct FInputActionValue&);

    void OnJumpStarted(const struct FInputActionValue&);
    void OnJumpTriggered(const struct FInputActionValue&);
    void OnJumpCompleted(const struct FInputActionValue&);

    void OnMoveX_Triggered(const struct FInputActionValue& Value);
    void OnMoveX_Completed(const struct FInputActionValue& Value);

    void OnSteer_Triggered(const struct FInputActionValue& Value);
    void OnSteer_Completed(const struct FInputActionValue& Value);

    void SendCombinedAxis(); // compute (-1,0,+1) and forward

    ACPP_DownwellLiteCharacter* GetDWChar() const;
};



