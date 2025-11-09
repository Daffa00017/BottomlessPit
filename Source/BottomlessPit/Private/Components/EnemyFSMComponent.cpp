//Enemy Finite State Machine Component CPP Implementation


#include "Components/EnemyFSMComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Actor.h"

UEnemyFSMComponent::UEnemyFSMComponent()
{
	PrimaryComponentTick.bCanEverTick = false; // we use a timer instead
}

void UEnemyFSMComponent::BeginPlay()
{
	Super::BeginPlay();
	GetWorld()->GetTimerManager().SetTimer(InitRetryTimer, this, &UEnemyFSMComponent::StartFSM, 0.10f, false);
}

void UEnemyFSMComponent::StartFSM()
{
	AIC = Cast<AAIController>(GetOwner());
	BB = AIC.IsValid() ? AIC->GetBlackboardComponent() : nullptr;
	Pawn = AIC.IsValid() ? AIC->GetPawn() : nullptr;

	if (!AIC.IsValid() || !BB.IsValid() || !Pawn.IsValid())
	{
		// Blackboard or Pawn not ready yet → try again shortly
		GetWorld()->GetTimerManager().SetTimer(InitRetryTimer, this, &UEnemyFSMComponent::StartFSM, 0.10f, false);
		return;
	}

	// Ready → start sensing on a fixed interval
	const float Interval = FMath::Max(0.05f, LOSInterval);
	GetWorld()->GetTimerManager().SetTimer(SenseTimer, this, &UEnemyFSMComponent::SenseAndDecide, Interval, true, Interval);
}

void UEnemyFSMComponent::SenseAndDecide()
{
    if (!BB.IsValid()) return;

    // 1) Look up the player (you can cache if you like)
    APawn* Me = Pawn.Get();
    APawn* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    const float Now = GetWorld()->GetTimeSeconds();

    // 2) Decide acquire/lose and set/clear BB.AttackTarget
    bool bHasTarget = false;
    if (Me && Player)
    {
        // range check (2D)
        FVector D = Player->GetActorLocation() - Me->GetActorLocation();
        D.Y = 0.f; // XZ plane
        const float Dist = D.Size();

        bool bLOSok = true;
        if (bRequireLOSForAcquire)
        {
            float Dummy;
            bLOSok = LOS2D(Me, Player, Dummy);
        }

        if (Dist <= AcquireRange && bLOSok)
        {
            // acquire or refresh target
            BB->SetValueAsObject(BB_AttackTarget, Player);
            LastSeenTime = Now;
            bHasTarget = true;
        }
        else
        {
            // if we recently saw the player, keep target until grace passes
            const bool bGrace = (Now - LastSeenTime) <= LoseTargetTime;
            if (!bGrace)
            {
                BB->ClearValue(BB_AttackTarget);
            }
            else
            {
                bHasTarget = (BB->GetValueAsObject(BB_AttackTarget) != nullptr);
            }
        }
    }
    else
    {
        BB->ClearValue(BB_AttackTarget);
    }

    // 3) Recompute LOS/Dist only if we have a target
    bool bLOS = false;
    float DistTo = 0.f;
    if (bHasTarget)
    {
        APawn* MePtr = Pawn.Get();
        const AActor* T = Cast<AActor>(BB->GetValueAsObject(BB_AttackTarget));
        if (MePtr && T)
        {
            bLOS = LOS2D(MePtr, T, DistTo);
        }
    }

    // 4) Pick Mode
    EEnemyMode NewMode = EEnemyMode::Idle;
    if (bHasTarget)
    {
        NewMode = bLOS ? EEnemyMode::HoldBand : EEnemyMode::Reposition;
    }

    // 5) Push BB + state
    PushBB(NewMode, bLOS, DistTo);
    if (NewMode != State) ChangeState(NewMode);
}

void UEnemyFSMComponent::ChangeState(EEnemyMode NewState)
{
	const EEnemyMode Old = State;
	State = NewState;
	OnStateChanged.Broadcast(Old, NewState);

	// Optional: you can also clear flags on certain transitions:
	// if (NewState == EEnemyMode::HoldBand && BB.IsValid())
	// { BB->SetValueAsBool(TEXT("RequestNewStrafe"), false); }
}

void UEnemyFSMComponent::PushBB(EEnemyMode Mode, bool bHasLOS, float Dist)
{
	if (!BB.IsValid()) return;

	BB->SetValueAsEnum(BB_Mode, static_cast<uint8>(Mode));
	BB->SetValueAsBool(BB_HasLOS, bHasLOS);
	BB->SetValueAsFloat(BB_DistToTarget, Dist);
}

bool UEnemyFSMComponent::LOS2D(const APawn* Me, const AActor* T, float& OutDist) const
{
	if (!Me || !T) { OutDist = 0.f; return false; }

	const FVector M = Me->GetActorLocation();
	const FVector P = T->GetActorLocation();
	FVector D = P - M; D.Y = 0.f;               // XZ side-scroller; set D.Z=0 for XY
	OutDist = D.Size();
	if (OutDist > SightRange) return false;

	const FVector Start = FVector(M.X, M.Y, M.Z + EyeHeight);
	const FVector End = Start + D.GetSafeNormal() * (OutDist + 1.f);

	FHitResult Hit;
	FCollisionQueryParams Q(SCENE_QUERY_STAT(FSM_LOS), false, Me);
	if (!Me->GetWorld()->LineTraceSingleByChannel(Hit, Start, End, LOSChannel, Q))
		return true; // nothing hit

	return Hit.GetActor() == T;
}

const AActor* UEnemyFSMComponent::GetTarget() const
{
	if (!BB.IsValid()) return nullptr;
	return Cast<AActor>(BB->GetValueAsObject(BB_AttackTarget));
}




