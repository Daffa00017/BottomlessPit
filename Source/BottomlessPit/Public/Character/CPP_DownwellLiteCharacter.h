// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Delegates/DelegateCombinations.h"
#include "CPP_PaperZDParentCharacter.h"
#include "TimerManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "CPP_DownwellLiteCharacter.generated.h"

class UBoxComponent;

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

	/** Call when jump starts or when you detect “start falling”. */
	UFUNCTION(BlueprintCallable, Category = "Stomp")
	void StartStompWatch();

	/** Call when landing/canceling (Landed will also call this). */
	UFUNCTION(BlueprintCallable, Category = "Stomp")
	void StopStompWatch();

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

	UPROPERTY(BlueprintReadWrite, category = "Variable | Mechanic | Death")
	bool IsDead = false;

	UPROPERTY(BlueprintReadWrite,EditAnywhere, category = "Variable | Mechanic | Death")
	bool IsReadyToRestart = false;

	UPROPERTY(BlueprintReadWrite, category = "Variable | Movement | SmashDown")
	bool IsUpdatingGravity = false;

	UPROPERTY(BlueprintReadWrite, category = "Variable | Movement | SmashDown")
	bool IsDebugMode = false;

	bool GetIsDead() const { return IsDead; }
	bool GetIsReadyToRestart() const { return IsReadyToRestart; }

protected :

	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "Function | Movement ")
	void JumpFunction();

	// --- Tunables ---
	UPROPERTY(EditAnywhere, Category = "Stomp")
	float StompMonitorInterval = 0.01f;     // how often to poll Vz while arming

	UPROPERTY(EditAnywhere, Category = "Stomp")
	float StompMinDownSpeed = 300.f;        // arm when Vz < -this

	UPROPERTY(EditAnywhere, Category = "Stomp|Damage")
	float StompDamage = 1.f;                 // (use in BP overlap if you want)

	UPROPERTY(EditAnywhere, Category = "Stomp|Collision")
	TEnumAsByte<ECollisionChannel> EnemyChannel = ECC_GameTraceChannel1;

	UPROPERTY(VisibleAnywhere, Category = "Stomp|Sensor")
	UBoxComponent* StompSensor = nullptr;

	UPROPERTY(EditAnywhere, Category = "Stomp|Sensor")
	float StompSensorOffsetZ = -30.f;

	UPROPERTY(EditAnywhere, Category = "Stomp|Sensor")
	FVector StompSensorExtent = FVector(20.f, 10.f, 10.f);

	// Timers/state
	FTimerHandle Timer_StompMonitor;

	// internals
	void StompMonitorTick();
	void BeginStompActive();         // latched ON until Landed
	void SetStompCollisionEnabled(bool bEnabled);

	bool bStompActive = false;
	float LastStompTime = -1000.f;

	
private:
	
	float MoveAxis = 0.f;
	float PrevAxisForIdle = 9999.f;
	
};



