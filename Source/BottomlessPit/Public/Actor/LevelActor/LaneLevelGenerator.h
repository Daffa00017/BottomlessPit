// LaneLevelGenerator

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Actor/LevelActor/PlatformStrip.h"
#include "Pawn/Enemy/CPP_EnemyParent.h"
#include "LaneLevelGenerator.generated.h"

class UPaperSprite;
class UBoxComponent;
class APlatformStrip;
class ACPP_EnemyParent; // fwd

UCLASS()
class BOTTOMLESSPIT_API ALaneLevelGenerator : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ALaneLevelGenerator();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platforms")
	bool bSpawnPlatforms = false;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	/** Root scene */
	UPROPERTY(VisibleAnywhere, Category = "Wrap|Components")
	USceneComponent* Root;

	/** Seam triggers */
	UPROPERTY(VisibleAnywhere, Category = "Wrap|Components")
	UBoxComponent* LeftSeam;

	UPROPERTY(VisibleAnywhere, Category = "Wrap|Components")
	UBoxComponent* RightSeam;

	// ------------------- Tunables -------------------

	// === Runtime control (call these from your death/respawn flow) ===
	UFUNCTION(BlueprintCallable, Category = "Runtime")
	void PauseAndFlush(bool bAlsoWalls = true, bool bDisableWallBlockers = true);

	UFUNCTION(BlueprintCallable, Category = "Runtime")
	void ResumeSpawning(float StartDelaySeconds = 0.75f, float StartBelowPlayerScreens = 1.0f);

	/** Number of logic lanes across the screen (5 recommended). */
	UPROPERTY(EditAnywhere, Category = "Playfield")
	int32 NumLanes = 5;

	/** Width of one lane in UU. If you want ~10 tiles across with 16 UU tiles, set 2 tiles/lane -> 32 UU. */
	UPROPERTY(EditAnywhere, Category = "Playfield", meta = (ClampMin = "4"))
	float LaneWidthUU = 32.f;

	/** Camera/visible height in world units (used to size seam boxes if following player). */
	UPROPERTY(EditAnywhere, Category = "Playfield", meta = (ClampMin = "256"))
	float ScreenWorldHeightUU = 1024.f;

	/** Seam half-thickness along X (thin). */
	UPROPERTY(EditAnywhere, Category = "Wrap", meta = (ClampMin = "1"))
	float SeamHalfThicknessX = 8.f;

	/** How many screen-heights the seams extend up/down (half-extents). */
	UPROPERTY(EditAnywhere, Category = "Wrap", meta = (ClampMin = "1"))
	float SeamHalfHeightsInScreens = 3.f;

	/** Seams follow player Y so they always cover the current play region. */
	UPROPERTY(EditAnywhere, Category = "Wrap")
	bool bSeamsFollowPlayerY = true;

	/** Eligible actors: true = wrap any Pawn by default (plus tags). */
	UPROPERTY(EditAnywhere, Category = "Wrap")
	bool bWrapPawnsByDefault = true;

	/** Actors with this tag will be wrapped (e.g., add "WrapEligible" to Player/Enemies/Projectiles). */
	UPROPERTY(EditAnywhere, Category = "Wrap")

	FName EligibleTag = FName(TEXT("WrapEligible"));

	/** Small inward inset so we don't remain inside the seam after teleport. */
	UPROPERTY(EditAnywhere, Category = "Wrap", meta = (ClampMin = "0"))
	float InsetUU = 2.f;

	/** Cooldown after wrapping to avoid ping-ponging (seconds). */
	UPROPERTY(EditAnywhere, Category = "Wrap", meta = (ClampMin = "0"))
	float WrapCooldownSeconds = 0.10f;

	/** Draw seam debug lines. */
	UPROPERTY(EditAnywhere, Category = "Debug")
	bool bDrawDebugSeams = true;

	/** Optional reference to the player for seam Y-follow. */
	UPROPERTY(BlueprintReadWrite, Category = "Refs")
	AActor* PlayerRef = nullptr;

	// Tell the generator which sprite defines a "tile" (e.g., your 16x16 rock)
	UPROPERTY(EditAnywhere, Category = "Refs")
	UPaperSprite* ReferenceTileSprite = nullptr;

	// ------------------- Derived -------------------

	/** Total playfield width (NumLanes * LaneWidthUU). */
	UPROPERTY(VisibleAnywhere, Category = "Playfield")
	float PlayfieldWidthUU = 160.f;

	/** Xmin / Xmax seams (centered at world X=0). */
	UPROPERTY(VisibleAnywhere, Category = "Playfield")
	float Xmin = -80.f;

	UPROPERTY(VisibleAnywhere, Category = "Playfield")
	float Xmax = +80.f;

	UPROPERTY(EditAnywhere, Category = "Debug")
	bool bDrawLaneGuides = true;

	//--Platform--
	// ---- Rows / segments ----
	UPROPERTY(EditAnywhere, Category = "Rows")
	float RowHeightUU = 176.f;                  // ~11 tiles if 16 UU per tile
	UPROPERTY(EditAnywhere, Category = "Rows")
	int32 RowsPerSegment = 6;                   // about one screen worth
	UPROPERTY(EditAnywhere, Category = "Rows")
	float SpawnLeadScreens = 1.25f;             // how far ahead to build
	UPROPERTY(EditAnywhere, Category = "Rows")
	float CullBufferScreens = 2.5f;             // how far above to keep

	// ---- Scaffold turn chance (ramps later) ----
	UPROPERTY(EditAnywhere, Category = "Platforms")
	float TurnChanceStart = 0.25f;              // 25% at start
	UPROPERTY(EditAnywhere, Category = "Platforms")
	float TurnChanceMax = 0.40f;              // 40% deep
	UPROPERTY(EditAnywhere, Category = "Platforms")
	float DepthAtMaxScreens = 30.f;             // depth where it hits max

	// How thick (along +Y) each platform's collision should be. If <= 0, the strip auto-computes.
	UPROPERTY(EditAnywhere, Category = "Platforms|Tuning")
	float PlatformCollisionHeightUU = 96.f;

	// Roll (degrees) applied to each spawned platform's sprites (0 = default; try 90 or -90 if needed)
	UPROPERTY(EditAnywhere, Category = "Platforms|Tuning")
	float PlatformSpriteRollDeg = 0.f;

	// Pads applied to every spawned platform (can still be overridden per BP child)
	UPROPERTY(EditAnywhere, Category = "Platforms|Collision")
	float PlatformCollisionPadXUU = 0.f;         // extend left/right equally

	UPROPERTY(EditAnywhere, Category = "Platforms|Collision")
	float PlatformCollisionPadYUU = 0.f;         // extend forward/back equally

	UPROPERTY(EditAnywhere, Category = "Platforms|Collision")
	float PlatformCollisionTopBoostUU = 0.f;

	UPROPERTY(EditAnywhere, Category = "Platforms|Collision")
	float PlatformWallClearanceUU = 4.f;

	UPROPERTY(EditAnywhere, Category = "Platforms")
	int32 TilesPerLane = 2; // 1 or 2 recommended

	UPROPERTY(EditAnywhere, Category = "Platforms")
	int32 ScaffoldTilesPerRun = 2; // how many tiles wide a scaffold platform is

	// Prefab for scaffold floors
	UPROPERTY(EditAnywhere, Category = "Platforms")
	TSubclassOf<class APlatformStrip> ScaffoldPlatformClass;

	// Platform sizing (UU)
	UPROPERTY(EditAnywhere, Category = "Platforms")
	float ScaffoldWidthMul = 0.9f;   // 90% of lane width so it looks inset
	UPROPERTY(EditAnywhere, Category = "Platforms")
	float ScaffoldHeightUU = 24.f;   // thin slab; tweak to feel

	// Depth (Z) placement for spawned platforms
	UPROPERTY(EditAnywhere, Category = "Platforms|Depth")
	bool bLockPlatformsToPlayerZ = true;

	UPROPERTY(EditAnywhere, Category = "Platforms|Depth", meta = (EditCondition = "!bLockPlatformsToPlayerZ"))
	float PlatformsZ = 0.f; // use this when not locking to player

	// ---- Depth (Y) placement ----
	UPROPERTY(EditAnywhere, Category = "Platforms|Depth")
	bool bLockPlatformsToPlayerY = true;

	UPROPERTY(EditAnywhere, Category = "Platforms|Depth", meta = (EditCondition = "!bLockPlatformsToPlayerY"))
	float PlatformsY = 0.f; // used if not locking to player

	// ---- Platform pass (extra platforms) ----
	UPROPERTY(EditAnywhere, Category = "Platforms")
	float ExtraPlatformChanceStart = 0.35f;     // start density
	UPROPERTY(EditAnywhere, Category = "Platforms")
	float ExtraPlatformChanceMax = 0.55f;     // deeper density
	UPROPERTY(EditAnywhere, Category = "Platforms")
	float DepthAtMax_Platforms = 30.f;      // screens to reach max

	// Widths (in lanes) for non-scaffold runs
	UPROPERTY(EditAnywhere, Category = "Platforms")
	int32 MinRunLanes = 1;                      // 1..N lanes
	UPROPERTY(EditAnywhere, Category = "Platforms")
	int32 MaxRunLanes = 2;

	// Max horizontal gap allowed between reachable landings (in lanes)
	UPROPERTY(EditAnywhere, Category = "Platforms")
	int32 MaxGapLanes = 1;                      // safe shift per row

	// Max extra platforms per row (to keep it airy)
	UPROPERTY(EditAnywhere, Category = "Platforms")
	int32 MaxExtrasPerRow = 2;

	// Reroll attempts if a row fails validation
	UPROPERTY(EditAnywhere, Category = "Platforms")
	int32 RowRerollAttempts = 3;

	// Random tile length per platform strip (visual width in tiles)
	UPROPERTY(EditAnywhere, Category = "Platforms|Spawn")
	int32 MinTilesPerStrip = 2;

	UPROPERTY(EditAnywhere, Category = "Platforms|Spawn")
	int32 MaxTilesPerStrip = 12;

	// ---- Row spawn rules ----
	UPROPERTY(EditAnywhere, Category = "Platforms|Spawn", meta = (ClampMin = "2"))
	int32 SoloStripAtOrAboveTiles = 12;   // if any strip in the row has this many tiles or more, spawn only that one

	UPROPERTY(EditAnywhere, Category = "Platforms|Spawn")
	bool bForceDifferentSameRow = true;   // keep tile counts distinct across strips in the same row

	UPROPERTY(EditAnywhere, Category = "Platforms|Spawn", meta = (ClampMin = "1"))
	int32 MinTileCountDeltaSameRow = 1;   // at least this much difference (1 = “not equal”)

	UPROPERTY(EditAnywhere, Category = "Platforms|Spawn", meta = (ClampMin = "0.0"))
	float TileCountWeightExp = 1.4f;      // >0 biases toward longer strips; 0 = uniform

	UPROPERTY(EditAnywhere, Category = "Platforms|Spawn")
	float PlatformSpawnDelay = 1.2f;
	float PlatformSpawnStartTime = 0.f;

	UPROPERTY(VisibleAnywhere, Category = "Platforms|Spawn")
	bool bFirstPlatformSpawn = true;

	// Platform spawn offset (Y-axis) for smooth transition from the main menu
	UPROPERTY(EditAnywhere, Category = "Platforms|Spawn")
	float PlatformSpawnOffsetY = 1000.f;

	// ---- Hazards (ramps by depth) ----
	UPROPERTY(EditAnywhere, Category = "Hazards")
	float BreakableStart = 0.10f;       // 10% at start
	UPROPERTY(EditAnywhere, Category = "Hazards")
	float BreakableMax = 0.35f;       // 35% deep
	UPROPERTY(EditAnywhere, Category = "Hazards")
	float SpikesStart = 0.05f;       // 5% at start
	UPROPERTY(EditAnywhere, Category = "Hazards")
	float SpikesMax = 0.20f;       // 20% deep
	UPROPERTY(EditAnywhere, Category = "Hazards")
	float DepthAtMax_Haz = 30.f;        // screens to reach max

	UPROPERTY(EditAnywhere, Category = "Hazards")
	bool bNeverSpikeScaffold = true;

	// "Don't spike both sides of the same gap" soft guard
	UPROPERTY(EditAnywhere, Category = "Hazards")
	bool bAvoidDoubleSidedGapSpikes = true;

	// Breakables must not cover the whole scaffold run
	UPROPERTY(EditAnywhere, Category = "Hazards")
	bool bKeepOneSolidOnScaffold = true;

	//WALL
	// Walls (still used)
	UPROPERTY(EditAnywhere, Category = "Walls") bool bShowWalls = false;
	UPROPERTY(EditAnywhere, Category = "Walls") TArray<UPaperSprite*> WallVariants;
	UPROPERTY(EditAnywhere, Category = "Walls", meta = (ClampMin = "0")) float WallInsetX = 4.f;
	UPROPERTY(EditAnywhere, Category = "Walls") int32 WallVerticalTilesPadding = 2;
	UPROPERTY(EditAnywhere, Category = "Walls", meta = (ClampMin = "0.5")) float WallsHalfScreensVisible = 2.0f;
	UPROPERTY(EditAnywhere, Category = "Walls") float WallSpawnLeadScreens = 2.0f;
	UPROPERTY(EditAnywhere, Category = "Walls") float WallCullAboveScreens = 3.0f;

	float WallTileWUU = 0.f;
	float WallTileHUU = 0.f;
	float WallTopY = 0.f;
	float WallNextSpawnY = 0.f;

	UPROPERTY(EditAnywhere, Category = "Walls|Visual") float WallSpriteRollDeg = -90.f;
	UPROPERTY(EditAnywhere, Category = "Walls|Visual") bool bMirrorRightColumn = true;

	UPROPERTY(VisibleAnywhere, Category = "Walls|Collision") UBoxComponent* LeftWall = nullptr;
	UPROPERTY(VisibleAnywhere, Category = "Walls|Collision") UBoxComponent* RightWall = nullptr;
	UPROPERTY(EditAnywhere, Category = "Walls|Collision", meta = (ClampMin = "1")) float WallThicknessUU = 8.f;
	UPROPERTY(EditAnywhere, Category = "Walls|Collision") float WallBlockThicknessUU = 4.f;
	UPROPERTY(EditAnywhere, Category = "Walls|Collision") float WallFaceContactInsetUU = 0.0f;

	// Sprite columns (runtime)
	UPROPERTY() TArray<UPaperSpriteComponent*> WallSpritesLeft;
	UPROPERTY() TArray<UPaperSpriteComponent*> WallSpritesRight;

	// ---- Debug ----
	UPROPERTY(EditAnywhere, Category = "Debug")
	bool bDrawScaffoldDebug = true;

	// ---- Runtime state ----
	int32  ScaffoldLane = 2;
	int64  NextRowIndex = 0;
	float  CursorLocalY = 0.f;

	// ==== Crash isolation toggles ====
	UPROPERTY(EditAnywhere, Category = "Debug|Isolation")
	bool bGen_DryRun_NoSpawn = true;     // if true, GenerateScaffoldSegment won't spawn actors

	UPROPERTY(EditAnywhere, Category = "Debug|Isolation")
	bool bGen_SkipHazards = true;       // if true, skip ApplyHazardsToRuns

	UPROPERTY(EditAnywhere, Category = "Debug|Isolation")
	bool bGen_SkipRuns = false;          // if true, skip BuildRunsFromMask and SpawnRuns

	UPROPERTY(EditAnywhere, Category = "Debug|Isolation")
	int32 Gen_MaxRowsPerTick = 8;        // per-tick cap (0..32)

	UPROPERTY(EditAnywhere, Category = "Debug")
	bool bRevealPlatformCollision = false;

	//EnemiesSpawner
	UPROPERTY(EditAnywhere, Category = "Enemies")
	TSubclassOf<ACPP_EnemyParent> WalkerEnemyClass;

	UPROPERTY(EditAnywhere, Category = "Enemies")
	TSubclassOf<ACPP_EnemyParent> FlyerEnemyClass;

	UPROPERTY(EditAnywhere, Category = "Enemies")
	int32 MinTilesForWalker = 6;

	UPROPERTY(EditAnywhere, Category = "Enemies")
	float WalkerHoverZ = 12.f;        // how high above the strip to drop the pawn

	UPROPERTY(EditAnywhere, Category = "Enemies")
	float WalkerSpawnChance = 1.0f;   // 0..1 scalar; set <1 if you want variety

	UPROPERTY(EditAnywhere, Category = "Enemies")
	float FlyerSpawnAboveZ = 220.f;   // spawn flyers higher up than platforms

	// PoolEnemies
	UPROPERTY(EditAnywhere, Category = "Enemies|Pool") int32 PoolSize_Walker = 12;
	UPROPERTY(EditAnywhere, Category = "Enemies|Pool") int32 PoolSize_Flyer = 8;
	UPROPERTY(EditAnywhere, Category = "Enemies|Pool") int32 MaxActive_Walker = 6;
	UPROPERTY(EditAnywhere, Category = "Enemies|Pool") int32 MaxActive_Flyer = 4;

	// NEW: despawn band and recycle cadence
	UPROPERTY(EditAnywhere, Category = "Enemies|Pool") float DespawnAfterYDelta = 2000.f;
	UPROPERTY(EditAnywhere, Category = "Enemies|Pool") float RecycleInterval = 0.25f;

	// NEW: optional spacing so we don’t spawn too often vertically
	UPROPERTY(EditAnywhere, Category = "Enemies|Pool") float MinYBetweenSpawns_Walker = 700.f;
	UPROPERTY(EditAnywhere, Category = "Enemies|Pool") float MinYBetweenSpawns_Flyer = 1000.f;

	// MUST be UPROPERTY so GC keeps references
	UPROPERTY() TArray<TObjectPtr<ACPP_EnemyParent>> WalkerPool;
	UPROPERTY() TArray<TObjectPtr<ACPP_EnemyParent>> FlyerPool;

	float NextWalkerY = -FLT_MAX;
	float NextFlyerY = -FLT_MAX;

	// ---- Enemy local-depth gates (NEW) ----
	UPROPERTY(EditAnywhere, Category = "Enemies|Pool") float WalkerMinDYBetweenSpawnsUU = 400.f;
	UPROPERTY(EditAnywhere, Category = "Enemies|Pool") float FlyerMinDYBetweenSpawnsUU = 600.f;

	float NextWalkerLocalY = -FLT_MAX;
	float NextFlyerLocalY = -FLT_MAX;

	

