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

DEFINE_LOG_CATEGORY_STATIC(LogEnemy, Log, All);

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

void ACPP_EnemyParent::SetOverlapEnabled(bool bEnabled)
{
	if (Capsule)
	{
		Capsule->SetGenerateOverlapEvents(bEnabled);
		Capsule->SetCollisionEnabled(bEnabled ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
		if (bEnabled)
		{
			Capsule->SetCollisionResponseToAllChannels(ECR_Ignore);
			Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap); // player/channel -> overlap
		}
	}
}

void ACPP_EnemyParent::ConfigureCollision_Active()
{
	if (!Capsule) Capsule = FindComponentByClass<UCapsuleComponent>();
	if (!Capsule) return;

	Capsule->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Capsule->SetGenerateOverlapEvents(true);
	Capsule->SetCollisionObjectType(ECC_Pawn);

	Capsule->SetCollisionResponseToAllChannels(ECR_Ignore);
	// Overlap player so we can detect touch/hurt, etc.
	Capsule->SetCollisionResponseToChannel(PlayerChannel, ECR_Overlap);
	Capsule->SetCollisionResponseToChannel(ProjectileChannel, ECR_Block);
	// Block bullet traces/projectiles (trace or object channel—BP sets which one)
	Capsule->SetCollisionResponseToChannel(BulletChannel, ECR_Block);
}

void ACPP_EnemyParent::ConfigureCollision_Dying()
{
	if (!Capsule) Capsule = FindComponentByClass<UCapsuleComponent>();
	if (!Capsule) return;

	// During the death anim: be invisible to gameplay
	Capsule->SetGenerateOverlapEvents(false);
	Capsule->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Capsule->SetCollisionResponseToAllChannels(ECR_Ignore);
	// (Explicitly ignore both to be crystal clear)
	Capsule->SetCollisionResponseToChannel(PlayerChannel, ECR_Ignore);
	Capsule->SetCollisionResponseToChannel(BulletChannel, ECR_Ignore);
	Capsule->SetCollisionResponseToChannel(ProjectileChannel, ECR_Ignore);
}

void ACPP_EnemyParent::ConfigureCollision_Pooled()
{
	if (!Capsule) Capsule = FindComponentByClass<UCapsuleComponent>();
	if (!Capsule) return;

	Capsule->SetGenerateOverlapEvents(false);
	Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Capsule->SetCollisionResponseToAllChannels(ECR_Ignore);
}

void ACPP_EnemyParent::ActivateNow_Internal(const FVector& WorldPos)
{
	// visible & ticking
	SetActorLocation(WorldPos);
	SetActorHiddenInGame(false);
	SetActorTickEnabled(true);

	// collision for gameplay
	ConfigureCollision_Active();

	// reset health if present
	if (HealthComp)
	{
		HealthComp->isDead = false;
		HealthComp->SetCurrentHealth(HealthComp->GetMaxHealth());
	}
	if (Sprite) Sprite->SetVisibility(true, true);

	LifeState = EEnemyLifeState::Active;
	bActive = true;

	UE_LOG(LogEnemy, Log, TEXT("[ENEMY] ACTIVATE  %s  this=%p  Pos=(%.0f,%.0f,%.0f)"),
		*GetName(), this, WorldPos.X, WorldPos.Y, WorldPos.Z);

	// (Optional) BP hook
	OnPooledActivated();
}

void ACPP_EnemyParent::DeferredActivateFromPool()
{
	bPendingActivate = false;
	ActivateNow_Internal(PendingActivatePos);
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

void ACPP_EnemyParent::BeginDeath()
{
	// Already dying or pooled? nothing to do.
	if (LifeState == EEnemyLifeState::Dying || LifeState == EEnemyLifeState::Pooled)
		return;

	// Transition: Active -> Dying
	LifeState = EEnemyLifeState::Dying;
	bActive = false;

	if (HealthComp) HealthComp->isDead = true;
	ConfigureCollision_Dying();

	UE_LOG(LogEnemy, Warning, TEXT("[ENEMY] BEGIN_DEATH %s this=%p t=%.3f (LifeState=Dying)"),
		*GetName(), this, GetWorld()->TimeSeconds);
}

void ACPP_EnemyParent::Notify_DeathAnimFinished()
{
	SetActorHiddenInGame(true);
	DeactivateToPool();
}

void ACPP_EnemyParent::ActivateFromPool(const FVector& WorldPos)
{
	// Do NOT allow reuse while the death animation hasn't finished.
	if (LifeState == EEnemyLifeState::Dying)
	{
		UE_LOG(LogEnemy, Warning, TEXT("[ENEMY] ACTIVATE IGNORED (still Dying) %s this=%p"),
			*GetName(), this);
		return;
	}

	// Transition: Pooled -> Active
	LifeState = EEnemyLifeState::Active;
	bActive = true;

	SetActorLocation(WorldPos);
	SetActorHiddenInGame(false);
	SetActorTickEnabled(true);

	ConfigureCollision_Active();

	if (HealthComp)
	{
		HealthComp->isDead = false;
		HealthComp->SetCurrentHealth(HealthComp->GetMaxHealth());
	}
	if (Sprite) Sprite->SetVisibility(true, true);

	UE_LOG(LogEnemy, Log, TEXT("[ENEMY] ACTIVATE %s this=%p Pos=(%.0f,%.0f,%.0f) (LifeState=Active)"),
		*GetName(), this, WorldPos.X, WorldPos.Y, WorldPos.Z);
}

void ACPP_EnemyParent::DeactivateToPool()
{
	// Transition: Dying/Active -> Pooled
	LifeState = EEnemyLifeState::Pooled;
	bActive = false;

	ConfigureCollision_Pooled();

	SetActorTickEnabled(false);
	SetActorHiddenInGame(true);
	SetActorLocation(FVector(0.f, -100000.f, -100000.f)); // park

	UE_LOG(LogEnemy, Log, TEXT("[ENEMY] DEACTIVATE %s this=%p t=%.3f (LifeState=Pooled)"),
		*GetName(), this, GetWorld()->TimeSeconds);
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



