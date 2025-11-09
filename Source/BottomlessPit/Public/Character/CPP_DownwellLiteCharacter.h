// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Delegates/DelegateCombinations.h"
#include "CPP_PaperZDParentCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "CPP_DownwellLiteCharacter.generated.h"


UCLASS()
class BOTTOMLESSPIT_API ACPP_DownwellLiteCharacter : public ACPP_PaperZDParentCharacter
{
	GENERATED_BODY()
	
public:

	ACPP_DownwellLiteCharacter();
	virtual void Landed(const FHitResult& Hit) override;

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintNativeEvent, Category = "Movement")
	void CustomEventOnLanded(FHitResult HitResult);

	// Called by the Downwell controller
	void SetMoveAxis(float InAxis);
	void InputJumpPressed();
	void InputJumpTriggered();
	void InputJumpReleased();
	void InputDownPressed();
	void InputDownReleased();

	UPROPERTY(BlueprintAssignable, Category = "Input|Delegates") FOnDownSmashStarted      OnDownSmashStartedEvent;
	UPROPERTY(BlueprintAssignable, Category = "Input|Delegates") FOnDownSmashCompleted    OnDownSmashCompletedEvent;

	UPROPERTY(BlueprintReadWrite, category = "Variable | Movement | Jump")
	int CurrentAmmoCount = MaxAmmoCount;

	UPROPERTY(BlueprintReadWrite, category = "Variable | Movement | Jump")
	int MaxAmmoCount = 12;

	UPROPERTY(BlueprintReadWrite, category = "Variable | Movement | Jump")
	int JumpHeight = 650;

	UPROPERTY(BlueprintReadWrite, category = "Variable | Movement | Jump")
	int JumpHeightMidAir = 250;

	UPROPERTY(BlueprintReadWrite, category = "Variable | Movement | Jump")
	float OriGravityScale = 1;

	UPROPERTY(BlueprintReadWrite, category = "Variable | Movement | SmashDown")
	bool IsSmashingDown = false;

	UPROPERTY(BlueprintReadWrite, category = "Variable | Movement | SmashDown")
	bool IsDead = false;

	UPROPERTY(BlueprintReadWrite, category = "Variable | Movement | SmashDown")
	bool IsUpdatingGravity = false;

	UPROPERTY(BlueprintReadWrite, category = "Variable | Movement | SmashDown")
	bool IsDebugMode = false;

protected :

	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "Function | Movement ")
	void JumpFunction();

	
private:
	
	float MoveAxis = 0.f;
	float PrevAxisForIdle = 9999.f;
	
};



