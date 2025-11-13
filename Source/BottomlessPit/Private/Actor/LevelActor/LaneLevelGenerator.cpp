// laneLevel Implementation


#include "Actor/LevelActor/LaneLevelGenerator.h"
#include "Components/BoxComponent.h"
#include "DrawDebugHelpers.h"
#include "PaperSprite.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Actor/CameraManager/ACPP_DownwellCamera.h"
#include "GameFramework/MovementComponent.h"
#include "EngineUtils.h"
#include "Engine/Engine.h" 


//--Helpers--

inline float LaneGridLeftX_Local(int32 NumLanes, float LaneWidthUU)
{
	return -0.5f * (float(NumLanes) * LaneWidthUU);
}

inline float LaneCenterX_Local(int32 Lane, int32 NumLanes, float LaneWidthUU)
{
	const float left = LaneGridLeftX_Local(NumLanes, LaneWidthUU);
	const int32 idx = (Lane % NumLanes + NumLanes) % NumLanes;
	return left + (idx + 0.5f) * LaneWidthUU;
}

// Sets default values
ALaneLevelGenerator::ALaneLevelGenerator()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	LeftSeam = CreateDefaultSubobject<UBoxComponent>(TEXT("LeftSeam"));
	RightSeam = CreateDefaultSubobject<UBoxComponent>(TEXT("RightSeam"));
	LeftSeam->SetupAttachment(Root);
	RightSeam->SetupAttachment(Root);

	// seams collide as overlaps
	LeftSeam->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	LeftSeam->SetCollisionResponseToAllChannels(ECR_Ignore);
	LeftSeam->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	RightSeam->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	RightSeam->SetCollisionResponseToAllChannels(ECR_Ignore);
	RightSeam->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	LeftSeam->SetGenerateOverlapEvents(true);
	RightSeam->SetGenerateOverlapEvents(true);
	LeftSeam->OnComponentBeginOverlap.AddDynamic(this, &ALaneLevelGenerator::OnLeftSeamBeginOverlap);
	RightSeam->OnComponentBeginOverlap.AddDynamic(this, &ALaneLevelGenerator::OnRightSeamBeginOverlap);

	//  CREATE WALLS HERE (not in OnConstruction)
	LeftWall = CreateDefaultSubobject<UBoxComponent>(TEXT("LeftWall"));
	RightWall = CreateDefaultSubobject<UBoxComponent>(TEXT("RightWall"));
	LeftWall->SetupAttachment(Root);
	RightWall->SetupAttachment(Root);

	auto SetupWall = [](UBoxComponent* B)
		{
			B->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			B->SetCollisionResponseToAllChannels(ECR_Block);
			B->SetGenerateOverlapEvents(false);
		};
	SetupWall(LeftWall);
	SetupWall(RightWall);
}

void ALaneLevelGenerator::BeginPlay()
{
	Super::BeginPlay();

	RecomputePlayfield();
	UpdateSeamPosY();

	// Cache wall sprite size (your side walls are pitched -> height in Z)
	WallTileWUU = WallTileHUU = 0.f;
	if (WallVariants.Num() > 0 && IsValid(WallVariants[0]))
	{
		const FBoxSphereBounds B = WallVariants[0]->GetRenderBounds();
		WallTileWUU = B.BoxExtent.X * 2.f; // width across X
		WallTileHUU = B.BoxExtent.Z * 2.f; // height along Z
	}

	if (bShowWalls) InitWallsInfinite();

	ScaffoldLane = FMath::Clamp(NumLanes / 2, 0, NumLanes - 1);

	WarmEnemyPools();
	GetWorldTimerManager().SetTimer(PoolTimer, this, &ALaneLevelGenerator::TryEnemiesPool, RecycleInterval, true);
}


void ALaneLevelGenerator::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RecomputePlayfield();
	UpdateSeamPosY();
	//RebuildWalls();
}

void ALaneLevelGenerator::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bSeamsFollowPlayerY) { UpdateSeamPosY(); }
	if (bDrawDebugSeams) { DrawSeamDebug(); }

	if (bSpawnPlatforms)
	{
		if (PlatformSpawnStartTime == 0.f)  // Only set this once when spawn starts
		{
			PlatformSpawnStartTime = GetWorld()->GetTimeSeconds();
		}

		// Check if the delay has passed
		if (GetWorld()->GetTimeSeconds() - PlatformSpawnStartTime >= PlatformSpawnDelay)
		{
			if (bFirstPlatformSpawn)
			{
				// Spawn first platform below the player
				if (PlayerRef)
				{
					// Adjust spawn position to be below the player
					CursorLocalY = PlayerLocalY() + PlatformSpawnOffsetY;
					bFirstPlatformSpawn = false; // Stop doing this after the first platform
				}
			}

			EnsureSegmentsAhead();   // Spawn platforms after the delay
			CullOldRows();           // Clean up old platforms
		}
	}
	if (bDrawScaffoldDebug) { DrawScaffoldDebug(); }

	// >>> Do the Z-loop first so any per-frame wall spawn/cull uses the fresh positions
	if (bUseZLoop) { ZLoopIfNeeded(); }

	// >>> On the same frame we looped, skip the wall updater to avoid double-spawn
	if (bShowWalls && !bJustDidZLoop)
	{
		UpdateWallsInfinite();
	}

	// clear one-frame guard at end of tick
	bJustDidZLoop = false;
}

void ALaneLevelGenerator::RecomputePlayfield()
{
	// If a reference sprite is set, you can derive lane width from its frame size.
	if (ReferenceTileSprite)
	{
		// Bounds are already in UU (no PPUU divide needed).
		const FBoxSphereBounds B = ReferenceTileSprite->GetRenderBounds();
		const float TileWUU = B.BoxExtent.X * 2.f;

		// OPTIONAL: if you want lane width driven by tiles, uncomment and plug your count:
		// LaneWidthUU = FMath::Max(1, TilesPerLane) * TileWUU;
	}

	PlayfieldWidthUU = FMath::Max(NumLanes, 1) * FMath::Max(LaneWidthUU, 1.f);
	Xmin = -0.5f * PlayfieldWidthUU;
	Xmax = +0.5f * PlayfieldWidthUU;

	SeamHalfHeightUU = SeamHalfHeightsInScreens * ScreenWorldHeightUU;

	// Local positions, not world:
	LeftSeam->SetRelativeLocation(FVector(Xmin, 0.f, 0.f));
	RightSeam->SetRelativeLocation(FVector(Xmax, 0.f, 0.f));

	// Box extents are in local space; X thin, Y tall, Z small
	const FVector Extents(SeamHalfThicknessX, SeamHalfHeightUU, 100.f);
	LeftSeam->SetBoxExtent(Extents);
	RightSeam->SetBoxExtent(Extents);
}

void ALaneLevelGenerator::UpdateSeamPosY()
{
	float LocalY = 0.f;
	if (bSeamsFollowPlayerY && PlayerRef)
	{
		const FTransform T = GetActorTransform();
		const FVector PlayerLocal = T.InverseTransformPosition(PlayerRef->GetActorLocation());
		LocalY = PlayerLocal.Y; // follow along the actor’s local +Y (ForwardVector)
	}
	// keep X at +/-W/2, update local Y
	LeftSeam->SetRelativeLocation(FVector(Xmin, LocalY, 0.f));
	RightSeam->SetRelativeLocation(FVector(Xmax, LocalY, 0.f));
}

bool ALaneLevelGenerator::IsEligible(AActor* A) const
{
	if (!A) return false;

	// Always allow the explicit player ref
	if (A == PlayerRef) return true;

	// Tag-based eligibility
	if (EligibleTag != NAME_None && A->ActorHasTag(EligibleTag))
		return true;

	// Optionally: any Pawn
	if (bWrapPawnsByDefault && A->IsA<APawn>())
		return true;

	return false;
}

void ALaneLevelGenerator::TryWrap(AActor* A, bool bFromLeftSeam)
{
	if (!A) return;

	const float Now = GetWorld()->GetTimeSeconds();
	if (float* Until = CooldownUntil.Find(A))
		if (Now < *Until) return;

	const float W = PlayfieldWidthUU;
	const FVector Right = GetActorRightVector(); // local +X
	const FVector Inset = Right * InsetUU;
	const FVector Shift = Right * W;

	FVector L = A->GetActorLocation();
	if (bFromLeftSeam)
		L = L + Shift - Inset;   // to right side, nudge inward
	else
		L = L - Shift + Inset;   // to left side, nudge inward

	FHitResult Hit;
	A->SetActorLocation(L, /*bSweep=*/true, &Hit, ETeleportType::TeleportPhysics);

	CooldownUntil.FindOrAdd(A) = Now + WrapCooldownSeconds;
}