private:

	// cooldown tracker
	TMap<TWeakObjectPtr<AActor>, float> CooldownUntil;

	// cached seam half height in UU
	float SeamHalfHeightUU = 0.f;

	// Overlap handlers
	UFUNCTION()
	void OnLeftSeamBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void OnRightSeamBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
		const FHitResult& SweepResult);

	// Implementation
	void RecomputePlayfield();
	void UpdateSeamPosY();
	bool IsEligible(AActor* A) const;
	void TryWrap(AActor* A, bool bFromLeftSeam);
	void DrawSeamDebug() const;
	void EnsureSegmentsAhead();
	void CullOldRows();
	void GenerateScaffoldSegment();
	void DrawScaffoldDebug();
	void GenerateScaffoldSegment_Simple();  // NEW
	void InitWallsInfinite();
	void UpdateWallsInfinite();
	FVector RowCenterWorld(int32 Lane, float LocalY) const;
	void DestroyAllRows();
	void DestroyAllWalls(bool bDisableBlockers);


	// Build a mask for one row given scaffold lane; bits 0..NumLanes-1 (supports >8 lanes)
	uint16 BuildRowMask_WithExtras(int32 ScaffoldLaneIdx, uint16 PrevMask) const;

	// Validate mask given previous row's reachable lanes (simple check)
	bool ValidateRowMask(uint16 PrevMask, uint16 ThisMask) const;

	UPROPERTY(EditAnywhere, Category = "Debug")
	bool bUsePlatformSafeMode = true;  // start true to diagnose


	//--Helper--

	float DepthScreens() const
	{
		// how many segments we built so far (approx): NextRowIndex / RowsPerSegment
		return (RowsPerSegment > 0) ? float(NextRowIndex) / float(RowsPerSegment) : 0.f;
	}
	float TurnChance() const
	{
		const float t = FMath::Clamp(DepthScreens() / FMath::Max(DepthAtMaxScreens, 1.f), 0.f, 1.f);
		return FMath::Lerp(TurnChanceStart, TurnChanceMax, t);
	}

	float PlayerLocalY() const
	{
		if (!PlayerRef) return 0.f;
		return GetActorTransform().InverseTransformPosition(PlayerRef->GetActorLocation()).Y;
	}

	struct FRowBit
	{
		uint16 ScaffoldBits = 0;    // was uint8
		int64  RowIndex = 0;
		float  LocalY = 0.f;
		TArray<TWeakObjectPtr<APlatformStrip>> Actors;
	};

	TArray<FRowBit> LiveRows;

	// (optional) small helper:
	void DespawnRow(FRowBit& Row);

	float ExtraPlatformChance() const
	{
		const float t = FMath::Clamp(DepthScreens() / FMath::Max(DepthAtMax_Platforms, 1.f), 0.f, 1.f);
		return FMath::Lerp(ExtraPlatformChanceStart, ExtraPlatformChanceMax, t);
	}

	enum class ERunKind : uint8 { Solid, Breakable, SpikeTop };

	float BreakableRatio() const
	{
		const float t = FMath::Clamp(DepthScreens() / FMath::Max(DepthAtMax_Haz, 1.f), 0.f, 1.f);
		return FMath::Lerp(BreakableStart, BreakableMax, t);
	}
	float SpikeRatio() const
	{
		const float t = FMath::Clamp(DepthScreens() / FMath::Max(DepthAtMax_Haz, 1.f), 0.f, 1.f);
		return FMath::Lerp(SpikesStart, SpikesMax, t);
	}

	// A compact run we can assign a hazard kind to, then spawn
	struct FRowRun
	{
		int32 StartLane = 0;
		int32 LenLanes = 1;
		bool  bScaffold = false;
		ERunKind Kind = ERunKind::Solid;
	};

	void BuildRunsFromMask(uint16 Mask, int32 InScaffoldLane, TArray<FRowRun>& OutRuns) const;

	// Apply hazard choices to runs (mutates Runs[i].Kind)
	void ApplyHazardsToRuns(uint16 PrevMask, uint16 CurrMask, TArray<FRowRun>& Runs) const;

	// Spawn runs with chosen Kind; pushes created actors to OutActors
	void SpawnRuns(const TArray<FRowRun>& Runs, float LocalY, TArray<TWeakObjectPtr<class APlatformStrip>>& OutActors);

	//EnemiesSpawnerHelper
	void TrySpawnEnemyOnStrip(class APlatformStrip* Plat, int32 TilesWide, ERunKind RunKind);

	// --- pool helpers (private) ---
	void WarmEnemyPools();
	ACPP_EnemyParent* BorrowFromPool(TArray<TObjectPtr<ACPP_EnemyParent>>& Pool, TSubclassOf<ACPP_EnemyParent> Cls, int32 PoolCap);
	void TryEnemiesPool();
	void RecycleIfOutOfWindow(ACPP_EnemyParent* E, float PlayerY);

	FTimerHandle PoolTimer;

};



