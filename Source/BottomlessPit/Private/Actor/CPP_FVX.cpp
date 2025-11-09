// Visual Effects CPP Implementation

#include "Actor/CPP_FVX.h"
#include "DrawDebugHelpers.h"
#include "PaperFlipbookComponent.h"
#include "PaperSprite.h"
#include "PaperSpriteComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameFramework/Character.h"

// Sets default values
ACPP_FVX::ACPP_FVX()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	VFXPivot = CreateDefaultSubobject<USceneComponent>(TEXT("PC_VFXPivot"));
	RootComponent = VFXPivot;

	VFXFlipbook = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("PC_VFXFlipbook"));
	VFXFlipbook->SetupAttachment(VFXPivot);
	VFXFlipbook->SetRelativeLocation(FVector::ZeroVector);

	TrailSprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("PC_TrailSprite"));
	TrailSprite->SetupAttachment(RootComponent);

	// IMPORTANT: make trail absolute so it won't inherit root moves/rotations
	TrailSprite->SetUsingAbsoluteLocation(true);
	TrailSprite->SetUsingAbsoluteRotation(true);
	TrailSprite->SetUsingAbsoluteScale(true);
	TrailSprite->SetRelativeScale3D(FVector::OneVector);

	TrailSprite->SetHiddenInGame(true);
	TrailSprite->SetTranslucentSortPriority(10);
}

void ACPP_FVX::PoolObjectToGameMode_Implementation()
{
}

void ACPP_FVX::ActivateTrail(const FVector& From, const FVector& To)
{
	if (!TrailSprite || !bTrailReady) return;

	// --- 2D plane clamp (side-scroller) ---
	FVector S = From, E = To;
	if (!FMath::IsNaN(ForcePlaneY)) { S.Y = ForcePlaneY; E.Y = ForcePlaneY; }

	// 2D direction & length (XZ)
	const FVector2D D2(E.X - S.X, E.Z - S.Z);
	float Len2D = D2.Size();
	if (Len2D <= KINDA_SMALL_NUMBER) return;

	// Small epsilon so we never visually overrun the impact
	const float PixelUU = 1.f / FMath::Max(PixelsPerUnit, 0.001f);
	Len2D = FMath::Max(0.f, Len2D - 0.5f * PixelUU);

	// ---------- FIXED ROTATION ----------
	// Build a 3D direction in X–Z and let MakeFromX compute the proper Pitch-only rotator.
	const FVector Dir(D2.X, 0.f, D2.Y);                 // (X, Y=0, Z)
	const FRotator Rot = FRotationMatrix::MakeFromX(Dir.GetSafeNormal()).Rotator();
	// ------------------------------------

	// Absolute transforms (no parent inheritance)
	TrailSprite->SetUsingAbsoluteLocation(true);
	TrailSprite->SetUsingAbsoluteRotation(true);
	TrailSprite->SetUsingAbsoluteScale(true);

	// Place at the start (pivot = Center-Left), stretch along +X
	const float LocalScaleX = Len2D / CachedBaseLenLocalUU;
	TrailSprite->SetWorldRotation(Rot);
	TrailSprite->SetWorldLocation(S, false, nullptr, ETeleportType::TeleportPhysics);
	TrailSprite->SetWorldScale3D(FVector(LocalScaleX, 1.f, 1.f));

	// Material params
	if (!TrailMID) TrailMID = TrailSprite->CreateDynamicMaterialInstance(0);
	if (TrailMID)
	{
		TrailMID->SetScalarParameterValue(TEXT("Tiling"), Len2D / FMath::Max(TileWorldUU, 0.001f));
		TrailMID->SetScalarParameterValue(TEXT("Thickness"), Thickness);
		TrailMID->SetScalarParameterValue(TEXT("DashFill"), 1.05f); // slight overlap to kill gaps
		TrailMID->SetScalarParameterValue(TEXT("HeadTaper"), HeadTaper);
		TrailMID->SetScalarParameterValue(TEXT("Fade"), 1.0f);
		TrailMID->SetScalarParameterValue(TEXT("UseDashes"), bUseDashes ? 1.f : 0.f);
	}

	TrailSprite->SetVisibility(true, true);
	TrailSprite->SetHiddenInGame(false);

	// Timed fade
	GetWorldTimerManager().ClearTimer(TrailFadeTimer);
	if (TrailLife > 0.f)
	{
		const float StartTime = GetWorld()->GetTimeSeconds();
		GetWorldTimerManager().SetTimer(
			TrailFadeTimer,
			[this, StartTime]()
			{
				if (!TrailMID) return;
				const float t = (GetWorld()->GetTimeSeconds() - StartTime) / FMath::Max(TrailLife, 0.001f);
				const float FadeVal = 1.f - FMath::Clamp(t, 0.f, 1.f);
				TrailMID->SetScalarParameterValue(TEXT("Fade"), FadeVal);
				if (t >= 1.f)
				{
					GetWorldTimerManager().ClearTimer(TrailFadeTimer);
					TrailSprite->SetHiddenInGame(true);
				}
			},
			0.016f, true
		);
	}

	//UE_LOG(LogTemp, Warning, TEXT("(Len=%.2f, Base=%.2f, CompRelX=%.3f, LocalScaleX=%.3f)"),Len2D, CachedBaseLenLocalUU, LocalScaleX, LocalScaleX);
}