void ALaneLevelGenerator::WarmEnemyPools()
{
	UWorld* W = GetWorld(); if (!W) return;

	auto SpawnIntoPool = [&](TArray<TObjectPtr<ACPP_EnemyParent>>& Pool,
		TSubclassOf<ACPP_EnemyParent> Cls, int32 Count)
		{
			if (!*Cls || Count <= 0) return;
			Pool.Reserve(Pool.Num() + Count);
			for (int32 i = 0; i < Count; ++i)
			{
				FActorSpawnParameters sp;
				sp.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				sp.Owner = this;

				ACPP_EnemyParent* E = W->SpawnActor<ACPP_EnemyParent>(Cls, FVector(0.f, -100000.f, 0.f), FRotator::ZeroRotator, sp);
				if (E)
				{
					E->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
					E->DeactivateToPool();
					Pool.Add(E);
				}
			}
		};

	// Seed Walker pool with a MIX of regular + ALT so BorrowFromPool can return either.
	const int32 AltCount = FMath::Max(0, PoolSize_Walker / 2);               // half (floor)
	const int32 BaseCount = FMath::Max(0, PoolSize_Walker - AltCount);        // rest
	SpawnIntoPool(WalkerPool, WalkerEnemyClass, BaseCount);
	SpawnIntoPool(WalkerPool, WalkerAltEnemyClass, AltCount);

	// Flyers unchanged
	SpawnIntoPool(FlyerPool, FlyerEnemyClass, PoolSize_Flyer);
}

ACPP_EnemyParent* ALaneLevelGenerator::BorrowFromPool(TArray<TObjectPtr<ACPP_EnemyParent>>& Pool, TSubclassOf<ACPP_EnemyParent> Cls, int32 PoolCap)
{
	// Choose which round-robin cursor to use based on which pool we’re scanning.
	int32& rr = (&Pool == &WalkerPool) ? NextWalkerIdx : NextFlyerIdx;

	const int32 N = Pool.Num();
	const float Now = GetWorld() ? GetWorld()->TimeSeconds : 0.f;

	// First pass: pick only strictly POOLED enemies that also passed MinReuseDelay
	for (int32 t = 0; t < N; ++t)
	{
		const int32 idx = (rr + t) % N;
		ACPP_EnemyParent* E = Pool[idx];
		if (!E) continue;

		// Only reuse if truly pooled (never pick Dying/Active)
		if (E->GetLifeState() != EEnemyLifeState::Pooled) continue;

		// Optional: honor per-enemy reuse cooldown (from our earlier patch)
		// If you didn’t add these fields, you can remove the two lines below.
		const float sincePool = Now - E->PooledAtTime;
		if (sincePool < E->MinReuseDelay) continue;

		// Fairness: advance the cursor past this pick
		rr = (idx + 1) % FMath::Max(N, 1);
		return E;
	}

	// If nothing reusable yet and we’re allowed to grow, spawn one more
	if (*Cls && N < PoolCap)
	{
		if (UWorld* W = GetWorld())
		{
			FActorSpawnParameters sp;
			sp.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			sp.Owner = this;

			if (ACPP_EnemyParent* E = W->SpawnActor<ACPP_EnemyParent>(Cls, FVector(0.f, -100000.f, 0.f), FRotator::ZeroRotator, sp))
			{
				E->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
				E->DeactivateToPool(); // puts it in Pooled state immediately
				Pool.Add(E);

				// Put cursor after the new slot for fairness on next call
				rr = Pool.Num() % Pool.Num();
				return E;
			}
		}
	}

	// None eligible this moment; let the caller skip this frame or try next tick.
	return nullptr;
}

void ALaneLevelGenerator::TryEnemiesPool()
{
	// explicit element types per your compile requirement
	for (ACPP_EnemyParent* E : WalkerPool) { RecycleIfOutOfWindow(E, 0.f); }
	for (ACPP_EnemyParent* E : FlyerPool) { RecycleIfOutOfWindow(E, 0.f); }
}

void ALaneLevelGenerator::RecycleIfOutOfWindow(ACPP_EnemyParent* E, float PlayerY)
{
	if (!E || !E->IsActive() || !PlayerRef) return;

	const FTransform GenXform = GetActorTransform();

	// project both to generator-local space to avoid world-axis sign issues
	const float playerLocalY = GenXform.InverseTransformPosition(PlayerRef->GetActorLocation()).Y;
	const float enemyLocalY = GenXform.InverseTransformPosition(E->GetActorLocation()).Y;

	// "above" the player = smaller localY (we descend as localY increases)
	const float screenH = (ScreenWorldHeightUU > 1.f) ? ScreenWorldHeightUU : 2000.f; // fallback if unset
	const float despawnThresholdLocalY = playerLocalY - 1.5f * screenH;

	// small padding to prevent jittery instant-despawn when right at the boundary
	const float pad = 0.05f * screenH;

	if (enemyLocalY < despawnThresholdLocalY - pad)
	{
		E->DeactivateToPool();
	}
}

void ALaneLevelGenerator::ZLoopIfNeeded()
{
	if (!bEnableZLoop || !PlayerRef) return;

	const FVector GenLoc = GetActorLocation();
	const FVector PlayerLoc = PlayerRef->GetActorLocation();

	const float triggerZ = GenLoc.Z - LoopThresholdDownUU; // we fall “down” (lower Z)
	if (PlayerLoc.Z < triggerZ)
	{
		// how far past the trigger are we?
		const float overshoot = triggerZ - PlayerLoc.Z;
		const int32 k = FMath::FloorToInt(overshoot / LoopSpanUU) + 1;  // at least one chunk
		const float deltaZ = k * LoopSpanUU; // positive: lift upwards in Z

		ShiftAllByWorldZ(deltaZ);
	}
}

void ALaneLevelGenerator::ShiftAllByWorldZ(float DeltaZ)
{
	if (FMath::IsNearlyZero(DeltaZ)) return;

	const FVector worldOffset(0.f, 0.f, DeltaZ);
	const float   deltaLocalY = GetActorTransform().InverseTransformVector(worldOffset).Y;

	// --- carry ALL pooled enemies (cheap, no misses)
	for (ACPP_EnemyParent* E : WalkerPool)
		if (E) E->AddActorWorldOffset(worldOffset, false, nullptr, ETeleportType::TeleportPhysics);
	for (ACPP_EnemyParent* E : FlyerPool)
		if (E) E->AddActorWorldOffset(worldOffset, false, nullptr, ETeleportType::TeleportPhysics);

	// 1) player
	if (PlayerRef)
	{
		PlayerRef->AddActorWorldOffset(worldOffset, false, nullptr, ETeleportType::TeleportPhysics);
		if (UWorld* W = GetWorld())
		{
			const float until = W->TimeSeconds + LoopContactGrace;
			// PlayerRef->SetContactGraceUntil(until); // if you implemented it
		}
	}

	// 1b) camera (spawned)
	if (CameraActor)
	{
		if (auto* DWCam = Cast<AACPP_DownwellCamera>(CameraActor))
		{
			DWCam->NotifyWorldZLoop(DeltaZ);
		}
		else
		{
			CameraActor->AddActorWorldOffset(FVector(0, 0, DeltaZ), false, nullptr, ETeleportType::TeleportPhysics);
		}
	}

	// 3) live strips + cached localY
	for (FRowBit& Row : LiveRows)
	{
		for (TWeakObjectPtr<APlatformStrip>& W : Row.Actors)
			if (APlatformStrip* A = W.Get())
				A->AddActorWorldOffset(worldOffset, false, nullptr, ETeleportType::TeleportPhysics);

		Row.LocalY += deltaLocalY;
	}

	// 4) cursors
	CursorLocalY += deltaLocalY;
	NextWalkerLocalY += deltaLocalY;
	NextFlyerLocalY += deltaLocalY;
	NextWalkerY += deltaLocalY;
	NextFlyerY += deltaLocalY;

	// 5) sticky followers
	for (AActor* A : StickyFollowers)
		if (A) A->AddActorWorldOffset(worldOffset, false, nullptr, ETeleportType::TeleportPhysics);

	// 6) wall cursors
	WallTopY += deltaLocalY;
	WallNextSpawnY += deltaLocalY;

	// === Walls: only top-fill now (instant visual cover), bottom appends next Tick ===
	FillWallsAfterLoop();
	bJustDidZLoop = true;

	UE_LOG(LogTemp, Log, TEXT("[ZLoop] +Z=%.0f  deltaLocalY=%.0f  Cursor=%.0f"),
		DeltaZ, deltaLocalY, CursorLocalY);
}

