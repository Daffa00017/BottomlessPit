// PlatFormStrip header

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PaperSprite.h"
#include "PaperSpriteComponent.h"
#include "PlatformStrip.generated.h"

UENUM(BlueprintType)
enum class ETileBand : uint8
{
	TwoCaps,   // [LeftCap][RightCap]
	Three,     // [LeftCap][Middle xN>=1][RightCap]
	Five       // [OuterLeft][MiddleLeft][Middle xN>=0][MiddleRight][OuterRight]
};

UENUM(BlueprintType)
enum class EPlatformKind : uint8 { Solid, Breakable, SpikeTop };

class UBoxComponent;
class UPaperSpriteComponent;
struct FDamageEvent;
struct FPointDamageEvent;

UCLASS()
class BOTTOMLESSPIT_API APlatformStrip : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APlatformStrip();

    UFUNCTION(BlueprintCallable, Category = "Platform|Build") void BuildTiled(float TargetWidthUU);
    UFUNCTION(BlueprintCallable, Category = "Platform|Build") void BuildTiledByCount(int32 TileCount);
    UFUNCTION(BlueprintCallable, Category = "Platform|Build") void BuildTiledByCount_Safe(int32 TileCount);
    UFUNCTION(BlueprintCallable, Category = "Platform|Build") void BuildTiledByCount_Flex(int32 Count);
    UFUNCTION(BlueprintCallable, Category = "Platform") void SetVisualKind(EPlatformKind InKind);
    UFUNCTION(BlueprintCallable, Category = "Platform|Render") void SetSpriteRollDegrees(float InRoll);
    UFUNCTION(BlueprintCallable, Category = "Collision|Debug") void SetCollisionVisible(bool bVisible);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tiling") ETileBand Style = ETileBand::Three;

    // Assign these in the BP child (sprites from your artist)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tiling") UPaperSprite* OuterLeft = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tiling") UPaperSprite* MiddleLeft = nullptr;   // used in Five
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tiling") UPaperSprite* Middle = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tiling") UPaperSprite* MiddleRight = nullptr;   // used in Five
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tiling") UPaperSprite* OuterRight = nullptr;

    // --- Add under your existing sprite fields (keep your current ones) ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tiling|Breakable") UPaperSprite* Break_OuterLeft = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tiling|Breakable") UPaperSprite* Break_MiddleLeft = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tiling|Breakable") UPaperSprite* Break_Middle = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tiling|Breakable") UPaperSprite* Break_MiddleRight = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tiling|Breakable") UPaperSprite* Break_OuterRight = nullptr;

    // Build mixed strips (some tiles solid, some breakable)
    UFUNCTION(BlueprintCallable, Category = "Platform|Build")
    void BuildTiledByCount_WithBreaks(int32 Count, const TArray<int32>& BreakIndices, bool bPerSegmentCollision);

    // Break a specific tile at runtime (called by damage)
    UFUNCTION(BlueprintCallable, Category = "Platform")
    void BreakTile(int32 TileIndex);

    // AActor override: route point-damage to a tile index
    virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
        class AController* EventInstigator, AActor* DamageCauser) override;

    UFUNCTION(BlueprintCallable, Category = "Platform|Breakable")
    void BreakInRadius(const FVector& WorldCenter, float RadiusUU);

    UFUNCTION(BlueprintCallable, Category = "Platform|Breakable")
    void BreakAtWorldPoint(const FVector& WorldPoint); // still handy elsewhere


    // If you want 1 row high, collision height = tileH; otherwise expose this
    UPROPERTY(EditAnywhere, Category = "Tiling") float CollisionHeightUU_Override = -1.f; // <0 = use tileH

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform|Render")
    float SpriteRollDeg = 0.f;

    // How much to shrink the collider on the left & right (UU each side)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision")
    float CollisionInsetXUU = 8.f;

    // Thickness along +Y (forward/back). If <=0, keep existing.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision")
    float CollisionDepthUU = 24.f;

    // Grow/shrink the collision box (UU). All applied in the strip's local space.
