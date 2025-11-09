// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Engine/EngineTypes.h"
#include "Components/HealthComponent.h"
#include "Enum/AIMovementState.h"
#include "Utility/Util_BpAsyncEnemyAnim.h"
#include "CPP_EnemyParent.generated.h"

class UCapsuleComponent;
class UPaperZDAnimationComponent;
class UPaperZDAnimSequence;
class UPaperZDAnimInstance;
class UPaperFlipbookComponent;
class UFloatingPawnMovement;
class UHealthComponent;
class UUtil_BpAsyncEnemyAnim;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAIMoveStateChanged, EAIMovementState, Old, EAIMovementState, New);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemyAnimsLoaded, const FEnemyAnimResolved&, LoadedAnims);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEnemyAnimsFailed);

UCLASS()
class BOTTOMLESSPIT_API ACPP_EnemyParent : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ACPP_EnemyParent();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UPROPERTY(BlueprintAssignable, Category = "Animations")
	FOnEnemyAnimsLoaded OnAnimsReady;

	/** * YOUR NEW DELEGATE: Fired if loading fails.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Animations")
	FOnEnemyAnimsFailed OnAnimsLoadFailed;

	/** Stores the final loaded anims. Read-only for Blueprints. */
	UPROPERTY(BlueprintReadOnly, Category = "Animations")
	FEnemyAnimResolved ResolvedAnims;

	// --- Components ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCapsuleComponent* Capsule;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UPaperFlipbookComponent* Sprite;

	/** Lightweight movement good for 2D pawns */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	UFloatingPawnMovement* MoveComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UHealthComponent* HealthComp;

	/** Current movement state for PaperZD, UI, etc. */
	UPROPERTY(BlueprintReadOnly, Category = "AI|State")
	EAIMovementState AIMoveState = EAIMovementState::Idle;

	/** Lightweight change API (no tick). Safe to call from tasks/services. */
	UFUNCTION(BlueprintCallable, Category = "AI|State")
	void SetAIMoveState(EAIMovementState NewState);

	UPROPERTY(BlueprintAssignable, Category = "AI|State")
	FOnAIMoveStateChanged OnAIMoveStateChanged;

	// --- Tuning (optional) ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move")
	float MaxSpeed = 800.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move")
	float WalkSpeed = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move")
	float RunSpeed = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move")
	float Accel = 1800.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move")
	float Friction = 8.f;

	// Drive the arm flipbook via PaperZD
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Anim")
	TObjectPtr<UPaperZDAnimationComponent> BodyAnim;

	// Optional: default anim instance class just for the arm (if you use state machines)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Anim")
	TSubclassOf<UPaperZDAnimInstance> BodyAnimInstanceClass;

	//Pool Enemy

	bool bActive = false;

	UFUNCTION(BlueprintCallable) bool IsActive() const { return bActive; }

	UFUNCTION(BlueprintCallable)
	virtual void ActivateFromPool(const FVector& WorldPos);

	UFUNCTION(BlueprintCallable)
	virtual void DeactivateToPool();

	// So Blueprint can just call this on “death” (no interface needed)
	UFUNCTION(BlueprintCallable) void RequestDeactivate() 
	{ 
		DeactivateToPool(); 
	}

	// Optional: let BP react to pool events
	UFUNCTION(BlueprintImplementableEvent) void OnPooledActivated();
	UFUNCTION(BlueprintImplementableEvent) void OnPooledDeactivated();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animations")
	FName AnimationRowName;

	/** * Set this in your Blueprint children.
	 * This tells the class WHICH table to load from.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animations")
	UDataTable* AnimationDataTable;

private:
	/** This UPROPERTY() is crucial to stop the async action from being garbage collected. */
	UPROPERTY()
	UUtil_BpAsyncEnemyAnim* AnimLoadAction;

	/** C++ function that is called by the async action on success */
	UFUNCTION()
	void HandleAnimsLoaded_Internal(FName RowName, const FEnemyAnimResolved& Anim);

	/** C++ function that is called by the async action on failure */
	UFUNCTION()
	void HandleAnimLoadFailed_Internal();

};