void ALaneLevelGenerator::FillWallsAfterLoop()
{
	if (WallTileHUU <= 0.f) return;

	const float playerY = (PlayerRef)
		? GetActorTransform().InverseTransformPosition(PlayerRef->GetActorLocation()).Y
		: 0.f;

	// Ensure coverage above the visible band (what you see right after a loop “teleport up”)
	const float wantTopY = playerY
		- WallsHalfScreensVisible * ScreenWorldHeightUU
		- WallVerticalTilesPadding * WallTileHUU;

	const float leftX = Xmin + WallInsetX;
	const float rightX = Xmax - WallInsetX;

	auto PrependPairAt = [&](float y)
		{
			// Left
			UPaperSprite* L = nullptr;
			for (int t = 0; t < 8 && !L; ++t)
			{
				L = WallVariants[FMath::RandHelper(WallVariants.Num())];
				if (!IsValid(L)) L = nullptr;
			}
			if (!L && WallVariants.Num() > 0) L = WallVariants[0];

			auto* SL = NewObject<UPaperSpriteComponent>(this);
			SL->RegisterComponent();
			SL->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);
			SL->SetSprite(L);
			SL->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			SL->SetRelativeRotation(FRotator(0.f, 0.f, WallSpriteRollDeg));
			SL->SetRelativeLocation(FVector(leftX, y, 0.f));
			WallSpritesLeft.Insert(SL, 0);

			// Right
			UPaperSprite* R = nullptr;
			for (int t = 0; t < 8 && !R; ++t)
			{
				R = WallVariants[FMath::RandHelper(WallVariants.Num())];
				if (!IsValid(R)) R = nullptr;
			}
			if (!R && WallVariants.Num() > 0) R = WallVariants[0];

			auto* SR = NewObject<UPaperSpriteComponent>(this);
			SR->RegisterComponent();
			SR->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);
			SR->SetSprite(R);
			SR->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			SR->SetRelativeRotation(FRotator(0.f, 0.f, WallSpriteRollDeg));
			if (bMirrorRightColumn) SR->SetRelativeScale3D(FVector(-1.f, 1.f, 1.f));
			SR->SetRelativeLocation(FVector(rightX, y, 0.f));
			WallSpritesRight.Insert(SR, 0);

			WallTopY -= WallTileHUU;
		};

	// Start one tile above the current top (if any); otherwise seed from the target top.
	float nextY = (WallSpritesLeft.Num() > 0 && WallSpritesLeft[0])
		? (WallSpritesLeft[0]->GetRelativeLocation().Y - WallTileHUU)
		: wantTopY;

	int guard = 0;
	while (nextY > wantTopY - 0.5f * WallTileHUU && guard++ < 512)
	{
		PrependPairAt(nextY);
		nextY -= WallTileHUU;
	}

}

void ALaneLevelGenerator::RecenterWorldZ(bool bZeroPlayerVelocity)
{
	if (!PlayerRef) return;

	const float DeltaZ = -PlayerRef->GetActorLocation().Z;
	if (FMath::IsNearlyZero(DeltaZ, 1.f))
		return;

	if (bZeroPlayerVelocity)
	{
		if (UMovementComponent* MoveComp = PlayerRef->FindComponentByClass<UMovementComponent>())
		{
			MoveComp->Velocity = FVector::ZeroVector;
		}
	}

	// Shift player/cam/enemies/rows and update cursors
	ShiftAllByWorldZ(DeltaZ);

	// NEW: immediately reseed the visible wall band near the player
	if (bShowWalls)
	{
		DestroyAllWalls(/*bDisableBlockers*/false);
		InitWallsInfinite();
	}
}

void ALaneLevelGenerator::OnLeftSeamBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (IsEligible(OtherActor))
	{
		TryWrap(OtherActor, /*bFromLeftSeam=*/true);
	}
}

void ALaneLevelGenerator::OnRightSeamBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (IsEligible(OtherActor))
	{
		TryWrap(OtherActor, /*bFromLeftSeam=*/false);
	}
}

void ALaneLevelGenerator::DrawSeamDebug() const
{
	if (!GetWorld()) return;

	// --- existing seam lines ---
	const FVector Lc = LeftSeam->GetComponentLocation();
	const FVector Rc = RightSeam->GetComponentLocation();
	const FVector UpLocalY = GetActorForwardVector();  // local +Y
	const float   HalfH = SeamHalfHeightUU;
	const FVector Dy = UpLocalY * HalfH;

	// seams (cyan)
	DrawDebugLine(GetWorld(), Lc - Dy, Lc + Dy, FColor::Cyan, false, 0.f, 0, 1.5f);
	DrawDebugLine(GetWorld(), Rc - Dy, Rc + Dy, FColor::Cyan, false, 0.f, 0, 1.5f);

	// --- lane center guides ---
	if (bDrawLaneGuides)
	{
		const FTransform T = GetActorTransform();
		const float BandCenterLocalY = T.InverseTransformPosition(Lc).Y; // left/right share same Y

		for (int32 Lane = 0; Lane < NumLanes; ++Lane)
		{
			const float Xl = LaneCenterX_Local(Lane, NumLanes, LaneWidthUU); // <-- updated
			const FVector A_local(Xl, BandCenterLocalY - HalfH, 0);
			const FVector B_local(Xl, BandCenterLocalY + HalfH, 0);

			const FVector A = T.TransformPosition(A_local);
			const FVector B = T.TransformPosition(B_local);

			DrawDebugLine(GetWorld(), A, B, FColor::Red, false, 0.f, 0, 0.8f);
		}
	}
}

void ALaneLevelGenerator::EnsureSegmentsAhead()
{
	if (ScreenWorldHeightUU <= KINDA_SMALL_NUMBER) return;

	const float targetLocalY = PlayerLocalY() + SpawnLeadScreens * ScreenWorldHeightUU;

	const int32 cap = FMath::Clamp(Gen_MaxRowsPerTick, 0, 32);
	int32 built = 0;

	while (CursorLocalY + 0.1f < targetLocalY && built < cap)
	{
		GenerateScaffoldSegment();
		++built;
		if (bGen_DryRun_NoSpawn)
		{
			// If we’re dry-running, CursorLocalY still advances inside GenerateScaffoldSegment.
			// Nothing else to do here.
		}
	}
}

void ALaneLevelGenerator::CullOldRows()
{
	const float CullY = PlayerLocalY() - CullBufferScreens * ScreenWorldHeightUU; // local +Y is your row axis
	for (int32 i = LiveRows.Num() - 1; i >= 0; --i)
	{
		if (LiveRows[i].LocalY < CullY)
		{
			DespawnRow(LiveRows[i]);
			LiveRows.RemoveAt(i);
		}
	}
}

void ALaneLevelGenerator::GenerateScaffoldSegment()
{
	// Occasionally turn the scaffold lane a step left/right (keeps the “path” alive)
	if (FMath::FRand() < TurnChance())
	{
		const int dir = (FMath::FRand() < 0.5f) ? -1 : +1;
		ScaffoldLane = (ScaffoldLane + dir + NumLanes) % NumLanes;
	}

	// Previous row mask (for reachability + anti-stacking + "no back-to-back empty")
	const uint16 PrevMask = (LiveRows.Num() > 0) ? LiveRows.Last().ScaffoldBits : 0;

	// --- Row skipping (creates vertical gaps) ---
	// Reuse the platforms density ramp: when extras are rare, allow more empty rows.
	// We also avoid two empty rows in a row (PrevMask==0 guard).
	const float pExtra = ExtraPlatformChance();                 // 0..1 from your existing ramp
	const float rowSkipChance = FMath::Clamp(0.35f - 0.5f * pExtra, 0.f, 0.35f);
	const bool  bCanSkip = (PrevMask != 0);                     // don't create two blanks in a row
	const bool  bSkipThisRow = bCanSkip && (FMath::FRand() < rowSkipChance);

	// Row record (we always advance CursorLocalY)
	FRowBit Row;
	Row.RowIndex = NextRowIndex++;
	Row.LocalY = CursorLocalY;

	uint16 FinalMask = 0;

	if (!bSkipThisRow)
	{
		// Build: scaffold + extras, with anti-stack bias vs previous row
		const uint16 Mask0 = BuildRowMask_WithExtras(ScaffoldLane, PrevMask);

		// Validate against previous row for reachability; reroll a few times if needed
		FinalMask = Mask0;
		int Reroll = 0;
		while (!ValidateRowMask(PrevMask, FinalMask) && Reroll < RowRerollAttempts)
		{
			FinalMask = BuildRowMask_WithExtras(ScaffoldLane, PrevMask);
			++Reroll;
		}

		if (!bGen_SkipRuns && !bGen_DryRun_NoSpawn)
		{
			// Runs
			TArray<FRowRun> Runs;
			BuildRunsFromMask(FinalMask, ScaffoldLane, Runs);

			// Hazards behind a flag
			if (!bGen_SkipHazards)
			{
				ApplyHazardsToRuns(PrevMask, FinalMask, Runs);
			}

			// Spawn rows of platforms at this LocalY
			SpawnRuns(Runs, Row.LocalY, Row.Actors);
		}
		else if (!bGen_DryRun_NoSpawn && bGen_SkipRuns)
		{
			GenerateScaffoldSegment_Simple();
			return;
		}
	}

	// Bookkeeping + advance
	Row.ScaffoldBits = FinalMask;   // 0 if the row was skipped
	LiveRows.Add(Row);
	CursorLocalY += RowHeightUU;
}

