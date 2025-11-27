// Visual Effects CPP Implementation

#include "Actor/CPP_FVX.h"
#include "DrawDebugHelpers.h"
#include "PaperFlipbookComponent.h"
#include "PaperSprite.h"
#include "PaperSpriteComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Utility/Util_BpAsyncVFXFlipbooks.h"
#include "GameFramework/Character.h"

ACPP_FVX::ACPP_FVX()
{
	PrimaryActorTick.bCanEverTick = true;

	VFXPivot = CreateDefaultSubobject<USceneComponent>(TEXT("PC_VFXPivot"));
	RootComponent = VFXPivot;

	VFXFlipbook = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("PC_VFXFlipbook"));
	VFXFlipbook->SetupAttachment(VFXPivot);
	VFXFlipbook->SetRelativeLocation(FVector::ZeroVector);

	TrailSprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("PC_TrailSprite"));
	TrailSprite->SetupAttachment(RootComponent);

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

	FVector S = From, E = To;
	if (!FMath::IsNaN(ForcePlaneY)) { S.Y = ForcePlaneY; E.Y = ForcePlaneY; }

	const FVector2D D2(E.X - S.X, E.Z - S.Z);
	float Len2D = D2.Size();
	if (Len2D <= KINDA_SMALL_NUMBER) return;

	const float PixelUU = 1.f / FMath::Max(PixelsPerUnit, 0.001f);
	Len2D = FMath::Max(0.f, Len2D - 0.5f * PixelUU);

	const FVector Dir(D2.X, 0.f, D2.Y);
	const FRotator Rot = FRotationMatrix::MakeFromX(Dir.GetSafeNormal()).Rotator();

	TrailSprite->SetUsingAbsoluteLocation(true);
	TrailSprite->SetUsingAbsoluteRotation(true);
	TrailSprite->SetUsingAbsoluteScale(true);

	const float LocalScaleX = Len2D / CachedBaseLenLocalUU;
	TrailSprite->SetWorldRotation(Rot);
	TrailSprite->SetWorldLocation(S, false, nullptr, ETeleportType::TeleportPhysics);
	TrailSprite->SetWorldScale3D(FVector(LocalScaleX, 1.f, 1.f));

	if (!TrailMID) TrailMID = TrailSprite->CreateDynamicMaterialInstance(0);
	if (TrailMID)
	{
		TrailMID->SetScalarParameterValue(TEXT("Tiling"), Len2D / FMath::Max(TileWorldUU, 0.001f));
		TrailMID->SetScalarParameterValue(TEXT("Thickness"), Thickness);
		TrailMID->SetScalarParameterValue(TEXT("DashFill"), 1.05f);
		TrailMID->SetScalarParameterValue(TEXT("HeadTaper"), HeadTaper);
		TrailMID->SetScalarParameterValue(TEXT("Fade"), 1.0f);
		TrailMID->SetScalarParameterValue(TEXT("UseDashes"), bUseDashes ? 1.f : 0.f);
	}

	TrailSprite->SetVisibility(true, true);
	TrailSprite->SetHiddenInGame(false);

	GetWorldTimerManager().ClearTimer(TrailFadeTimer);
	if (TrailLife > 0.f && GetWorld())
	{
		TrailFadeStartTime = GetWorld()->GetTimeSeconds();
		GetWorldTimerManager().SetTimer(
			TrailFadeTimer,
			this,
			&ACPP_FVX::TickTrailFade,
			0.016f,
			true
		);
	}
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

void ACPP_FVX::ActivateVFXByName(FName RowName, const FVector& ImpactLoc)
{
	if (!VFXDataTable || RowName.IsNone())
		return;

	if (const FVFXAnimationResolved* Found = CachedRows.Find(RowName))
	{
		PlayResolvedLocation(*Found, ImpactLoc);
		return;
	}

	if (InFlight.Contains(RowName))
	{
		PendingLocations.FindOrAdd(RowName).Add(ImpactLoc);
		return;
	}

	PendingLocations.FindOrAdd(RowName).Add(ImpactLoc);

	UUtil_BpAsyncVFXFlipbooks* Loader =
		UUtil_BpAsyncVFXFlipbooks::LoadVFXFlipbooksRowAsync(this, VFXDataTable, RowName);

	if (!Loader) return;

	InFlight.Add(RowName, Loader);
	Loader->OnCompleted.AddDynamic(this, &ACPP_FVX::OnLoaderCompleted);
	Loader->OnFailed.AddDynamic(this, &ACPP_FVX::OnLoaderFailed);
	Loader->Activate();
}

