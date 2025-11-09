// Fill out your copyright notice in the Description page of Project Settings.


#include "Pawn/Enemy/CPP_EnemyParent.h"

#include "Components/CapsuleComponent.h"
#include "PaperZDAnimationComponent.h"
#include "PaperZDAnimInstance.h"
#include "PaperFlipbookComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Kismet/GameplayStatics.h"
#include "Utility/Util_BpAsyncEnemyAnim.h"
#include "AIController.h"


// Sets default values
ACPP_EnemyParent::ACPP_EnemyParent()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	HealthComp = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
	// Root collision
	Capsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule"));
	SetRootComponent(Capsule);
	Capsule->InitCapsuleSize(18.f, 44.f);
	Capsule->SetCollisionProfileName(TEXT("Pawn"));
	Capsule->SetCanEverAffectNavigation(false);

	// Visual (Paper2D/PaperZD)
	Sprite = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("Sprite"));
	Sprite->SetupAttachment(RootComponent);
	Sprite->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Movement (lightweight)
	MoveComp = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("MoveComp"));
	MoveComp->SetPlaneConstraintEnabled(true);
	// Side-scroller on XZ plane: lock Y
	MoveComp->SetPlaneConstraintNormal(FVector(0.f, 1.f, 0.f));

	// Allow AI possession
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	// We'll set AIControllerClass from C++ or the Blueprint (see controller below)

	BodyAnim = CreateDefaultSubobject<UPaperZDAnimationComponent>(TEXT("PC_BodyAnim"));
}

void ACPP_EnemyParent::HandleAnimsLoaded_Internal(FName RowName, const FEnemyAnimResolved& Anim)
{
	UE_LOG(LogTemp, Log, TEXT("Anims loaded for: %s"), *RowName.ToString());

	// 1. Store the anims so you can use them in C++
	ResolvedAnims = Anim;

	// 2. As you said, no special C++ interface logic. Keeping it simple.

	// 3. Broadcast the BLUEPRINT delegate!
	OnAnimsReady.Broadcast(ResolvedAnims);
}

void ACPP_EnemyParent::HandleAnimLoadFailed_Internal()
{
	UE_LOG(LogTemp, Error, TEXT("FAILED to load anims for row: %s"), *AnimationRowName.ToString());

	// Broadcast the BLUEPRINT fail delegate
	OnAnimsLoadFailed.Broadcast();
}

void ACPP_EnemyParent::SetAIMoveState(EAIMovementState NewState)
{
	if (AIMoveState == NewState) return;      // no spam
	const EAIMovementState Old = AIMoveState;
	AIMoveState = NewState;
	OnAIMoveStateChanged.Broadcast(Old, NewState);
}

void ACPP_EnemyParent::ActivateFromPool(const FVector& WorldPos)
{
	HealthComp->isDead = false;
	HealthComp->SetCurrentHealth(HealthComp->GetMaxHealth());
	SetActorLocation(WorldPos);
	SetActorHiddenInGame(false);
	SetActorTickEnabled(true);
	SetActorEnableCollision(true);
	EAIMovementState::Idle;
	bActive = true;
	OnPooledActivated();
	// reset movement / state here
}

void ACPP_EnemyParent::DeactivateToPool()
{
	HealthComp->isDead = true;
	
	SetActorEnableCollision(false);
	bActive = false;
	OnPooledDeactivated();
	// clear targets, velocities, etc.
}

// Called when the game starts or when spawned
void ACPP_EnemyParent::BeginPlay()
{
	Super::BeginPlay();
	
	if (MoveComp)
	{
		MoveComp->MaxSpeed = MaxSpeed;
		MoveComp->Acceleration = Accel;
		MoveComp->Deceleration = FMath::Max(1000.f, Friction * 100.f);
	}

	if (AnimationDataTable && !AnimationRowName.IsNone())
	{
		// 1. Create the Async Action
		AnimLoadAction = UUtil_BpAsyncEnemyAnim::LoadEnemyAnimRowAsync(
			this,
			AnimationDataTable,
			AnimationRowName,
			-1 // VariantIndex
		);

		if (AnimLoadAction)
		{
			// 2. Bind our C++ functions to the action's delegates
			//
			//    *** THIS IS THE FIX ***
			//    Use your class name 'ACPP_EnemyParent' not 'AMyBaseEnemy'
			//
			AnimLoadAction->OnCompleted.AddDynamic(this, &ACPP_EnemyParent::HandleAnimsLoaded_Internal);
			AnimLoadAction->OnFailed.AddDynamic(this, &ACPP_EnemyParent::HandleAnimLoadFailed_Internal);

			// 3. Start the action
			AnimLoadAction->Activate();
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Enemy '%s' has no AnimationDataTable or AnimationRowName set."), *GetName());
		// Instantly fire the fail delegate
		OnAnimsLoadFailed.Broadcast();
	}
}

// Called every frame
void ACPP_EnemyParent::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void ACPP_EnemyParent::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}