void ALaneLevelGenerator::DrawScaffoldDebug()
{
	if (!GetWorld()) return;

	const float boxW = LaneWidthUU * 0.80f;
	const float boxH = RowHeightUU * 0.25f;
	const FVector extents(boxW * 0.5f, boxH * 0.5f, 8.f);

	for (const FRowBit& Row : LiveRows)
	{
		TArray<FRowRun> Runs;
		BuildRunsFromMask(Row.ScaffoldBits, ScaffoldLane, Runs);

		for (const FRowRun& R : Runs)
		{
			const float leftCenterX = LaneCenterX_Local(R.StartLane, NumLanes, LaneWidthUU);
			const float centerX = leftCenterX + 0.5f * float(R.LenLanes - 1) * LaneWidthUU;

			const FVector localCenter(centerX, Row.LocalY, 0.f);
			const FVector worldCenter = GetActorTransform().TransformPosition(localCenter);

			DrawDebugBox(GetWorld(), worldCenter, extents, FQuat::Identity,
				R.bScaffold ? FColor::Cyan : FColor::Silver,
				false, 0.f, 0, 0.9f);
		}
	}
}

void ALaneLevelGenerator::GenerateScaffoldSegment_Simple()
{
	if (FMath::FRand() < TurnChance())
	{
		const int dir = (FMath::FRand() < 0.5f) ? -1 : +1;
		ScaffoldLane = (ScaffoldLane + dir + NumLanes) % NumLanes;
	}

	FRowBit row;
	row.ScaffoldBits = (uint16(1) << ScaffoldLane);
	row.RowIndex = NextRowIndex++;
	row.LocalY = CursorLocalY;

	if (ScaffoldPlatformClass && !bGen_DryRun_NoSpawn)
	{
		const int32 tiles = FMath::Max(TilesPerLane, 1);

		const float centerX = LaneCenterX_Local(ScaffoldLane, NumLanes, LaneWidthUU);
		FVector worldPos = GetActorTransform().TransformPosition(FVector(centerX, row.LocalY, 0.f));
		if (bLockPlatformsToPlayerY && PlayerRef) worldPos.Y = PlayerRef->GetActorLocation().Y;
		else                                       worldPos.Y = PlatformsY;

		const FRotator worldRot = FRotator::ZeroRotator;

		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		P.Owner = this;

		if (APlatformStrip* Plat = GetWorld()->SpawnActor<APlatformStrip>(ScaffoldPlatformClass, worldPos, worldRot, P))
		{
			Plat->SetVisualKind(EPlatformKind::Solid);
			Plat->SetSpriteRollDegrees(PlatformSpriteRollDeg);

			// base thickness (+Y) before pads/boost
			Plat->CollisionHeightUU_Override = (PlatformCollisionHeightUU > 0.f) ? PlatformCollisionHeightUU : -1.f;

			// show real collider if desired
			Plat->SetCollisionVisible(bRevealPlatformCollision);

			// pads/boost from generator (extend L/R, thicken F/B, lift top only)
			Plat->SetCollisionPads(PlatformCollisionPadXUU, PlatformCollisionPadYUU, PlatformCollisionTopBoostUU);

			// build the collider-only strip (fallback path); tiling path also calls ApplyCollisionSizing
			Plat->BuildDebugFallback(tiles);
			row.Actors.Add(Plat);
		}
	}

	LiveRows.Add(row);
	CursorLocalY += RowHeightUU;
}

void ALaneLevelGenerator::InitWallsInfinite()
{
	// wipe old
	for (auto* C : WallSpritesLeft)  if (C) C->DestroyComponent();
	for (auto* C : WallSpritesRight) if (C) C->DestroyComponent();
	WallSpritesLeft.Empty();
	WallSpritesRight.Empty();

	if (WallTileHUU <= 0.f || WallVariants.Num() == 0) return;

	// pick a valid ref
	UPaperSprite* Ref = nullptr;
	for (auto* S : WallVariants) if (IsValid(S)) { Ref = S; break; }
	if (!Ref) return;

	const float bandHalfUU = WallsHalfScreensVisible * ScreenWorldHeightUU;

	// center the initial band around current player Y (or 0 if none)
	const float centerLocalY = (PlayerRef)
		? GetActorTransform().InverseTransformPosition(PlayerRef->GetActorLocation()).Y
		: 0.f;

	// initial Y coverage
	const float startY = centerLocalY - bandHalfUU - WallVerticalTilesPadding * WallTileHUU;
	const float endY = centerLocalY + bandHalfUU + WallVerticalTilesPadding * WallTileHUU;

	// helper spawner for one tile at (X,Y)
	auto SpawnTile = [&](TArray<UPaperSpriteComponent*>& Out, float XLocal, float YLocal, bool bRight)
		{
			// pick a variant (skip nulls)
			UPaperSprite* Pick = nullptr;
			for (int t = 0; t < 8 && !Pick; ++t)
			{
				Pick = WallVariants[FMath::RandHelper(WallVariants.Num())];
				if (!IsValid(Pick)) Pick = nullptr;
			}
			if (!Pick) Pick = Ref;

			auto* S = NewObject<UPaperSpriteComponent>(this);
			S->RegisterComponent();
			S->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);
			S->SetSprite(Pick);
			S->SetCollisionEnabled(ECollisionEnabled::NoCollision);

			// face camera (+Z); roll matches your art orientation
			S->SetRelativeRotation(FRotator(0.f, 0.f, WallSpriteRollDeg));
			if (bRight && bMirrorRightColumn) S->SetRelativeScale3D(FVector(-1.f, 1.f, 1.f));

			S->SetRelativeLocation(FVector(XLocal, YLocal, 0.f));
			Out.Add(S);
		};

	const float leftX = Xmin + WallInsetX;
	const float rightX = Xmax - WallInsetX;

	// fill the initial band
	float y = startY;
	while (y <= endY + 0.5f * WallTileHUU)
	{
		SpawnTile(WallSpritesLeft, leftX, y, /*bRight=*/false);
		SpawnTile(WallSpritesRight, rightX, y, /*bRight=*/true);
		y += WallTileHUU;
	}

	// place/size blockers on art face
	const float halfW = 0.5f * WallTileWUU;
	const float faceLeftX = leftX + halfW - WallFaceContactInsetUU;
	const float faceRightX = rightX - halfW + WallFaceContactInsetUU;

	const FVector halfExtents(WallBlockThicknessUU * 0.5f, bandHalfUU, 2000.f);
	if (LeftWall) { LeftWall->SetBoxExtent(halfExtents);  LeftWall->SetRelativeLocation(FVector(faceLeftX, centerLocalY, 0.f)); }
	if (RightWall) { RightWall->SetBoxExtent(halfExtents); RightWall->SetRelativeLocation(FVector(faceRightX, centerLocalY, 0.f)); }

	// track range
	WallTopY = startY;
	WallNextSpawnY = y;         // first Y below the seeded band
}