void ACPP_FVX::ActivateVFXByNameTransform(FName RowName, const FTransform& WorldTransform)
{
	if (!VFXDataTable || RowName.IsNone())
		return;

	if (const FVFXAnimationResolved* Found = CachedRows.Find(RowName))
	{
		PlayResolvedTransform(*Found, WorldTransform);
		return;
	}

	if (InFlight.Contains(RowName))
	{
		PendingTransforms.FindOrAdd(RowName).Add(WorldTransform);
		return;
	}

	PendingTransforms.FindOrAdd(RowName).Add(WorldTransform);

	UUtil_BpAsyncVFXFlipbooks* Loader =
		UUtil_BpAsyncVFXFlipbooks::LoadVFXFlipbooksRowAsync(this, VFXDataTable, RowName);

	if (!Loader) return;

	InFlight.Add(RowName, Loader);
	Loader->OnCompleted.AddDynamic(this, &ACPP_FVX::OnLoaderCompleted);
	Loader->OnFailed.AddDynamic(this, &ACPP_FVX::OnLoaderFailed);
	Loader->Activate();
}

void ACPP_FVX::BeginPlay()
{
	Super::BeginPlay();

    if (TrailSprite)
    {
        TrailSprite->SetHiddenInGame(true);
        TrailSprite->SetTranslucentSortPriority(10);

        const FBoxSphereBounds BLocal = TrailSprite->GetLocalBounds();
        float LocalWidthUU = BLocal.BoxExtent.X * 2.f; 

        if (LocalWidthUU <= KINDA_SMALL_NUMBER)
        {
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

	if (VFXPivot)
	{
		VFXPivot->SetWorldLocation(ImpactLoc, false, nullptr, ETeleportType::TeleportPhysics);
	}
	SetActorRotation(ComputeLookAtFromPlayer(ImpactLoc), ETeleportType::TeleportPhysics);

	if (VFXFlipbook)
	{
		if (Anim) { VFXFlipbook->SetFlipbook(Anim); }
		VFXFlipbook->SetLooping(bVFXLooping);
		VFXFlipbook->PlayFromStart();
	}

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
			DeActivateVFX(); 
		}
	}
}

void ACPP_FVX::ActivateVFXTransform(UPaperFlipbook* Anim, const FTransform& WorldTransform)
{
	SetActorHiddenInGame(false);
	SetActorTickEnabled(true);

	if (VFXPivot)
	{
		VFXPivot->SetWorldTransform(WorldTransform, false, nullptr, ETeleportType::TeleportPhysics);
	}
	else
	{
		SetActorTransform(WorldTransform, false, nullptr, ETeleportType::TeleportPhysics);
	}
	const FRotator DesiredRot = WorldTransform.GetRotation().Rotator();
	SetActorRotation(DesiredRot, ETeleportType::TeleportPhysics);

	
	if (VFXFlipbook)
	{
		if (Anim) { VFXFlipbook->SetFlipbook(Anim); }
		VFXFlipbook->SetLooping(bVFXLooping);
		VFXFlipbook->PlayFromStart();
	}
	
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
			DeActivateVFX(); 
		}
	}
}

void ACPP_FVX::DeActivateVFX()
{
	GetWorldTimerManager().ClearTimer(FVXTimer);
	GetWorldTimerManager().ClearTimer(TrailFadeTimer);

	if (TrailMID) TrailMID->SetScalarParameterValue(TEXT("Fade"), 0.f);
	SetTrailVisible(false);

	if (VFXFlipbook)
	{
		VFXFlipbook->Stop();
	}
	VFXPivot->SetWorldLocation(OriginLoc, false, nullptr, ETeleportType::TeleportPhysics);
	SetActorHiddenInGame(true);
	SetActorTickEnabled(false);
	PoolObjectToGameMode();

}


void ACPP_FVX::TickTrailFade()
{
	UWorld* World = GetWorld();
	if (!World || !TrailMID || !TrailSprite)
	{
		GetWorldTimerManager().ClearTimer(TrailFadeTimer);
		return;
	}

	const float t = (World->GetTimeSeconds() - TrailFadeStartTime) / FMath::Max(TrailLife, 0.001f);
	const float FadeVal = 1.f - FMath::Clamp(t, 0.f, 1.f);
	TrailMID->SetScalarParameterValue(TEXT("Fade"), FadeVal);

	if (t >= 1.f)
	{
		GetWorldTimerManager().ClearTimer(TrailFadeTimer);
		TrailSprite->SetHiddenInGame(true);
	}
}

FRotator ACPP_FVX::ComputeLookAtFromPlayer(const FVector& Target) const
{
	ACharacter* Player = UGameplayStatics::GetPlayerCharacter(this, 0);
	const FVector Start = Player ? Player->GetActorLocation() : GetActorLocation();
	return UKismetMathLibrary::FindLookAtRotation(Start, Target);
}

