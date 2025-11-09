//GunParent CPP

#include "Actor/CPP_Gun.h"
#include "PaperFlipbookComponent.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
ACPP_Gun::ACPP_Gun()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Create GunPivot FIRST
	GunPivot = CreateDefaultSubobject<USceneComponent>(TEXT("PC_GunPivot"));
	RootComponent = GunPivot; // Now it's valid

	// Then create and attach GunFlipbook
	GunFlipbook = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("PC_GunFlipbook"));
	GunFlipbook->SetupAttachment(GunPivot);
	GunFlipbook->SetRelativeLocation(FVector::ZeroVector);

	Muzzle = CreateDefaultSubobject<USceneComponent>(TEXT("PC_Muzzle"));
	Muzzle->SetupAttachment(GunFlipbook);

    DamageTypeClass = UDamageType::StaticClass();

}

void ACPP_Gun::CoreFireFromMuzzle(const FVector& AimDir)
{
    if (!GetWorld()) return;

    // Lock and normalize to 2D plane
    FVector Dir = AimDir; Dir.Y = 0.f;
    if (!Dir.Normalize()) return;

    const FVector Start = GetMuzzleWorldLocation();
    const FVector End = Start + Dir * TraceRange;

    FCollisionQueryParams QP(SCENE_QUERY_STAT(GunHitscan), false, this);
    if (AActor* OwnerActor = GetOwner()) QP.AddIgnoredActor(OwnerActor);
    QP.AddIgnoredActor(this);

    FHitResult Hit;
    const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, FireTraceChannel, QP);

    if (bDrawDebug)
    {
        const FVector DrawEnd = bHit ? Hit.ImpactPoint : End;
        DrawDebugLine(GetWorld(), Start, DrawEnd, FColor::Yellow, false, 0.12f, 0, 1.5f);
        DrawDebugPoint(GetWorld(), DrawEnd, 6.f, bHit ? FColor::Red : FColor::Green, false, 0.12f);
    }

    if (bHit)
    {
        // TODO: deal damage/impact here if you want it on the gun side
        // UGameplayStatics::ApplyPointDamage(Hit.GetActor(), Damage, Dir, Hit, nullptr, this, DamageTypeClass);
    }
}

FVector ACPP_Gun::GetMuzzleWorldLocation() const
{
    return Muzzle ? Muzzle->GetComponentLocation() : GetActorLocation();
}

void ACPP_Gun::CoreHitscanFromMuzzle_PastCursor()
{
    if (!GetWorld()) return;

    FVector CursorOnPlane;
    if (!GetCursorWorldOnOwnerPlane(CursorOnPlane)) return;

    const FVector Start = GetMuzzleWorldLocation();

    // Forward from muzzle that respects negative scale (flip)
    FVector FwdFlat = Muzzle
        ? Muzzle->GetComponentTransform().TransformVector(FVector::ForwardVector)
        : GetActorTransform().TransformVector(FVector::ForwardVector);
    FwdFlat.Y = 0.f;            // lock to 2D plane
    //ddaFwdFlat.Z = 0.f;            // make snap purely horizontal
    if (!FwdFlat.Normalize())
    {
        // fallback if transform is degenerate
        const AActor* Ref = GetOwner() ? GetOwner() : this;
        const float sign = (Start.X - Ref->GetActorLocation().X) >= 0.f ? 1.f : -1.f;
        FwdFlat = FVector(sign, 0.f, 0.f);
    }

    // Vector toward cursor on plane
    FVector Delta = CursorOnPlane - Start;
    Delta.Y = 0.f;

    // Signed forward distance along the muzzle forward
    const float Along = FVector::DotProduct(Delta, FwdFlat);

    // If the cursor is sufficiently in front, use it; otherwise snap forward
    const FVector Dir = (Along > FrontGateDist && !Delta.IsNearlyZero())
        ? Delta.GetSafeNormal()
        : FwdFlat;

    const FVector End = Start + Dir * TraceRange;

    FCollisionQueryParams QP(SCENE_QUERY_STAT(GunHitscan), false, this);
    if (AActor* OwnerActor = GetOwner()) QP.AddIgnoredActor(OwnerActor);
    QP.AddIgnoredActor(this);

    FHitResult Hit;
    const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, FireTraceChannel, QP);

    if (bDrawDebug)
    {
        const FVector DrawEnd = bHit ? Hit.ImpactPoint : End;
        DrawDebugLine(GetWorld(), Start, DrawEnd, FColor::Yellow, false, 0.12f, 0, 1.5f);
        DrawDebugPoint(GetWorld(), DrawEnd, 6.f, bHit ? FColor::Red : FColor::Green, false, 0.12f);
    }

    if (bHit)
    {
        // Apply damage/impact here
        UGameplayStatics::ApplyPointDamage(Hit.GetActor(), Damage, Dir, Hit, nullptr, this, DamageTypeClass);
        ShootOnHit(Hit);
    }
    else
    {
        // Miss FX to End (tracer, dust, etc.)
    }
}

void ACPP_Gun::MirrorIntoMuzzleFrontHemisphere(const AActor* Owner, const FVector& MuzzleWorld, FVector& DeltaWorld, float DeadZoneDeg)
{
    if (!Owner) return;

    // Define "front" as from body center to muzzle
    FVector Front = MuzzleWorld - Owner->GetActorLocation();
    Front.Y = 0.f;
    if (!Front.Normalize())
    {
        Front = FVector(1, 0, 0); // fallback: world +X
    }

    // Keep everything on your 2D plane
    DeltaWorld.Y = 0.f;

    // If the angle between Delta and Front is > 90° (i.e., behind), reflect it.
    const float dotN = FVector::DotProduct(DeltaWorld.GetSafeNormal(), Front); // normalized dot
    const float deadZoneCos = FMath::Cos(FMath::DegreesToRadians(DeadZoneDeg));

    if (dotN < deadZoneCos) // behind (or just barely off) -> reflect across plane normal to Front
    {
        const float along = FVector::DotProduct(DeltaWorld, Front); // not normalized
        DeltaWorld = DeltaWorld - 2.f * along * Front; // classic reflection: V' = V - 2(V·N)N
    }
}

void ACPP_Gun::ShootOnHit_Implementation(FHitResult ShootResult)
{
    // Your actual logic here
   // UE_LOG(LogTemp, Warning, TEXT("ShootOnHit called with actor: %s"), *GetNameSafe(ShootResult.GetActor()));
}



// Called when the game starts or when spawned
void ACPP_Gun::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogTemp, Warning, TEXT("Root: %s | Flipbook Parent: %s"),
		*GetRootComponent()->GetName(),
		*GunFlipbook->GetAttachParent()->GetName());

}

// Called every frame
void ACPP_Gun::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

bool ACPP_Gun::GetCursorWorldOnOwnerPlane(FVector& OutWorld) const
{
    if(!GetWorld()) return false;
    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (!PC) return false;

    FVector WL, WD;
    if (!PC->DeprojectMousePositionToWorld(WL, WD)) return false;

    const AActor* Ref = GetOwner() ? GetOwner() : this;
    const double ActorY = (double)Ref->GetActorLocation().Y;
    const double DeltaY = ActorY - (double)WL.Y;
    const double DirY = (double)WD.Y;

    const double t = FMath::IsNearlyZero(DirY) ? DeltaY : (DeltaY / DirY); // mirrors your BP select
    OutWorld = WL + WD * (float)t;
    return true;
}