void ALaneLevelGenerator::UpdateWallsInfinite()
{
	if (WallTileHUU <= 0.f) return;

	// Player Y in generator-local space decides when to add/cull; we do NOT move existing tiles.
	const float playerY = (PlayerRef)
		? GetActorTransform().InverseTransformPosition(PlayerRef->GetActorLocation()).Y
		: 0.f;

	const float leftX = Xmin + WallInsetX;
	const float rightX = Xmax - WallInsetX;

	// --- Keep WallNextSpawnY in sync with the actual bottom-most tile to avoid double spawns.
	auto LastY = [](const TArray<UPaperSpriteComponent*>& Arr) -> float
		{
			for (int32 i = Arr.Num() - 1; i >= 0; --i)
				if (Arr[i]) return Arr[i]->GetRelativeLocation().Y;
			return -FLT_MAX;
		};
	{
		const float lastY = FMath::Max(LastY(WallSpritesLeft), LastY(WallSpritesRight));
		if (lastY > -FLT_MAX / 2.f)
		{
			WallNextSpawnY = FMath::Max(WallNextSpawnY, lastY + WallTileHUU);
		}
	}

	// ---- Append below until we’re WallSpawnLeadScreens ahead ----
	const float wantBottomY = playerY + WallSpawnLeadScreens * ScreenWorldHeightUU;

	auto AppendPairAt = [&](float y)
		{
			// Guard: if we somehow got the exact same Y, skip to prevent “double walls”.
			if (WallSpritesLeft.Num() > 0 && WallSpritesLeft.Last() &&
				FMath::IsNearlyEqual(WallSpritesLeft.Last()->GetRelativeLocation().Y, y, 0.01f))  return;
			if (WallSpritesRight.Num() > 0 && WallSpritesRight.Last() &&
				FMath::IsNearlyEqual(WallSpritesRight.Last()->GetRelativeLocation().Y, y, 0.01f))  return;

			// Left
			{
				UPaperSprite* Pick = nullptr;
				for (int t = 0; t < 8 && !Pick; ++t)
				{
					Pick = WallVariants[FMath::RandHelper(WallVariants.Num())];
					if (!IsValid(Pick)) Pick = nullptr;
				}
				if (!Pick && WallVariants.Num() > 0) Pick = WallVariants[0];

				auto* S = NewObject<UPaperSpriteComponent>(this);
				S->RegisterComponent();
				S->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);
				S->SetSprite(Pick);
				S->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				S->SetRelativeRotation(FRotator(0.f, 0.f, WallSpriteRollDeg));
				S->SetRelativeLocation(FVector(leftX, y, 0.f));
				WallSpritesLeft.Add(S);
			}
			// Right
			{
				UPaperSprite* Pick = nullptr;
				for (int t = 0; t < 8 && !Pick; ++t)
				{
					Pick = WallVariants[FMath::RandHelper(WallVariants.Num())];
					if (!IsValid(Pick)) Pick = nullptr;
				}
				if (!Pick && WallVariants.Num() > 0) Pick = WallVariants[0];

				auto* S = NewObject<UPaperSpriteComponent>(this);
				S->RegisterComponent();
				S->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);
				S->SetSprite(Pick);
				S->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				S->SetRelativeRotation(FRotator(0.f, 0.f, WallSpriteRollDeg));
				if (bMirrorRightColumn) S->SetRelativeScale3D(FVector(-1.f, 1.f, 1.f));
				S->SetRelativeLocation(FVector(rightX, y, 0.f));
				WallSpritesRight.Add(S);
			}
		};

	int guard = 0;
	while (WallNextSpawnY < wantBottomY && guard++ < 512)
	{
		AppendPairAt(WallNextSpawnY);
		WallNextSpawnY += WallTileHUU;
	}

	// ---- Cull far above to save perf/mem ----
	const float cullAboveY = playerY - WallCullAboveScreens * ScreenWorldHeightUU;

	while (WallSpritesLeft.Num() > 0 &&
		WallSpritesLeft[0] &&
		(WallSpritesLeft[0]->GetRelativeLocation().Y + WallTileHUU) < cullAboveY)
	{
		if (WallSpritesLeft[0])  WallSpritesLeft[0]->DestroyComponent();
		if (WallSpritesRight[0]) WallSpritesRight[0]->DestroyComponent();
		WallSpritesLeft.RemoveAt(0);
		WallSpritesRight.RemoveAt(0);
		WallTopY += WallTileHUU;
	}

	// ---- Keep blocker boxes centered on the visible band (so they always block) ----
	const float halfW = 0.5f * WallTileWUU;
	const float faceLeftX = leftX + halfW - WallFaceContactInsetUU;
	const float faceRightX = rightX - halfW + WallFaceContactInsetUU;

	const float bandHalfUU = WallsHalfScreensVisible * ScreenWorldHeightUU;
	const float bandCenterY = playerY;

	if (LeftWall) { LeftWall->SetRelativeLocation(FVector(faceLeftX, bandCenterY, 0.f));  LeftWall->SetBoxExtent(FVector(WallBlockThicknessUU * 0.5f, bandHalfUU, 2000.f)); }
	if (RightWall) { RightWall->SetRelativeLocation(FVector(faceRightX, bandCenterY, 0.f)); RightWall->SetBoxExtent(FVector(WallBlockThicknessUU * 0.5f, bandHalfUU, 2000.f)); }
}

FVector ALaneLevelGenerator::RowCenterWorld(int32 Lane, float LocalY) const
{
	const float Xl = LaneCenterX_Local(Lane, NumLanes, LaneWidthUU); // <-- updated
	const FVector Local(Xl, LocalY, 0.f);
	return GetActorTransform().TransformPosition(Local);
}

uint16 ALaneLevelGenerator::BuildRowMask_WithExtras(int32 ScaffoldLaneIdx, uint16 PrevMask) const
{
	const int32 L = FMath::Max(NumLanes, 1);
	const uint16 ONE = 1;

	auto wrap = [&](int i) { return (i % L + L) % L; };

	// Track taken lanes; we’ll “paint” runs into this
	TArray<bool> taken; taken.Init(false, L);

	auto take_run = [&](int start, int len)
		{
			for (int k = 0; k < len; ++k) taken[wrap(start + k)] = true;
		};

	auto can_place_len = [&](int start, int wantLen)->int
		{
			int can = 0;
			for (int k = 0; k < wantLen; ++k)
			{
				if (taken[wrap(start + k)]) break;
				++can;
			}
			return can;
		};

	// ---- Always include scaffold lane as a 1-lane run ----
	take_run(wrap(ScaffoldLaneIdx), 1);

	// Base probability from your ramp (same as before)
	const float pBase = ExtraPlatformChance();  // 0.35 .. 0.55 by depth
	const int32 minL = FMath::Clamp(MinRunLanes, 1, L);
	const int32 maxL = FMath::Clamp(MaxRunLanes, minL, L);

	int extras = 0;
	int safety = 0;

	while (extras < MaxExtrasPerRow && safety++ < 32)
	{
		// Pick a candidate start on a free lane
		int start = FMath::RandRange(0, L - 1);
		bool found = false;
		for (int t = 0; t < L; ++t)
		{
			const int s = wrap(start + t);
			if (!taken[s]) { start = s; found = true; break; }
		}
		if (!found) break; // no room left

		// Anti-stack bias if previous row used this lane
		const bool prevHere = ((PrevMask & (ONE << start)) != 0);
		const float p = prevHere ? (pBase * 0.35f) : pBase;
		if (FMath::FRand() >= p) continue;

		// Choose a run length in [minL..maxL], with a slight nudge toward longer runs
		int wantLen = FMath::RandRange(minL, maxL);
		if (FMath::FRand() < 0.35f) wantLen = maxL;

		const int canLen = can_place_len(start, wantLen);
		if (canLen <= 0) continue;

		take_run(start, canLen);
		UE_LOG(LogTemp, Verbose, TEXT("[RowMask] placed run start=%d len=%d (want=%d)  min=%d max=%d"),
			start, canLen, wantLen, minL, maxL);
		++extras;
	}

	// Emit bitmask
	uint16 mask = 0;
	for (int i = 0; i < L; ++i) if (taken[i]) mask |= (ONE << i);

	UE_LOG(LogTemp, Verbose, TEXT("[RowMask] result=0x%04x  lanes=%d  MinRunLanes=%d  MaxRunLanes=%d  Extras=%d"),
		mask, L, minL, maxL, extras);
	return mask;
}

bool ALaneLevelGenerator::ValidateRowMask(uint16 PrevMask, uint16 ThisMask) const
{
	if (PrevMask == 0) return true;

	auto BitSet = [](uint16 m, int i) { return (m & (uint16(1) << i)) != 0; };

	for (int lane = 0; lane < NumLanes; ++lane)
	{
		if (!BitSet(ThisMask, lane)) continue;

		bool ok = false;
		for (int d = -MaxGapLanes; d <= MaxGapLanes && !ok; ++d)
		{
			const int prev = (lane + d + NumLanes) % NumLanes; // wrap ok
			if (BitSet(PrevMask, prev)) ok = true;
		}
		if (!ok) return false;
	}
	return true;
}

void ALaneLevelGenerator::BuildRunsFromMask(uint16 Mask, int32 InScaffoldLane, TArray<FRowRun>& OutRuns) const
{
	OutRuns.Reset();
	const int32 Lanes = FMath::Max(NumLanes, 1);
	auto Bit = [&](int i) { return (Mask & (uint16(1) << i)) != 0; };

	int lane = 0;
	while (lane < Lanes)
	{
		if (!Bit(lane)) { ++lane; continue; }

		const int start = lane;
		int len = 0;
		while (lane < Lanes && Bit(lane)) { ++len; ++lane; }

		FRowRun R;
		R.StartLane = start;
		R.LenLanes = FMath::Max(len, 1);
		R.bScaffold = (InScaffoldLane >= start && InScaffoldLane < start + len);
		R.Kind = ERunKind::Solid; // default; hazards may override
		OutRuns.Add(R);
	}
}

void ALaneLevelGenerator::DespawnRow(FRowBit& Row)
{
	for (TWeakObjectPtr<APlatformStrip>& W : Row.Actors)
		if (APlatformStrip* A = W.Get())
			A->Destroy();
	Row.Actors.Empty();
}