void ACPP_FVX::OnLoaderCompleted(FName RowName, const FVFXAnimationResolved& VFX)
{
	CachedRows.Add(RowName, VFX);

	if (TObjectPtr<UUtil_BpAsyncVFXFlipbooks>* L = InFlight.Find(RowName))
	{
		if (L->Get())
		{
			L->Get()->OnCompleted.RemoveAll(this);
			L->Get()->OnFailed.RemoveAll(this);
		}
	}
	InFlight.Remove(RowName);

	if (TArray<FTransform>* Transforms = PendingTransforms.Find(RowName))
	{
		const FTransform UseTransform = Transforms->Last();
		PendingTransforms.Remove(RowName);

		PlayResolvedTransform(VFX, UseTransform);
		return;
	}

	if (TArray<FVector>* Locations = PendingLocations.Find(RowName))
	{
		const FVector UseLoc = Locations->Last();
		PendingLocations.Remove(RowName);

		PlayResolvedLocation(VFX, UseLoc);
		return;
	}
}

void ACPP_FVX::OnLoaderFailed()
{
	InFlight.Reset();
	PendingLocations.Reset();
	PendingTransforms.Reset();
}

void ACPP_FVX::PlayResolvedInternal(const FVFXAnimationResolved& Resolved, const FTransform& WorldTransform, bool bAllowFace)
{
	// Be defensive: only trust AnimVar1 and AnimVar2.
	UPaperFlipbook* Anim = nullptr;

	if (Resolved.AnimVar1)
	{
		Anim = Resolved.AnimVar1;
	}
	else if (Resolved.AnimVar2)
	{
		Anim = Resolved.AnimVar2;
	}

	// Extra safety checks – cooked build is crashing because Anim is a bogus pointer (0xffffffffffffffff).
	// 1) Component must exist.
	if (!VFXFlipbook)
	{
		UE_LOG(LogTemp, Warning, TEXT("[FVX] PlayResolvedInternal: VFXFlipbook is null, aborting."));
		return;
	}

	// 2) Must actually have an animation.
	if (!Anim)
	{
		UE_LOG(LogTemp, Warning, TEXT("[FVX] PlayResolvedInternal: Resolved has no valid AnimVar1/AnimVar2, aborting."));
		return;
	}

	// 3) Pointer sanity check: in the crash, this was 0xffffffffffffffff.
	//    Real UObjects live in normal user space (0x0000... range), not with all high bits set.
	const UPTRINT PtrVal = reinterpret_cast<UPTRINT>(Anim);
	const UPTRINT HighBitsMask = (UPTRINT)0xFFFF000000000000ULL;

	if ((PtrVal & HighBitsMask) == HighBitsMask || PtrVal == (UPTRINT)0)
	{
		UE_LOG(LogTemp, Error, TEXT("[FVX] PlayResolvedInternal: Anim pointer looks invalid (0x%p). Skipping to avoid crash."), Anim);
		return;
	}

	// ---- Original logic below (unchanged) ----

	SetActorHiddenInGame(false);
	SetActorTickEnabled(true);

	// Apply transform to pivot if it exists, otherwise to the actor
	if (VFXPivot)
	{
		VFXPivot->SetWorldTransform(WorldTransform, false, nullptr, ETeleportType::TeleportPhysics);
	}
	else
	{
		SetActorTransform(WorldTransform, false, nullptr, ETeleportType::TeleportPhysics);
	}

	// Rotation: either face player or use the transform rotation
	if (bAllowFace)
	{
		const FVector Loc = WorldTransform.GetLocation();
		SetActorRotation(ComputeLookAtFromPlayer(Loc), ETeleportType::TeleportPhysics);
	}
	else
	{
		const FRotator DesiredRot = WorldTransform.GetRotation().Rotator();
		SetActorRotation(DesiredRot, ETeleportType::TeleportPhysics);
	}

	// Play the flipbook
	VFXFlipbook->SetFlipbook(Anim);
	VFXFlipbook->SetLooping(bVFXLooping);
	VFXFlipbook->SetPlaybackPosition(0.f, false);
	VFXFlipbook->PlayFromStart();

	// Clear any previous auto-deactivate timer
	GetWorldTimerManager().ClearTimer(FVXTimer);

	// Schedule auto-deactivate for non-looping VFX
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
			DeActivateVFX();
		}
	}
}

void ACPP_FVX::PlayResolvedLocation(const FVFXAnimationResolved& Resolved, const FVector& WorldLocation)
{
	PlayResolvedInternal(Resolved, FTransform(WorldLocation), true);
}

void ACPP_FVX::PlayResolvedTransform(const FVFXAnimationResolved& Resolved, const FTransform& WorldTransform)
{
	PlayResolvedInternal(Resolved, WorldTransform, false);
}

void ACPP_FVX::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}




