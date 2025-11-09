// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/HealthComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

// Sets default values for this component's properties
UHealthComponent::UHealthComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
	

	// ...
}


// Called when the game starts
void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();
	CanRegenerate = false;
	// ...
	SetCurrentHealth(MaxHealth);

}


// Called every frame
void UHealthComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UHealthComponent::AddHealth(float Amount)
{
	CurrentHealth = FMath::Clamp(CurrentHealth + Amount, 0.0f, MaxHealth);
	if (CurrentHealth <= 0.0f)
	{
		isDead = true;
		OnDead.Broadcast();
	}
	OnHealthChange.Broadcast(CurrentHealth);
}

void UHealthComponent::SetMaxHealth(float NewMaxHealth)
{
	if (NewMaxHealth == MaxHealth) return; // No change
	if( NewMaxHealth < 0.0f) NewMaxHealth = 0.0f;
	MaxHealth = NewMaxHealth;
	OnMaxHealthChange.Broadcast(MaxHealth);
}

void UHealthComponent::SetCurrentHealth(float NewCurrentHealth)
{
	if (NewCurrentHealth == CurrentHealth) return; // No change
	if (NewCurrentHealth < 0.0f) NewCurrentHealth = 0.0f;
	CurrentHealth = FMath::Clamp(NewCurrentHealth, 0.0f, MaxHealth);
	if (CurrentHealth <= 0.0f)
	{
		isDead = true;
		OnDead.Broadcast();
	}
	OnHealthChange.Broadcast(CurrentHealth);
	
}

void UHealthComponent::StartRegenerateHealth()
{
	if (!GetWorld() || isDead)  return;
	UKismetSystemLibrary::K2_ClearAndInvalidateTimerHandle(this, Th_RecoveryTimer);
	CanRegenerate = true;

	GetWorld()->GetTimerManager().SetTimer(
		Th_RecoveryTimer,
		this,
		&UHealthComponent::Timer_RegenerateHealth,
		RegenerateDuration,  // Now clearly different from member variable
		true,
		RegenerateDuration
	);
}

void UHealthComponent::BasicAhhRecovery()
{
	if (isDead) return;
		// Add 0.5 health
		AddHealth(0.5f);

		// Check if health reached maximum
		if (FMath::IsNearlyEqual(GetCurrentHealth(), GetMaxHealth()))
		{
			CanRegenerate = false;
			UKismetSystemLibrary::K2_ClearAndInvalidateTimerHandle(this, Th_RecoveryTimer);
		}
	}

void UHealthComponent::StartMercyInvicible()
{
	IsMercyInvicible = true;
	GetWorld()->GetTimerManager().SetTimer
	(
		Th_MercyInvicTimer,
		this,
		&UHealthComponent::Timer_StopMercyInvicible,
		MercyInvincibilityDuration,  // Now clearly different from member variable
		true,
		MercyInvincibilityDuration
	);
}


void UHealthComponent::Timer_RegenerateHealth()
{
	BasicAhhRecovery();
		// Check if current health is less than max health
		if (GetCurrentHealth() >= GetMaxHealth())
		{
			// Health is full, stop regeneration
			CanRegenerate = false;
			UKismetSystemLibrary::K2_ClearAndInvalidateTimerHandle(this, Th_RecoveryTimer);
			DoneRecovery.Broadcast();
		}
		
}

void UHealthComponent::Timer_StopMercyInvicible()
{
	IsMercyInvicible = false;
}