void ALaneLevelGenerator::ApplyHazardsToRuns(uint16 MaskAbove, uint16 MaskThis, TArray<FRowRun>& Runs) const
{
	const float pBreak = BreakableRatio();   // your curve
	const float pSpike = SpikeRatio();       // your curve

	auto laneHas = [&](uint16 m, int lane)
		{
			const int idx = (lane + NumLanes) % NumLanes;
			return (m & (uint16(1) << idx)) != 0;
		};

	// 1) Roll spike first, then breakable, for NON-scaffold
	for (FRowRun& R : Runs)
	{
		R.Kind = ERunKind::Solid;

		if (!R.bScaffold)
		{
			// Spike preference (e.g., if row above is empty over this lane)
			bool anySpike = false;
			for (int i = 0; i < R.LenLanes; ++i)
			{
				const int lane = (R.StartLane + i) % NumLanes;
				const bool noAbove = !laneHas(MaskAbove, lane);
				const bool spikeRoll = FMath::FRand() < pSpike;
				if (spikeRoll && noAbove) { anySpike = true; break; }
			}
			if (anySpike)
			{
				R.Kind = ERunKind::SpikeTop;
				continue;
			}

			// Breakable roll
			if (FMath::FRand() < pBreak)
			{
				R.Kind = ERunKind::Breakable;
			}
		}
	}

	// 2) Prevent scaffold from becoming fully non-solid by accident
	if (bKeepOneSolidOnScaffold)
	{
		for (FRowRun& R : Runs)
		{
			if (R.bScaffold && R.Kind != ERunKind::Solid)
			{
				R.Kind = ERunKind::Solid;
				break;
			}
		}
	}
}

