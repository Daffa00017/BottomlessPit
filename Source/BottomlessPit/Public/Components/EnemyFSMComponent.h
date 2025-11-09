//Enemy Finite State Machine Component Header

#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "EnemyFSMComponent.generated.h"

UENUM(BlueprintType)
enum class EEnemyMode : uint8
{
	Idle        UMETA(DisplayName = "Idle"),
	HoldBand    UMETA(DisplayName = "HoldBand"),     // same-floor standoff (maintain distance + fire)
	Reposition  UMETA(DisplayName = "Reposition"),   // slide along platform to regain LOS
	Dead        UMETA(DisplayName = "Dead")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEnemyStateChanged, EEnemyMode, OldState, EEnemyMode, NewState);

UCLASS(ClassGroup = (AI), meta = (BlueprintSpawnableComponent))
class BOTTOMLESSPIT_API UEnemyFSMComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEnemyFSMComponent();

	// Blackboard keys this FSM writes to:
	UPROPERTY(EditAnywhere, Category = "Blackboard") FName BB_Mode = "Mode";
	UPROPERTY(EditAnywhere, Category = "Blackboard") FName BB_HasLOS = "HasLOS";
	UPROPERTY(EditAnywhere, Category = "Blackboard") FName BB_DistToTarget = "DistToTarget";
	UPROPERTY(EditAnywhere, Category = "Blackboard") FName BB_AttackTarget = "AttackTarget";

	// Sensing / tuning
	UPROPERTY(EditAnywhere, Category = "Sense") float LOSInterval = 0.15f; // 6-7 Hz
	UPROPERTY(EditAnywhere, Category = "Sense") float SightRange = 2000.f;
	UPROPERTY(EditAnywhere, Category = "Sense") float EyeHeight = 30.f;
	UPROPERTY(EditAnywhere, Category = "Sense") TEnumAsByte<ECollisionChannel> LOSChannel = ECC_Visibility;

	// Distance band used for deciding HoldBand vs too far/too close (matches your task)
	UPROPERTY(EditAnywhere, Category = "Combat") float PreferMin = 450.f;
	UPROPERTY(EditAnywhere, Category = "Combat") float PreferMax = 650.f;

	// API
	UPROPERTY(BlueprintAssignable, Category = "FSM") FOnEnemyStateChanged OnStateChanged;

	// Read-only state
	UFUNCTION(BlueprintPure, Category = "FSM") EEnemyMode GetState() const { return State; }

	UFUNCTION(BlueprintCallable, Category = "FSM")
	void StartFSM();

	// Acquire / lose tuning
	UPROPERTY(EditAnywhere, Category = "Acquire")
	float AcquireRange = 1800.f;              // how close before we “lock on”
	UPROPERTY(EditAnywhere, Category = "Acquire")
	bool  bRequireLOSForAcquire = true;       // require LOS to acquire
	UPROPERTY(EditAnywhere, Category = "Acquire")
	float LoseTargetTime = 1.0f;              // how long w/o LOS/range to drop target

	// internal
	float LastSeenTime = -1000.f;

protected:
	virtual void BeginPlay() override;

private:
	TWeakObjectPtr<AAIController> AIC;
	TWeakObjectPtr<UBlackboardComponent> BB;
	TWeakObjectPtr<APawn> Pawn;
	EEnemyMode State = EEnemyMode::Idle;

	FTimerHandle SenseTimer;
	FTimerHandle InitRetryTimer;

	// Core
	void SenseAndDecide();            // runs at LOSInterval
	void ChangeState(EEnemyMode NewState);
	void PushBB(EEnemyMode Mode, bool bHasLOS, float Dist);

	// Helpers
	bool LOS2D(const APawn* Me, const AActor* T, float& OutDist) const;
	const AActor* GetTarget() const;
};



