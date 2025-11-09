// Fill out your copyright notice in the Description page of Project Settings.
//HealthComponent Header
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeadDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDoneRecoveryDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealthChangeDelegate, float, NewHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMaxHealthChangeDelegate, float, NewMaxHealth);


UCLASS( Blueprintable,ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BOTTOMLESSPIT_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UHealthComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	float CurrentHealth;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	float MaxHealth = 6.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	float RegenerateDuration = 3.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	float MercyInvincibilityDuration = 3.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	bool CanRegenerate = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	bool IsMercyInvicible = false;
	UPROPERTY(BlueprintReadWrite, Category = "Health")
	FTimerHandle Th_RecoveryTimer;
	UPROPERTY(BlueprintReadWrite, Category = "Health")
	FTimerHandle Th_MercyInvicTimer;



public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	bool isDead = false;
	/** Function to apply damage to the health component */

	UFUNCTION(BlueprintCallable, Category = "Health")
	void AddHealth(float Amount);
	UFUNCTION(BlueprintCallable, Category = "Health")
	void SetMaxHealth(float NewMaxHealth);
	UFUNCTION(BlueprintCallable, Category = "Health")
	void SetCurrentHealth(float NewCurrentHealth);
	UFUNCTION(BlueprintCallable, Category = "Health")
	void StartRegenerateHealth();
	UFUNCTION(BlueprintCallable, Category = "Health")
	void BasicAhhRecovery();
	UFUNCTION(BlueprintCallable, Category = "Health")
	void StartMercyInvicible();
	UFUNCTION()
	void Timer_RegenerateHealth();
	UFUNCTION()
	void Timer_StopMercyInvicible();
	
	UFUNCTION(BlueprintCallable, Category = "Health")
	float GetCurrentHealth() const { return CurrentHealth; }
	UFUNCTION(BlueprintCallable, Category = "Health")
	float GetMaxHealth() const { return MaxHealth; }


	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnDeadDelegate OnDead;
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnHealthChangeDelegate OnHealthChange;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnMaxHealthChangeDelegate OnMaxHealthChange;
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FDoneRecoveryDelegate DoneRecovery;


};