void ALaneLevelGenerator::SpawnRuns(const TArray<FRowRun>& Runs, 
									float LocalY, 
									TArray<TWeakObjectPtr<class APlatformStrip>>& OutActors)
{
	if (bGen_DryRun_NoSpawn || !ScaffoldPlatformClass) return;
	if (bGen_DryRun_NoSpawn || !ScaffoldPlatformClass) return;

	UWorld* W = GetWorld();
	if (!W) { UE_LOG(LogTemp, Error, TEXT("[SpawnRuns] World is null")); return; }

	// --- DEBUG TOGGLES (local; no header changes) ---
	const bool bDbgLog = true;          // log to Output Log
	const bool bDbgOnScreen = false;    // disable screen spam
	const float DbgLife = 2.5f;

	auto OSay = [&](const FString& S, FColor C = FColor::Cyan)
		{
			if (bDbgOnScreen && GEngine) GEngine->AddOnScreenDebugMessage(-1, DbgLife, C, S);
		};
	auto L = [&](const TCHAR* Fmt, auto... Args)
		{
			// if (bDbgLog) UE_LOG(LogTemp, Warning, Fmt, Args...);
		};

	// NEW: only force Visibility block on tiles that currently BLOCK Pawn (solid ones).
	// We do NOT change CollisionEnabled, so broken tiles can stay inert after damage.
	auto EnsureVisibilityOnSolid = [&](APlatformStrip* P)
		{
			if (!P) return;
			TInlineComponentArray<UPrimitiveComponent*> Comps(P);
			for (UPrimitiveComponent* PC : Comps)
			{
				if (!PC) continue;
				if (PC->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block)
				{
					PC->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
				}
			}
		};

	const int32 lanes = FMath::Max(NumLanes, 1);
	const int32 safeTPL = FMath::Max(TilesPerLane, 1); // tiles per lane

	struct FPlanned {
		int32 Index = 0;
		int32 TilesWide = 2;
		int32 LenLanes = 1;
		bool  bScaffold = false;
		bool  bSpawn = true;
		ERunKind Kind = ERunKind::Solid;

		// debug snapshot
		int32 Cap = 0, Lo = 0, Hi = 0;   // capacity & allowed range at pick time
	};

	auto CapacityTiles = [&](int32 lenLanes)->int32
		{
			return FMath::Max(1, lenLanes * safeTPL);
		};

	// Always call 'ok(v)'. For "no filter", pass [](int32){ return true; } at callsite.
	auto PickWeightedTileCount = [&](int32 lenLanes, TFunctionRef<bool(int32)> ok)->int32
		{
			const int32 cap = CapacityTiles(lenLanes);
			if (cap < 2) return 0; // can't fit even 2 tiles

			const int32 lo = FMath::Clamp(MinTilesPerStrip, 2, cap);
			const int32 hi = FMath::Clamp(MaxTilesPerStrip, lo, cap);

			const float exp = FMath::Max(TileCountWeightExp, 0.f);
			float total = 0.f;
			TArray<float> weights; weights.Reserve(hi - lo + 1);

			for (int v = lo; v <= hi; ++v)
			{
				const bool okv = ok(v);
				const float w = okv ? (exp > 0.f ? FMath::Pow(float(v - lo + 1), exp) : 1.f) : 0.f;
				weights.Add(w);
				total += w;
			}

			if (total <= KINDA_SMALL_NUMBER) return 0; // no legal choice under filter

			const float r = FMath::FRand() * total;
			float acc = 0.f;
			for (int i = 0; i < weights.Num(); ++i)
			{
				acc += weights[i];
				if (r <= acc) return lo + i;
			}
			return hi; // fallback (shouldn't hit)
		};

	// ---- NEW: breakable picker that guarantees at least 2-wide holes ----
	auto PickBreakableIndices = [&](int32 L)->TArray<int32>
		{
			TArray<int32> out;
			if (L < 4) return out; // no room for a 2-wide inner gap when caps are reserved

			// length -> probability of having any breakables, and number of "seeds"
			float pAny = 0.f; int32 minSeeds = 0, maxSeeds = 0;
			if (L <= 5) { pAny = 0.20f; minSeeds = 1; maxSeeds = 1; }
			else if (L <= 7) { pAny = 0.45f; minSeeds = 1; maxSeeds = 1; }
			else if (L <= 9) { pAny = 0.70f; minSeeds = 1; maxSeeds = 2; }
			else { pAny = 1.00f; minSeeds = 2; maxSeeds = 3; }

			if (FMath::FRand() > pAny) return out;

			const bool reserveEdges = true;     // keep caps solid
			const int  start = reserveEdges ? 1 : 0;
			const int  end = reserveEdges ? (L - 2) : (L - 1);

			// pool of candidate centers for pairs; avoid picking the very last inner
			// index if it can't have a right neighbor
			TArray<int32> pool;
			for (int i = start; i <= end; ++i) pool.Add(i);

			// we will form PAIRS (i plus one neighbor), make sure we don't exceed capacity
			const int32 innerSpan = end - start + 1;
			const int32 maxPossibleSeeds = FMath::Max(1, innerSpan / 2); // crude upper bound
			const int32 seeds = FMath::Clamp((minSeeds == maxSeeds) ? minSeeds : FMath::RandRange(minSeeds, maxSeeds),
				1, maxPossibleSeeds);

			auto removeAround = [&](int center)
				{
					// remove the center, its chosen neighbor (handled later), and one ring around
					pool.Remove(center - 2);
					pool.Remove(center - 1);
					pool.Remove(center);
					pool.Remove(center + 1);
					pool.Remove(center + 2);
				};

			int taken = 0;
			while (taken < seeds && pool.Num() > 0)
			{
				const int pickI = FMath::RandRange(0, pool.Num() - 1);
				const int c = pool[pickI];

				// choose a neighbor (prefer the side with space)
				TArray<int32> neighbors;
				if (c - 1 >= start) neighbors.Add(c - 1);
				if (c + 1 <= end)   neighbors.Add(c + 1);
				if (neighbors.Num() == 0) { pool.RemoveAt(pickI); continue; }

				const int nIdx = neighbors[FMath::RandRange(0, neighbors.Num() - 1)];
				out.Add(c);
				out.Add(nIdx);

				// keep indices unique & ordered
				out.Sort();

				// remove around both tiles to keep pairs separated
				removeAround(c);
				removeAround(nIdx);

				// also explicitly remove the exact neighbor entries if still present
				pool.Remove(nIdx);

				taken++;
			}

			// clamp to bounds
			out.SetNum(FMath::Min(out.Num(), L));

			// sort then dedupe in-place (linear after sort)
			out.Sort();
			int32 w = 0;
			for (int32 i = 0; i < out.Num(); ++i)
			{
				if (i == 0 || out[i] != out[i - 1])
					out[w++] = out[i];
			}
			out.SetNum(w, EAllowShrinking::No);

			return out;
		};

	// ---------- 1) Plan a size for each run ----------
	TArray<FPlanned> Plan; Plan.Reserve(Runs.Num());

	for (int32 i = 0; i < Runs.Num(); ++i)
	{
		const FRowRun& R = Runs[i];

		FPlanned P;
		P.Index = i;
		P.LenLanes = FMath::Clamp(R.LenLanes, 1, lanes);
		P.bScaffold = R.bScaffold;
		P.Kind = R.Kind;

		P.Cap = CapacityTiles(P.LenLanes);
		P.Lo = FMath::Clamp(MinTilesPerStrip, 2, P.Cap);
		P.Hi = FMath::Clamp(MaxTilesPerStrip, P.Lo, P.Cap);

		const int32 pick = PickWeightedTileCount(P.LenLanes, [](int32) { return true; });
		P.bSpawn = (pick >= 2);
		P.TilesWide = P.bSpawn ? pick : 0;

		if (bDbgLog)
		{
			L(TEXT("[SpawnRuns][Plan] RowY=%.0f Run=%d LenLanes=%d TPL=%d -> Cap=%d Range=[%d..%d] Pick=%d  (MaxTilesPerStrip=%d)"),
				LocalY, i, P.LenLanes, safeTPL, P.Cap, P.Lo, P.Hi, pick, MaxTilesPerStrip);
			if (P.Hi < 8)
			{
				L(TEXT("  └─ NOTE: Hi<8 => you will NEVER see 8+ here (reason: min(MaxTilesPerStrip=%d, Cap=%d)=%d)"),
					MaxTilesPerStrip, P.Cap, P.Hi);
			}
		}

		Plan.Add(P);
	}

	// Keep only those that can spawn
	TArray<int32> cand;
	for (int32 i = 0; i < Plan.Num(); ++i) if (Plan[i].bSpawn) cand.Add(i);
	if (cand.Num() == 0) { L(TEXT("[SpawnRuns] no candidates can fit >=2 tiles at RowY=%.0f"), LocalY); return; }

	// ---------- 2) Solo rule (threshold collapse) ----------
	{
		int32 bestIdx = cand[0];
		for (int32 id : cand) if (Plan[id].TilesWide > Plan[bestIdx].TilesWide) bestIdx = id;

		if (Plan[bestIdx].TilesWide >= SoloStripAtOrAboveTiles)
		{
			L(TEXT("[SpawnRuns][Solo] RowY=%.0f collapsing to ONE strip (Run=%d Tiles=%d >= SoloStripAtOrAboveTiles=%d)"),
				LocalY, bestIdx, Plan[bestIdx].TilesWide, SoloStripAtOrAboveTiles);

			for (int32 i = 0; i < Plan.Num(); ++i) Plan[i].bSpawn = (i == bestIdx);
			cand.Reset(); cand.Add(bestIdx);
		}
		else
		{
			L(TEXT("[SpawnRuns][Solo] RowY=%.0f not triggered (best=%d tiles, threshold=%d)"),
				LocalY, Plan[bestIdx].TilesWide, SoloStripAtOrAboveTiles);
		}
	}

	// ---------- 3) Select up to two strips ----------
	int32 primary = INDEX_NONE;
	for (int32 id : cand) if (Plan[id].bScaffold) { primary = id; break; }
	if (primary == INDEX_NONE) primary = cand[FMath::RandRange(0, cand.Num() - 1)];

	int32 secondary = INDEX_NONE;
	if (cand.Num() > 1)
	{
		TArray<int32> others; others.Reserve(cand.Num() - 1);
		for (int32 id : cand) if (id != primary) others.Add(id);
		if (others.Num() > 0) secondary = others[FMath::RandRange(0, others.Num() - 1)];
	}

	for (int32 i = 0; i < Plan.Num(); ++i) Plan[i].bSpawn = (i == primary || i == secondary);

	if (secondary == INDEX_NONE)
	{
		L(TEXT("[SpawnRuns][Select] RowY=%.0f only one candidate selected (primary Run=%d, Tiles=%d)"),
			LocalY, primary, Plan[primary].TilesWide);
	}
	else
	{
		L(TEXT("[SpawnRuns][Select] RowY=%.0f primary Run=%d Tiles=%d | secondary Run=%d Tiles=%d"),
			LocalY, primary, Plan[primary].TilesWide, secondary, Plan[secondary].TilesWide);
	}

	// ---------- 4) Enforce 'different size' if possible ----------
	TArray<int32> live; for (int32 i = 0; i < Plan.Num(); ++i) if (Plan[i].bSpawn) live.Add(i);

	if (bForceDifferentSameRow && live.Num() == 2)
	{
		const int32 ia = live[0];
		const int32 ib = live[1];

		const int32 delta = FMath::Abs(Plan[ia].TilesWide - Plan[ib].TilesWide);
		if (delta < MinTileCountDeltaSameRow)
		{
			L(TEXT("[SpawnRuns][Diff] RowY=%.0f duplicate/too-close: A=%d (Run %d), B=%d (Run %d), need >= %d"),
				LocalY, Plan[ia].TilesWide, ia, Plan[ib].TilesWide, ib, MinTileCountDeltaSameRow);

			// Try to repick B with constraint
			const int32 alt = PickWeightedTileCount(
				Plan[ib].LenLanes,
				[&](int32 v) { return FMath::Abs(v - Plan[ia].TilesWide) >= MinTileCountDeltaSameRow; }
			);

			if (alt >= 2)
			{
				L(TEXT("  └─ re-rolled B to %d (cap=%d range=[%d..%d])"), alt, Plan[ib].Cap, Plan[ib].Lo, Plan[ib].Hi);
				Plan[ib].TilesWide = alt;
			}
			else
			{
				// Keep both (so we don't collapse rows too often) but explain why it failed
				L(TEXT("  └─ no legal alternative for B within [Lo..Hi] that meets delta; keeping duplicate (RowY=%.0f)"), LocalY);
			}
		}
	}

	// ---------- 5) Spawn ----------
	const FTransform& Axf = GetActorTransform();

	for (const FPlanned& P : Plan)
	{
		if (!P.bSpawn) continue;
		const FRowRun& R = Runs[P.Index];

		const float leftCenterX = LaneCenterX_Local(R.StartLane, lanes, LaneWidthUU);
		const float centerX = leftCenterX + 0.5f * float(P.LenLanes - 1) * LaneWidthUU;

		FVector localPos(centerX, LocalY, 0.f);
		if (bLockPlatformsToPlayerY && PlayerRef)
			localPos.Y = GetTransform().InverseTransformPositionNoScale(PlayerRef->GetActorLocation()).Y;

		const FVector worldPos = Axf.TransformPosition(localPos);
		FActorSpawnParameters sp; sp.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn; sp.Owner = this;

		APlatformStrip* Plat = W->SpawnActor<APlatformStrip>(ScaffoldPlatformClass, worldPos, FRotator::ZeroRotator, sp);
		if (!Plat) { UE_LOG(LogTemp, Warning, TEXT("[SpawnRuns] failed to spawn APlatformStrip")); continue; }

		switch (P.Kind) {
		default:
		case ERunKind::Solid:     Plat->SetVisualKind(EPlatformKind::Solid);     break;
		case ERunKind::Breakable: Plat->SetVisualKind(EPlatformKind::Breakable); break;
		case ERunKind::SpikeTop:  Plat->SetVisualKind(EPlatformKind::SpikeTop);  break;
		}

		Plat->SetSpriteRollDegrees(PlatformSpriteRollDeg);
		Plat->CollisionHeightUU_Override = (PlatformCollisionHeightUU > 0.f) ? PlatformCollisionHeightUU : -1.f;
		Plat->SetCollisionPads(PlatformCollisionPadXUU, PlatformCollisionPadYUU, PlatformCollisionTopBoostUU);
		Plat->SetCollisionVisible(bRevealPlatformCollision);
		Plat->AttachToComponent(Root, FAttachmentTransformRules::KeepWorldTransform);

		// choose break tiles; each seed spawns a 2-wide breakable pair
		const TArray<int32> BreakIdx = PickBreakableIndices(P.TilesWide);

		if (BreakIdx.Num() > 0)
		{
			// Keep segment colliders so tiles are solid BEFORE they break
			const bool bPerSegmentCollision = true;
			Plat->BuildTiledByCount_WithBreaks(P.TilesWide, BreakIdx, bPerSegmentCollision);

			// Only mark solid pieces as Visibility=Block (no changes to CollisionEnabled)
			EnsureVisibilityOnSolid(Plat);
		}
		else
		{
			if (bUsePlatformSafeMode) Plat->BuildDebugFallback(P.TilesWide);
			else                      Plat->BuildTiledByCount_Flex(P.TilesWide);

			// Only mark solid pieces as Visibility=Block
			EnsureVisibilityOnSolid(Plat);
		}

		// wall clamp
		const float halfX = Plat->GetCollisionHalfExtentX();
		float faceLeftX = Xmin, faceRightX = Xmax;
		if (WallTileWUU > 0.f) {
			const float leftColX = Xmin + WallInsetX;
			const float rightColX = Xmax - WallInsetX;
			const float halfWallW = 0.5f * WallTileWUU;
			faceLeftX = leftColX + halfWallW - WallFaceContactInsetUU;
			faceRightX = rightColX - halfWallW + WallFaceContactInsetUU;
		}
		const float minCenterX = faceLeftX + halfX + PlatformWallClearanceUU;
		const float maxCenterX = faceRightX - halfX - PlatformWallClearanceUU;
		const FVector curLocal = Axf.InverseTransformPosition(Plat->GetActorLocation());
		const float clampedX = FMath::Clamp(curLocal.X, minCenterX, maxCenterX);
		if (!FMath::IsNearlyEqual(clampedX, curLocal.X, 0.1f)) {
			const FVector newWorld = Axf.TransformPosition(FVector(clampedX, curLocal.Y, curLocal.Z));
			Plat->SetActorLocation(newWorld, false);
		}

		TrySpawnEnemyOnStrip(Plat, P.TilesWide, P.Kind);

		// Label
		const FString tag = FString::Printf(TEXT("RowY=%.0f Run=%d Len=%d Cap=%d [%d..%d] -> Tiles=%d"),
			LocalY, P.Index, P.LenLanes, P.Cap, P.Lo, P.Hi, P.TilesWide);
		L(TEXT("[SpawnRuns][Spawn] %s"), *tag);
		OSay(tag, FColor::Green);

#if WITH_EDITOR
		Plat->SetActorLabel(FString::Printf(TEXT("PStrip_%dW_Row%.0f_Run%d"), P.TilesWide, LocalY, P.Index));
#endif

		OutActors.Add(Plat);
	}
}