// X = left/right across lanes, Y = fall axis, Z = out-of-plane (unchanged here).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision")
    float CollisionPadXUU = 0.f;            // +X/-X (symmetric)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision")
    float CollisionPadYUU = 0.f;            // front/back (symmetric)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision")
    float CollisionTopBoostYUU = 0.f;       // one-sided: lifts the TOP face upward (toward -Y)

    // If true, the collider's TOP face is placed at local Y=0 (sprite row), so it hugs the art.
    // If false, sizing is symmetric (top boost only raises the top by half and lowers center).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision")
    bool bCollisionAnchorTopToRow = true;

    // Convenience if you want to push from the generator
    UFUNCTION(BlueprintCallable, Category = "Collision")
    void SetCollisionPads(float InPadX, float InPadY, float InTopBoostY);

    // Optional: let generator set them at spawn time
    UFUNCTION(BlueprintCallable, Category = "Collision")
    void SetCollisionDepthUU(float InDepth) { CollisionDepthUU = InDepth; }

    UFUNCTION(BlueprintCallable, Category = "Collision")
    void SetCollisionInsetXUU(float InInset) { CollisionInsetXUU = InInset; }

    UFUNCTION(BlueprintCallable, Category = "Collision")
    void SetCollisionAnchorTop(bool bAnchor) { bCollisionAnchorTopToRow = bAnchor; }

    UFUNCTION(BlueprintCallable, Category = "Collision")
    float GetCollisionHalfExtentX() const;

    UPROPERTY(EditDefaultsOnly, Category = "Collision")
    TEnumAsByte<ECollisionChannel> PlayerObjectChannel = ECC_Pawn;

    UPROPERTY(EditDefaultsOnly, Category = "Collision")
    TEnumAsByte<ECollisionChannel> ProjectileObjectChannel = ECC_Pawn;

    // assign sprites (variants) for each kind
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visuals")
    UPaperSprite* SolidMiddle = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visuals")
    UPaperSprite* BreakableMiddle = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visuals")
    UPaperSprite* SpikeTopMiddle = nullptr;

    // which set to use when building
    UFUNCTION(BlueprintCallable) void SetKind(EPlatformKind InKind) { VisualKind = InKind; }
    UFUNCTION(BlueprintCallable) EPlatformKind GetKind() const { return VisualKind; }

    // Misc
    UPROPERTY(EditAnywhere, Category = "Tiling") int32   TranslucentSortPriority = 10;
    UPROPERTY(EditAnywhere, Category = "Tiling") float   VisualYOffsetUU = 0.f;  // nudge if needed

    // Old API for strips; now it just calls BuildTiled
    UFUNCTION(BlueprintCallable) void SetSizeUU(float WidthUU, float HeightUU);

    UFUNCTION(BlueprintCallable, Category = "Platform")
    float GetCollisionHeightUU() const;

    UFUNCTION(BlueprintCallable, Category = "Debug")
    void BuildDebugFallback(int32 TileCount);


    //EnemySpawner

    UFUNCTION(BlueprintPure, Category = "Platform|Spawn")
    FVector GetTileCenterWorld(int32 TileIndex, float HoverZ = 8.f) const;

    UFUNCTION(BlueprintPure, Category = "Platform|Spawn")
    bool GetRandomSpawnPoint(int32& OutTileIdx, FVector& OutWorld, float HoverZ = 8.f, int32 ExcludeEdgeTiles = 1) const;

protected:
    UPROPERTY(VisibleAnywhere) USceneComponent* Root;
    UPROPERTY(VisibleAnywhere) UBoxComponent* Box;
    // Keep one “marker” sprite; we’ll hide it and spawn children for tiles
    UPROPERTY(VisibleAnywhere) UPaperSpriteComponent* Sprite;

    // helpers
    FVector2f GetTileSizeUU(UPaperSprite* S) const; // (w,h) in UU from PPUU and source px
    void      ClearBuiltTiles();                    // delete previously spawned tile components
    void AddTile(UPaperSprite* S, float XLocal, float ZLocal);
    void ApplyCollisionSizing(float UsedWidthUU, float TileHeightUU);

    UPROPERTY(EditAnywhere, Category = "Visuals") EPlatformKind VisualKind = EPlatformKind::Solid;

    UPaperSprite* PickMiddle() const
    {
        switch (VisualKind)
        {
        default:
        case EPlatformKind::Solid:     return SolidMiddle ? SolidMiddle : Middle;
        case EPlatformKind::Breakable: return BreakableMiddle ? BreakableMiddle : Middle;
        case EPlatformKind::SpikeTop:  return SpikeTopMiddle ? SpikeTopMiddle : Middle;
        }
    }

private :
    // runtime bookkeeping
    TArray<bool> TileIsBreakable;
    TArray<bool> TileIsBroken;
    TArray<UPaperSpriteComponent*> TileSprites; // optional, to hide on break
    TArray<class UBoxComponent*> SegmentBoxes;  // per-segment colliders when we have holes

    // last build geometry (to map hit->tile)
    int32  BuiltCount = 0;
    float  BuiltTileW = 16.f;
    float  BuiltLeftX = 0.f;   // center of tile #0 in local X
    float  BuiltTileH = 16.f;
    bool  bUsingSegmentCollision = false;

    void   RebuildSegmentCollision(); // build colliders for contiguous solid spans
    UPaperSprite* PickSlotSprite(bool bBreak, int tileIndex, int total) const;

    float CachedHalfExtentX = 0.f;
};




