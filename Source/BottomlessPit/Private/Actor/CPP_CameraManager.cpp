// CPP_CameraManager.cpp  (Base)
#include "Actor/CPP_CameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

ACPP_CameraManager::ACPP_CameraManager()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("RootComp"));
	SetRootComponent(RootComp);

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->bDoCollisionTest = false;          // base: no camera collision
	SpringArm->SetUsingAbsoluteRotation(true);    // stable orientation

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
}

void ACPP_CameraManager::BeginPlay()
{
	Super::BeginPlay();

	// Apply initial knobs
	SpringArm->TargetArmLength = ArmLength;
	SpringArm->AddLocalOffset(ArmLocalOffset);
	SpringArm->SetRelativeRotation(ArmRotation);

	// If no explicit target, default to player 0 pawn
	if (!FollowTarget)
	{
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
		{
			FollowTarget = PC ? PC->GetPawn() : nullptr;
		}
	}

	// Projection
	Camera->ProjectionMode = bUseOrthographic ? ECameraProjectionMode::Orthographic
		: ECameraProjectionMode::Perspective;
	if (bUseOrthographic)
	{
		Camera->OrthoWidth = OrthoWidth;
	}

	// If you liked your SpringArm lag feeling, keep it here:
	// SpringArm->bEnableCameraLag = true;
	// SpringArm->CameraLagSpeed   = /* CamLagSpeed equivalent */;
}

void ACPP_CameraManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Auto-expire timed focus
	if (bFocusActive && FocusKeepSeconds > 0.f)
	{
		FocusTimeLeft -= DeltaSeconds;
		if (FocusTimeLeft <= 0.f)
		{
			CancelFocus();
		}
	}

	FVector Desired;

	if (IsFocusing())
	{
		const FVector Curr = GetActorLocation();
		FVector Goal = FocusTarget->GetActorLocation();
		if (!bFocusCenterX) Goal.X = Curr.X;     // keep current X if you don’t want horizontal centering
		Goal.Z += FocusZOffset;

		// Lerp with dedicated focus speed (keeps your normal FollowLerpSpeed for non-focus)
		Desired = FMath::VInterpTo(Curr, Goal, DeltaSeconds, FocusLerp);
	}
	else
	{
		Desired = ComputeDesiredLocation(DeltaSeconds); // your child logic (downwell/sidescroll)
	}

	const FVector NewLoc = FMath::VInterpTo(GetActorLocation(), Desired, DeltaSeconds, FollowLerpSpeed);
	SetActorLocation(NewLoc);

	// (keep your hot knobs code)
	SpringArm->TargetArmLength = ArmLength;
	SpringArm->SetRelativeRotation(ArmRotation);
	Camera->ProjectionMode = bUseOrthographic ? ECameraProjectionMode::Orthographic : ECameraProjectionMode::Perspective;
	if (bUseOrthographic) { Camera->OrthoWidth = OrthoWidth; }
}

void ACPP_CameraManager::SetFollowTarget(AActor* NewTarget)
{
	FollowTarget = NewTarget;
}

void ACPP_CameraManager::AdoptAsViewTarget(float BlendTime)
{
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		PC->SetViewTargetWithBlend(this, BlendTime);
	}
}

void ACPP_CameraManager::FocusOnActor(AActor* Target, float LerpSpeed, bool bCenterX, float ZOffset, float KeepSeconds)
{
	if (!IsValid(Target))
	{
		CancelFocus();
		return;
	}
	FocusTarget = Target;
	bFocusActive = true;
	FocusLerp = FMath::Max(0.f, LerpSpeed);
	bFocusCenterX = bCenterX;
	FocusZOffset = ZOffset;
	FocusKeepSeconds = FMath::Max(0.f, KeepSeconds);
	FocusTimeLeft = FocusKeepSeconds;
}

void ACPP_CameraManager::FocusOnPlayer(float LerpSpeed, bool bCenterX, float ZOffset, float KeepSeconds)
{
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		if (APawn* P = PC->GetPawn())
		{
			FocusOnActor(P, LerpSpeed, bCenterX, ZOffset, KeepSeconds);
			return;
		}
	}
	CancelFocus();
}

void ACPP_CameraManager::CancelFocus()
{
	bFocusActive = false;
	FocusTarget = nullptr;
	FocusTimeLeft = 0.f;
}