void ALaneLevelGenerator::TrySpawnEnemyOnStrip(APlatformStrip* Plat, int32 TilesWide, ERunKind RunKind)
{
	if (!Plat) return;

	// Platform "row depth" in generator local space
	const float localY = GetActorTransform().InverseTransformPosition(Plat->GetActorLocation()).Y;

	// Count currently-active enemies in a pool
	auto CountActive = [](const TArray<TObjectPtr<ACPP_EnemyParent>>& Pool) -> int32
		{
			int32 n = 0;
			for (ACPP_EnemyParent* E : Pool)
			{
				if (E && E->IsActive()) { ++n; }
			}
			return n;
		};

	// Platform top Z from bounds
	FVector PlatOrigin(0.f), PlatExtents(0.f);
	Plat->GetActorBounds(true, /*out*/PlatOrigin, /*out*/PlatExtents);
	const float PlatformTopZ = PlatOrigin.Z + PlatExtents.Z;

	// Enemy half-height (capsule preferred, fallback to root bounds)
	auto GetEnemyHalfZ = [](UClass* EnemyClass) -> float
		{
			if (!EnemyClass) return 0.f;
			const ACPP_EnemyParent* CDO = Cast<ACPP_EnemyParent>(EnemyClass->GetDefaultObject());
			if (!CDO) return 0.f;

			if (const UCapsuleComponent* Cap = CDO->FindComponentByClass<UCapsuleComponent>())
			{
				return Cap->GetScaledCapsuleHalfHeight();
			}
			if (const UPrimitiveComponent* RootPrim = Cast<UPrimitiveComponent>(CDO->GetRootComponent()))
			{
				return RootPrim->Bounds.BoxExtent.Z;
			}
			return 0.f;
		};

	const float WalkerHalfZ = GetEnemyHalfZ(WalkerEnemyClass);
	const float FlyerHalfZ = GetEnemyHalfZ(FlyerEnemyClass);

	const bool bGroundOK = (RunKind != ERunKind::SpikeTop);

	// -------------------------
	// Regular Walker (grounded)
	// -------------------------
	if (bGroundOK && WalkerEnemyClass && TilesWide >= MinTilesForWalker)
	{
		if (CountActive(WalkerPool) < MaxActive_Walker &&
			localY >= NextWalkerLocalY &&
			FMath::FRand() <= WalkerSpawnChance)
		{
			if (ACPP_EnemyParent* W = BorrowFromPool(WalkerPool, WalkerEnemyClass, PoolSize_Walker))
			{
				FVector Spawn = Plat->GetActorLocation();
				Spawn.Z = PlatformTopZ + WalkerHalfZ + WalkerHoverZ;
				W->ActivateFromPool(Spawn);
				NextWalkerLocalY = localY + WalkerMinDYBetweenSpawnsUU;
			}
		}
	}

	// ------------------------------------------------------
	// Walker ALT (same behavior; your BP has an extra tag)
	// Chance = 0.5 * WalkerSpawnChance (shares same gates)
	// ------------------------------------------------------
	if (bGroundOK && WalkerAltEnemyClass && TilesWide >= MinTilesForWalker)
	{
		if (CountActive(WalkerPool) < MaxActive_Walker &&
			localY >= NextWalkerLocalY &&
			FMath::FRand() <= (0.5f * WalkerSpawnChance))
		{
			if (ACPP_EnemyParent* WB = BorrowFromPool(WalkerPool, WalkerAltEnemyClass, PoolSize_Walker))
			{
				FVector Spawn = Plat->GetActorLocation();
				Spawn.Z = PlatformTopZ + WalkerHalfZ + WalkerHoverZ;
				WB->ActivateFromPool(Spawn);
				NextWalkerLocalY = localY + WalkerMinDYBetweenSpawnsUU;
			}
		}
	}

	// --------------
	// Flyer (air)
	// --------------
	if (FlyerEnemyClass)
	{
		if (CountActive(FlyerPool) < MaxActive_Flyer &&
			localY >= NextFlyerLocalY)
		{
			if (ACPP_EnemyParent* F = BorrowFromPool(FlyerPool, FlyerEnemyClass, PoolSize_Flyer))
			{
				FVector Spawn = Plat->GetActorLocation();
				Spawn.Z = PlatformTopZ + FlyerHalfZ + FlyerSpawnAboveZ;
				F->ActivateFromPool(Spawn);
				NextFlyerLocalY = localY + FlyerMinDYBetweenSpawnsUU;
			}
		}
	}
}

void ALaneLevelGenerator::DestroyAllRows()
{
	for (FRowBit& Row : LiveRows)
	{
		DespawnRow(Row);
	}
	LiveRows.Empty();
	NextRowIndex = 0;
	// park the cursor at current player Y so we can decide a fresh offset on resume
	CursorLocalY = PlayerLocalY();
}

void ALaneLevelGenerator::DestroyAllWalls(bool bDisableBlockers)
{
	// visual tiles
	for (auto* C : WallSpritesLeft)  if (C) C->DestroyComponent();
	for (auto* C : WallSpritesRight) if (C) C->DestroyComponent();
	WallSpritesLeft.Empty();
	WallSpritesRight.Empty();

	// reset streaming cursors
	WallTopY = 0.f;
	WallNextSpawnY = 0.f;

	// optionally disable the blocking boxes too
	if (bDisableBlockers)
	{
		if (LeftWall) { LeftWall->SetCollisionEnabled(ECollisionEnabled::NoCollision);  LeftWall->SetHiddenInGame(true); }
		if (RightWall) { RightWall->SetCollisionEnabled(ECollisionEnabled::NoCollision); RightWall->SetHiddenInGame(true); }
	}
}


void ALaneLevelGenerator::PauseAndFlush(bool bAlsoWalls, bool bDisableWallBlockers)
{
	// stop generation
	bSpawnPlatforms = false;

	// clear spawned content
	DestroyAllRows();
	if (bAlsoWalls) DestroyAllWalls(bDisableWallBlockers);

	// reset “start after delay” state so resume is clean
	PlatformSpawnStartTime = 0.f;
	bFirstPlatformSpawn = true;

	// ⬇️ ADD THESE LINES (append before the closing brace)
	NextWalkerLocalY = -FLT_MAX;
	NextFlyerLocalY = -FLT_MAX;

	// optional hard flush of currently active enemies
	for (TObjectPtr<ACPP_EnemyParent>& P : WalkerPool) if (P && P->IsActive()) P->DeactivateToPool();
	for (TObjectPtr<ACPP_EnemyParent>& P : FlyerPool)  if (P && P->IsActive()) P->DeactivateToPool();
}

void ALaneLevelGenerator::ResumeSpawning(float StartDelaySeconds, float StartBelowPlayerScreens)
{
	// re-enable blockers if you had disabled them
	if (LeftWall) { LeftWall->SetHiddenInGame(false);  LeftWall->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); }
	if (RightWall) { RightWall->SetHiddenInGame(false); RightWall->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); }

	// seed wall visuals again if you want them visible
	if (bShowWalls) InitWallsInfinite();

	// schedule spawning with your existing delay/first-spawn path
	PlatformSpawnDelay = StartDelaySeconds;
	PlatformSpawnStartTime = 0.f;     // let Tick set it on first frame
	bFirstPlatformSpawn = true;

	// choose where the very first row starts (below player)
	PlatformSpawnOffsetY = StartBelowPlayerScreens * ScreenWorldHeightUU;

	bSpawnPlatforms = true;

	// ⬇️ ADD THESE LINES (append before the closing brace)
	NextWalkerLocalY = -FLT_MAX;
	NextFlyerLocalY = -FLT_MAX;
}