void ACPP_FVX::SetTrailVisible(bool bVisible)
{
	if (!TrailSprite) return;
	TrailSprite->SetHiddenInGame(!bVisible);
	TrailSprite->SetVisibility(bVisible, true);
}

void ACPP_FVX::ActivateBulletImpact(UPaperFlipbook* BurstAnim, const FVector& Muzzle, const FVector& Impact)
{
	ActivateTrail(Muzzle, Impact);
	ActivateVFX(BurstAnim, Impact);
}

// Called when the game starts or when spawned
void ACPP_FVX::BeginPlay()
{
	Super::BeginPlay();
	//FVXFlipbook->SetLooping(false);
	
    if (TrailSprite)
    {
        TrailSprite->SetHiddenInGame(true);
        TrailSprite->SetTranslucentSortPriority(10);

        // Get unscaled LOCAL bounds (no world/relative scale applied).
        const FBoxSphereBounds BLocal = TrailSprite->GetLocalBounds();
        float LocalWidthUU = BLocal.BoxExtent.X * 2.f; // width along +X in local space

        // Fallback if the asset has zero bounds (rare)
        if (LocalWidthUU <= KINDA_SMALL_NUMBER)
        {
            // 16x2 sprite imported at PixelsPerUnit
            LocalWidthUU = FMath::Max(1e-3f, TrailSourceWidthPx / FMath::Max(PixelsPerUnit, 0.001f));
        }

        CachedBaseLenLocalUU = FMath::Max(LocalWidthUU, 1e-3f);
        bTrailReady = true;
    }
}

void ACPP_FVX::ActivateVFX(UPaperFlipbook* Anim, const FVector& ImpactLoc)
{
	SetActorHiddenInGame(false);
	SetActorTickEnabled(true);

	// 1) Move + face target (teleport physics, like your K2_SetWorldLocation with bTeleport=true)
	if (VFXPivot)
	{
		VFXPivot->SetWorldLocation(ImpactLoc, false, nullptr, ETeleportType::TeleportPhysics);
	}
	SetActorRotation(ComputeLookAtFromPlayer(ImpactLoc), ETeleportType::TeleportPhysics);

	// 2) Set flipbook + play
	if (VFXFlipbook)
	{
		if (Anim) { VFXFlipbook->SetFlipbook(Anim); }
		VFXFlipbook->SetLooping(bVFXLooping);
		VFXFlipbook->PlayFromStart();
	}

	// 3) Arm a one-shot timer (GetFlipbookLength + small pad), unless looping
	GetWorldTimerManager().ClearTimer(FVXTimer);

	if (!bVFXLooping && VFXFlipbook && VFXFlipbook->GetFlipbook())
	{
		const float Len = FMath::Max(0.f, VFXFlipbook->GetFlipbookLength());
		const float Delay = Len + ExtraHoldSeconds;
		if (Delay > 0.f)
		{
			GetWorldTimerManager().SetTimer(
				FVXTimer,
				this,
				&ACPP_FVX::DeActivateVFX,
				Delay,
				false
			);
		}
		else
		{
			DeActivateVFX(); // zero-length safety
		}
	}
}

void ACPP_FVX::DeActivateVFX()
{
	// 4) default “finish” behavior (BP can override to return to pool)
	GetWorldTimerManager().ClearTimer(FVXTimer);
	GetWorldTimerManager().ClearTimer(TrailFadeTimer);

	if (TrailMID) TrailMID->SetScalarParameterValue(TEXT("Fade"), 0.f);
	SetTrailVisible(false);

	if (VFXFlipbook)
	{
		VFXFlipbook->Stop();
	}

	// Hide or destroy depending on your pooling strategy
	// If you're using pooling via GM, override this in BP and call GM.I_GM_PoolFVX(self)
	VFXPivot->SetWorldLocation(OriginLoc, false, nullptr, ETeleportType::TeleportPhysics);
	SetActorHiddenInGame(true);
	SetActorTickEnabled(false);
	PoolObjectToGameMode();

}


FRotator ACPP_FVX::ComputeLookAtFromPlayer(const FVector& Target) const
{
	ACharacter* Player = UGameplayStatics::GetPlayerCharacter(this, 0);
	const FVector Start = Player ? Player->GetActorLocation() : GetActorLocation();
	return UKismetMathLibrary::FindLookAtRotation(Start, Target);
}

// Called every frame
void ACPP_FVX::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}




