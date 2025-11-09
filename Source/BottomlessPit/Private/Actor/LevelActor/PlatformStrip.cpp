// PlatformStrip implementation


#include "Actor/LevelActor/PlatformStrip.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/EngineTypes.h"     // FDamageEvent, FHitResult
#include "Engine/DamageEvents.h"    // FPointDamageEvent, FRadialDamageEvent, and inline helpers
#include "GameFramework/DamageType.h"
#include "Engine/HitResult.h"

// Sets default values
APlatformStrip::APlatformStrip()
{
    PrimaryActorTick.bCanEverTick = false;
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
    Box->SetupAttachment(Root);
    Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Box->SetCollisionResponseToAllChannels(ECR_Ignore);
    Box->SetCollisionResponseToChannel(PlayerObjectChannel, ECR_Block);
    Box->SetCollisionResponseToChannel(ProjectileObjectChannel, ECR_Block);
    Box->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
    Box->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

    Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Sprite"));
    Sprite->SetupAttachment(Root);
    Sprite->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Sprite->SetHiddenInGame(true); // we build with child sprites
}


FVector2f APlatformStrip::GetTileSizeUU(UPaperSprite* S) const
{
    if (!S) return FVector2f(16.f, 16.f);
    const float ppuu = FMath::Max(S->GetPixelsPerUnrealUnit(), 0.001f);
    const FVector2D px = S->GetSourceSize();
    return FVector2f(px.X / ppuu, px.Y / ppuu);
}

void APlatformStrip::ClearBuiltTiles()
{
    TArray<USceneComponent*> children;
    Root->GetChildrenComponents(false, children);
    for (USceneComponent* c : children)
    {
        if (c != Box && c != Sprite) { c->DestroyComponent(); }
    }
}

void APlatformStrip::AddTile(UPaperSprite* S, float XLocal, float ZLocal)
{
    if (!S) return;
    auto* C = NewObject<UPaperSpriteComponent>(this);
    C->RegisterComponent();
    C->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);
    C->SetSprite(S);

    // Put tiles in XZ plane so the camera (looking along ±Y) actually sees them.
    C->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));   // << KEY LINE
    C->SetRelativeLocation(FVector(XLocal, 0.f, ZLocal + VisualYOffsetUU));
    C->SetRelativeRotation(FRotator(0.f, 0.f, SpriteRollDeg));

    C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    C->SetHiddenInGame(false);                          // make sure it’s visible
    C->SetMobility(EComponentMobility::Movable);
    C->SetTranslucentSortPriority(TranslucentSortPriority);

}

void APlatformStrip::ApplyCollisionSizing(float UsedWidthUU, float TileHeightUU)
{
    if (!Box) return;

    const float baseY = (CollisionHeightUU_Override > 0.f) ? CollisionHeightUU_Override : TileHeightUU;

    const float halfX = FMath::Max(UsedWidthUU * 0.5f + CollisionPadXUU, 2.f);
    const float halfY = FMath::Max(baseY * 0.5f + CollisionPadYUU * 0.5f + CollisionTopBoostYUU * 0.5f, 2.f);
    const float halfZ = FMath::Max(Box->GetUnscaledBoxExtent().Z, 2.f);
    const float centerShiftY = bCollisionAnchorTopToRow ? (+halfY) : (-0.5f * CollisionTopBoostYUU);

    Box->SetBoxExtent(FVector(halfX, halfY, halfZ), /*bUpdateOverlaps=*/true);
    Box->SetRelativeLocation(FVector(0.f, centerShiftY, 0.f));
    Box->MarkRenderStateDirty();

    CachedHalfExtentX = halfX;   // << cache used for wall-clamp
}

void APlatformStrip::RebuildSegmentCollision()
{
    // clear old segments
    for (UBoxComponent* B : SegmentBoxes) if (B) B->DestroyComponent();
    SegmentBoxes.Empty();

    // same Y/Z sizing as your big box so depth/height/anchor match
    const float baseY = (CollisionHeightUU_Override > 0.f) ? CollisionHeightUU_Override : BuiltTileH;
    const float halfY = FMath::Max(baseY * 0.5f + CollisionPadYUU * 0.5f + CollisionTopBoostYUU * 0.5f, 2.f);
    const float centerShiftY = bCollisionAnchorTopToRow ? (+halfY) : (-0.5f * CollisionTopBoostYUU);
    const float halfZ = FMath::Max(Box ? Box->GetUnscaledBoxExtent().Z : 2.f, 2.f);

    auto solidAt = [&](int i)->bool
        {
            const bool breakable = (i < TileIsBreakable.Num()) ? TileIsBreakable[i] : false;
            const bool broken = (i < TileIsBroken.Num()) ? TileIsBroken[i] : false;
            // breakables are solid until actually broken
            return !(breakable && broken);
        };

    float maxHalfX = 0.f;

    int i = 0;
    while (i < BuiltCount)
    {
        if (!solidAt(i)) { ++i; continue; }
        int j = i;
        while (j < BuiltCount && solidAt(j)) ++j; // [i..j-1] is one solid span

        const int   spanCount = (j - i);
        const float spanW = spanCount * BuiltTileW;

        // >>> IMPORTANT: no X-pad on segments so they stay centered under their tiles.
        const float halfX = FMath::Max(0.5f * spanW, 2.f);
        const float centerX = BuiltLeftX + (float(i) + 0.5f * float(spanCount)) * BuiltTileW;

        UBoxComponent* B = NewObject<UBoxComponent>(this);
        B->RegisterComponent();
        B->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);

        // inherit profile; force enabled (big Box was disabled on first break)
        if (Box)
        {
            B->SetCollisionProfileName(Box->GetCollisionProfileName());
            B->SetCollisionObjectType(Box->GetCollisionObjectType());
        }
        else
        {
            B->SetCollisionProfileName(TEXT("BlockAll"));
            B->SetCollisionObjectType(ECC_WorldStatic);
        }
        B->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        B->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
        B->SetCollisionResponseToChannel(PlayerObjectChannel, ECR_Block);  // <-- add
        B->SetCollisionResponseToChannel(ProjectileObjectChannel, ECR_Block);
        B->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

        B->SetBoxExtent(FVector(halfX, halfY, halfZ), /*update=*/true);
        B->SetRelativeLocation(FVector(centerX, centerShiftY, 0.f));

        SegmentBoxes.Add(B);
        maxHalfX = FMath::Max(maxHalfX, halfX);

        i = j;
    }

    // If you cache half-extent for wall clamp after breaking, you can keep this at 0
    // or set to the max segment halfX. Your spawn-time clamp already used the big box.
    // CachedHalfExtentX = (SegmentBoxes.Num() > 0) ? maxHalfX : 0.f;
}

UPaperSprite* APlatformStrip::PickSlotSprite(bool bBreak, int tileIndex, int total) const
{
    const bool twoCaps = (Style == ETileBand::TwoCaps);
    const bool three = (Style == ETileBand::Three);
    const bool five = (Style == ETileBand::Five);

    auto pickBreak = [&](UPaperSprite* BreakSprite, UPaperSprite* Fallback)
        { return bBreak && BreakSprite ? BreakSprite : (Fallback ? Fallback : Middle); };

    if (twoCaps)
    {
        if (tileIndex == 0)        return bBreak ? (Break_OuterLeft ? Break_OuterLeft : OuterLeft) : OuterLeft;
        else /* tileIndex == 1 */       return bBreak ? (Break_OuterRight ? Break_OuterRight : OuterRight) : OuterRight;
    }
    else if (three)
    {
        if (tileIndex == 0)        return bBreak ? (Break_OuterLeft ? Break_OuterLeft : OuterLeft) : OuterLeft;
        else if (tileIndex == total - 1)  return bBreak ? (Break_OuterRight ? Break_OuterRight : OuterRight) : OuterRight;
        else                             return pickBreak(Break_Middle, PickMiddle());
    }
    else /* five */
    {
        if (tileIndex == 0)        return bBreak ? (Break_OuterLeft ? Break_OuterLeft : OuterLeft) : OuterLeft;
        else if (tileIndex == 1)        return bBreak ? (Break_MiddleLeft ? Break_MiddleLeft : (MiddleLeft ? MiddleLeft : PickMiddle()))
            : (MiddleLeft ? MiddleLeft : PickMiddle());
        else if (tileIndex == total - 2)  return bBreak ? (Break_MiddleRight ? Break_MiddleRight : (MiddleRight ? MiddleRight : PickMiddle()))
            : (MiddleRight ? MiddleRight : PickMiddle());
        else if (tileIndex == total - 1)  return bBreak ? (Break_OuterRight ? Break_OuterRight : OuterRight) : OuterRight;
        else                             return pickBreak(Break_Middle, PickMiddle());
    }
}

void APlatformStrip::BuildTiled(float TargetWidthUU)
{
    // Pick the “baseline” tile (Middle if present, else any non-null)
    UPaperSprite* Baseline = Middle ? Middle : (OuterLeft ? OuterLeft : OuterRight);
    if (!Baseline) {
        UE_LOG(LogTemp, Error, TEXT("PlatformStrip: No sprite set (Middle/OuterLeft/OuterRight are null)."));
        return;
    }

    const FVector2f TileSize = GetTileSizeUU(Baseline);
    const float tileW = TileSize.X;
    const float tileH = TileSize.Y;

    // Minimum counts based on style
    int32 minTiles = 0;
    switch (Style)
    {
    case ETileBand::TwoCaps: minTiles = 2; break;
    case ETileBand::Three:   minTiles = 3; break;
    case ETileBand::Five:    minTiles = 5; break;
    }

    // Choose how many tiles we can fit (at least minTiles)
    const int32 tilesFit = FMath::Max(minTiles, FMath::FloorToInt(TargetWidthUU / tileW));

    // Compute used width and left edge so it’s CENTERED on the actor
    const float usedW = tilesFit * tileW;
    const float leftX = -usedW * 0.5f + tileW * 0.5f; // center of first tile

    ClearBuiltTiles();

    // Place tiles left->right according to style
    int32 i = 0;
    if (Style == ETileBand::TwoCaps)
    {
        AddTile(OuterLeft, leftX + i++ * tileW, 0.f);
        AddTile(OuterRight, leftX + i++ * tileW, 0.f);
    }
    else if (Style == ETileBand::Three)
    {
        AddTile(OuterLeft, leftX + i++ * tileW, 0.f);
        // middles (at least 1)
        for (; i < tilesFit - 1; ++i) AddTile(Middle, leftX + i * tileW, 0.f);
        AddTile(OuterRight, leftX + i * tileW, 0.f);
    }
    else // Five
    {
        AddTile(OuterLeft, leftX + i++ * tileW, 0.f);
        AddTile(MiddleLeft, leftX + i++ * tileW, 0.f);
        // core middles (may be 0 if tilesFit==5)
        for (; i < tilesFit - 2; ++i) AddTile(Middle, leftX + i * tileW, 0.f);
        AddTile(MiddleRight, leftX + i++ * tileW, 0.f);
        AddTile(OuterRight, leftX + i * tileW, 0.f);
    }

    const float collH = (CollisionHeightUU_Override > 0.f) ? CollisionHeightUU_Override : tileH;
    // X = width, Y = (thin) depth, Z = height
    Box->SetBoxExtent(FVector(usedW * 0.5f, CollisionDepthUU * 0.5f, collH * 0.5f));
    Box->SetRelativeLocation(FVector(0.f, 0.f, 0.f)); // keep centered; we top-align in the generator

}

void APlatformStrip::BuildTiledByCount(int32 TileCount)
{
    // Pick a baseline sprite to read size/PPUU (prefer hazard-aware middle)
    UPaperSprite* Baseline = PickMiddle();
    if (!Baseline) Baseline = Middle ? Middle : (OuterLeft ? OuterLeft : OuterRight);
    if (!Baseline) return;

    const FVector2f Tile = GetTileSizeUU(Baseline);
    float tileW = Tile.X;
    const float tileH = Tile.Y;

    if (TileCount <= 0) TileCount = 1;

    ClearBuiltTiles();

    // --- Style-specific placement ---
    if (Style == ETileBand::TwoCaps)
    {
        // Use actual cap widths so collider span matches visuals
        const float wL = OuterLeft ? GetTileSizeUU(OuterLeft).X : tileW;
        const float wR = OuterRight ? GetTileSizeUU(OuterRight).X : tileW;
        const float usedW = wL + wR;

        // Center whole band; place each cap at its own half-width
        const float leftEdge = -0.5f * usedW;
        const float xL = leftEdge + 0.5f * wL;
        const float xR = leftEdge + wL + 0.5f * wR;

        AddTile(OuterLeft, xL, 0.f);
        AddTile(OuterRight, xR, 0.f);

        ApplyCollisionSizing(/*UsedWidthUU=*/usedW, /*TileHeightUU=*/tileH);
        return;
    }

    // For 3/5 styles, we assume uniform tile width (taken from Baseline)
    int32 tilesUsed = TileCount;
    if (Style == ETileBand::Three) tilesUsed = FMath::Max(TileCount, 3);
    else                           tilesUsed = FMath::Max(TileCount, 5);

    const float usedW = tilesUsed * tileW;
    const float leftX = -0.5f * usedW + 0.5f * tileW;

    if (Style == ETileBand::Three)
    {
        int32 i = 0;
        AddTile(OuterLeft, leftX + (i++) * tileW, 0.f);

        // Fill with hazard-aware middle if available
        UPaperSprite* MidFill = PickMiddle();
        for (; i < tilesUsed - 1; ++i)
        {
            AddTile(MidFill ? MidFill : Middle, leftX + i * tileW, 0.f);
        }

        AddTile(OuterRight, leftX + (i)*tileW, 0.f);
    }
    else // Five
    {
        int32 i = 0;
        AddTile(OuterLeft, leftX + (i++) * tileW, 0.f);
        AddTile(MiddleLeft, leftX + (i++) * tileW, 0.f);

        UPaperSprite* MidFill = PickMiddle();
        for (; i < tilesUsed - 2; ++i)
        {
            AddTile(MidFill ? MidFill : Middle, leftX + i * tileW, 0.f);
        }

        AddTile(MiddleRight, leftX + (i++) * tileW, 0.f);
        AddTile(OuterRight, leftX + (i)*tileW, 0.f);
    }

    // Size the real collider from the built span (pads/top-anchoring applied inside)
    ApplyCollisionSizing(/*UsedWidthUU=*/usedW, /*TileHeightUU=*/tileH);
}


void APlatformStrip::SetVisualKind(EPlatformKind InKind)
{
    VisualKind = InKind;
}

void APlatformStrip::SetSpriteRollDegrees(float InRoll)
{
    SpriteRollDeg = InRoll;
}

void APlatformStrip::SetCollisionVisible(bool bVisible)
{
    if (!Box) return;

    // Make the shape component render its wireframe even when not selected
    Box->bDrawOnlyIfSelected = false;

    // Show it in game
    Box->SetHiddenInGame(!bVisible);
    Box->SetVisibility(bVisible, true);

    // Make sure render state updates
    Box->MarkRenderStateDirty();
}

void APlatformStrip::BuildTiledByCount_WithBreaks(int32 Count, const TArray<int32>& BreakIndices, bool bPerSegmentCollision)
{
    // Baseline to get size/PPUU (prefer hazard-aware middle)
    UPaperSprite* Baseline = PickMiddle();
    if (!Baseline) Baseline = Middle ? Middle : (OuterLeft ? OuterLeft : OuterRight);
    if (!Baseline) return;

    const FVector2f T = GetTileSizeUU(Baseline);
    BuiltTileW = T.X;
    BuiltTileH = T.Y;

    // Clamp request and apply style minimums -> this is the real visual count
    const int32 Raw = FMath::Clamp(Count, 2, 128);
    int32 tilesUsed = Raw;
    if (Style == ETileBand::TwoCaps) tilesUsed = FMath::Max(Raw, 2);
    else if (Style == ETileBand::Three) tilesUsed = FMath::Max(Raw, 3);
    else                                tilesUsed = FMath::Max(Raw, 5);

    BuiltCount = tilesUsed;

    const float usedW = tilesUsed * BuiltTileW;
    BuiltLeftX = -0.5f * usedW + 0.5f * BuiltTileW;

    // flags
    TileIsBreakable.Init(false, BuiltCount);
    TileIsBroken.Init(false, BuiltCount);
    for (int idx : BreakIndices)
        if (idx >= 0 && idx < BuiltCount) TileIsBreakable[idx] = true;

    // visuals
    ClearBuiltTiles();
    TileSprites.Empty();
    TileSprites.Reserve(BuiltCount);

    for (int32 i = 0; i < BuiltCount; ++i)
    {
        const bool bBreak = TileIsBreakable[i];
        UPaperSprite* S = PickSlotSprite(bBreak, i, BuiltCount);
        AddTile(S, BuiltLeftX + float(i) * BuiltTileW, 0.f);

        TArray<USceneComponent*> kids; Root->GetChildrenComponents(false, kids);
        UPaperSpriteComponent* Comp = nullptr;
        for (int c = kids.Num() - 1; c >= 0; --c)
            if (auto* P = Cast<UPaperSpriteComponent>(kids[c])) { Comp = P; break; }
        TileSprites.Add(Comp);
    }

    // ---- COLLISION: one box per tile (perfectly centered + exact tile width) ----
    // Destroy any old boxes we might have
    for (UBoxComponent* B : SegmentBoxes) if (B) B->DestroyComponent();
    SegmentBoxes.Empty();
    SegmentBoxes.Reserve(BuiltCount);

    // size on Y/Z matches your big box logic so depth/height/anchor is consistent
    const float baseY = (CollisionHeightUU_Override > 0.f) ? CollisionHeightUU_Override : BuiltTileH;
    const float halfY = FMath::Max(baseY * 0.5f + CollisionPadYUU * 0.5f + CollisionTopBoostYUU * 0.5f, 2.f);
    const float centerShiftY = bCollisionAnchorTopToRow ? (+halfY) : (-0.5f * CollisionTopBoostYUU);
    const float halfZ = FMath::Max(Box ? Box->GetUnscaledBoxExtent().Z : 2.f, 2.f);

    for (int32 i = 0; i < BuiltCount; ++i)
    {
        UBoxComponent* B = NewObject<UBoxComponent>(this);
        B->RegisterComponent();
        B->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);

        // copy profile from the big box
        if (Box)
        {
            B->SetCollisionProfileName(Box->GetCollisionProfileName());
            B->SetCollisionObjectType(Box->GetCollisionObjectType());
        }
        else
        {
            B->SetCollisionProfileName(TEXT("BlockAll"));
            B->SetCollisionObjectType(ECC_WorldStatic);
        }
        B->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        B->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
        B->SetCollisionResponseToChannel(PlayerObjectChannel, ECR_Block);  // <-- add
        B->SetCollisionResponseToChannel(ProjectileObjectChannel, ECR_Block);        // player/enemies fall through
        B->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
        B->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

        const float halfX = FMath::Max(0.5f * BuiltTileW, 2.f); // EXACT tile width
        const float centerX = BuiltLeftX + float(i) * BuiltTileW; // tile center

        B->SetBoxExtent(FVector(halfX, halfY, halfZ), /*update=*/true);
        B->SetRelativeLocation(FVector(centerX, centerShiftY, 0.f));

        SegmentBoxes.Add(B);
    }

    // keep the big box only as a cached width for wall-clamp; disable/hide it
    ApplyCollisionSizing(/*UsedWidthUU=*/usedW, /*TileHeightUU=*/BuiltTileH);
    if (Box)
    {
        Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Box->SetHiddenInGame(true);         // optional: hide debug shape
        Box->SetVisibility(false, true);    // optional: hide debug shape
    }
}

void APlatformStrip::BreakTile(int32 TileIndex)
{
    if (TileIndex < 0 || TileIndex >= BuiltCount) return;
    if (!TileIsBreakable.IsValidIndex(TileIndex) || !TileIsBreakable[TileIndex]) return;
    if (!TileIsBroken.IsValidIndex(TileIndex) || TileIsBroken[TileIndex])    return;

    TileIsBroken[TileIndex] = true;

    // hide just that tile’s sprite
    if (TileSprites.IsValidIndex(TileIndex) && TileSprites[TileIndex])
    {
        TileSprites[TileIndex]->SetHiddenInGame(true);
        TileSprites[TileIndex]->SetVisibility(false, true);
    }

    // disable only that tile’s collider
    if (SegmentBoxes.IsValidIndex(TileIndex) && SegmentBoxes[TileIndex])
    {
        UBoxComponent* SB = SegmentBoxes[TileIndex];
        SB->SetCollisionEnabled(ECollisionEnabled::QueryOnly);                // still traceable, no physical block
        SB->SetCollisionResponseToChannel(PlayerObjectChannel, ECR_Ignore);        // player/enemies fall through
        SB->SetCollisionResponseToChannel(ProjectileObjectChannel, ECR_Ignore);        // player/enemies fall through
        SB->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);        // walkers “see” a hole/ledge here
    }
}

float APlatformStrip::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    // get a best-effort impact point
    FHitResult Hit;
    FVector   ImpulseDir = FVector::ZeroVector;
    DamageEvent.GetBestHitInfo(this, DamageCauser, Hit, ImpulseDir);

    const FVector worldHit =
        Hit.bBlockingHit ? Hit.ImpactPoint :
        (DamageCauser ? DamageCauser->GetActorLocation() : GetActorLocation());

    // map local X to tile index (tile 0 centered at BuiltLeftX)
    const FVector local = GetActorTransform().InverseTransformPosition(worldHit);
    const float   denom = FMath::Max(BuiltTileW, 0.001f);
    const int32   tile = FMath::Clamp(FMath::RoundToInt((local.X - BuiltLeftX) / denom),
        0, FMath::Max(0, BuiltCount - 1));

    BreakTile(tile);
    return DamageAmount;
}

void APlatformStrip::BreakInRadius(const FVector& WorldCenter, float RadiusUU)
{
    if (BuiltCount <= 0 || BuiltTileW <= KINDA_SMALL_NUMBER) return;

    const FVector L = GetActorTransform().InverseTransformPosition(WorldCenter);
    const float   r = FMath::Max(RadiusUU, 0.f);

    // convert radius in X to tile index range
    const float leftX  = L.X - r;
    const float rightX = L.X + r;

    int32 iLo = FMath::FloorToInt((leftX  - BuiltLeftX) / BuiltTileW);
    int32 iHi = FMath::FloorToInt((rightX - BuiltLeftX) / BuiltTileW);
    iLo = FMath::Clamp(iLo, 0, BuiltCount - 1);
    iHi = FMath::Clamp(iHi, 0, BuiltCount - 1);

    bool changed=false;
    for (int32 i=iLo; i<=iHi; ++i)
    {
        if (TileIsBreakable.IsValidIndex(i) && !TileIsBroken[i])
        {
            TileIsBroken[i] = true;
            if (TileSprites.IsValidIndex(i) && TileSprites[i])
            {
                TileSprites[i]->SetHiddenInGame(true);
                TileSprites[i]->SetVisibility(false, true);
            }
            changed = true;
        }
    }
    if (changed)
    {
        if (SegmentBoxes.Num()==0 && Box)
            Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        RebuildSegmentCollision();
    }
}

void APlatformStrip::BreakAtWorldPoint(const FVector& WorldPoint)
{
    if (BuiltCount <= 0 || BuiltTileW <= KINDA_SMALL_NUMBER) return;
    const FVector L = GetActorTransform().InverseTransformPosition(WorldPoint);
    const int32 idx = FMath::Clamp(
        FMath::RoundToInt((L.X - BuiltLeftX) / BuiltTileW), 0, BuiltCount - 1);
    BreakTile(idx);
}

void APlatformStrip::BuildTiledByCount_Safe(int32 TileCount)
{
    // Validate sprites needed for the chosen Style.
    // We favor the middle via PickMiddle() so hazard kinds can swap visuals.
    auto WarnFallback = [&](const TCHAR* Reason)
        {
            UE_LOG(LogTemp, Warning, TEXT("PlatformStrip: missing sprite(s) for Style %d (%s). Fallback to collider-only."),
                int32(Style), Reason);
            BuildDebugFallback(TileCount);
        };

    switch (Style)
    {
    case ETileBand::TwoCaps:
        if (!(OuterLeft && OuterRight)) { WarnFallback(TEXT("TwoCaps requires OuterLeft/OuterRight")); return; }
        break;

    case ETileBand::Three:
        if (!(OuterLeft && PickMiddle() && OuterRight)) { WarnFallback(TEXT("Three requires OuterLeft/Middle(or kind)/OuterRight")); return; }
        break;

    case ETileBand::Five:
        if (!(OuterLeft && MiddleLeft && PickMiddle() && MiddleRight && OuterRight))
        {
            WarnFallback(TEXT("Five requires OuterLeft/MiddleLeft/Middle(or kind)/MiddleRight/OuterRight"));
            return;
        }
        break;
    }

    // If we’re here, the sprite set is sufficient. Delegate to the regular tiler.
    // Let the tiler compute collision height unless explicitly overridden.
    if (CollisionHeightUU_Override <= 0.f) CollisionHeightUU_Override = -1.f;
    BuildTiledByCount(TileCount);
}

void APlatformStrip::BuildTiledByCount_Flex(int32 Count)
{
    // Pick baseline to get sizes/PPUU (prefer hazard-aware middle)
    UPaperSprite* Baseline = PickMiddle();
    if (!Baseline) Baseline = Middle ? Middle : (OuterLeft ? OuterLeft : OuterRight);
    if (!Baseline) return;

    const FVector2f T = GetTileSizeUU(Baseline);
    const float tileW = T.X;
    const float tileH = T.Y;

    const int32 N = FMath::Clamp(Count, 2, 128);

    ClearBuiltTiles();

    // Compute span we intend to draw (for collider sizing)
    float usedW = 0.f;

    if (N == 2)
    {
        usedW = 2.f * tileW;
        const float leftX = -0.5f * usedW + 0.5f * tileW;
        AddTile(OuterLeft, leftX + 0.f * tileW, 0.f);
        AddTile(OuterRight, leftX + 1.f * tileW, 0.f);
    }
    else if (N == 3)
    {
        usedW = 3.f * tileW;
        const float leftX = -0.5f * usedW + 0.5f * tileW;
        AddTile(OuterLeft, leftX + 0.f * tileW, 0.f);
        AddTile(Middle ? Middle : Baseline, leftX + 1.f * tileW, 0.f);
        AddTile(OuterRight, leftX + 2.f * tileW, 0.f);
    }
    else if (N == 4)
    {
        usedW = 4.f * tileW;
        const float leftX = -0.5f * usedW + 0.5f * tileW;
        AddTile(OuterLeft, leftX + 0.f * tileW, 0.f);
        AddTile(MiddleLeft ? MiddleLeft : (Middle ? Middle : Baseline), leftX + 1.f * tileW, 0.f);
        AddTile(MiddleRight ? MiddleRight : (Middle ? Middle : Baseline), leftX + 2.f * tileW, 0.f);
        AddTile(OuterRight, leftX + 3.f * tileW, 0.f);
    }
    else
    {
        usedW = float(N) * tileW;
        const float leftX = -0.5f * usedW + 0.5f * tileW;

        int32 i = 0;
        AddTile(OuterLeft, leftX + (i++) * tileW, 0.f);
        AddTile(MiddleLeft ? MiddleLeft : (Middle ? Middle : Baseline), leftX + (i++) * tileW, 0.f);

        const int32 innerCount = N - 4;
        UPaperSprite* MidFill = PickMiddle();
        for (int32 k = 0; k < innerCount; ++k, ++i)
            AddTile(MidFill ? MidFill : (Middle ? Middle : Baseline), leftX + i * tileW, 0.f);

        AddTile(MiddleRight ? MiddleRight : (Middle ? Middle : Baseline), leftX + (i++) * tileW, 0.f);
        AddTile(OuterRight, leftX + (i)*tileW, 0.f);
    }

    // Size collider to match span (pads/top-anchor handled inside helper)
    ApplyCollisionSizing(usedW, tileH);
}

void APlatformStrip::SetCollisionPads(float InPadX, float InPadY, float InTopBoostY)
{
    CollisionPadXUU = InPadX;
    CollisionPadYUU = InPadY;
    CollisionTopBoostYUU = InTopBoostY;
}

float APlatformStrip::GetCollisionHalfExtentX() const
{
    return CachedHalfExtentX;
}

void APlatformStrip::SetSizeUU(float WidthUU, float HeightUU)
{
    // Forward to tiled with width; keep height via CollisionHeightUU_Override if you like
    BuildTiled(WidthUU);
}

float APlatformStrip::GetCollisionHeightUU() const
{
    // Box extent.Y is half-height in UU (local). ScaledBoxExtent covers component scale.
    return Box ? (Box->GetScaledBoxExtent().Z * 2.f) : 0.f;
}

void APlatformStrip::BuildDebugFallback(int32 TileCount)
{
    // Minimal visual or none; we only care about the collider span here
    ClearBuiltTiles();

    // Derive a reasonable tile size from any available sprite; if none, use a sane default
    float tileW = 32.f, tileH = 16.f;
    if (UPaperSprite* Ref = Middle ? Middle : (OuterLeft ? OuterLeft : OuterRight))
    {
        const FVector2f Tile = GetTileSizeUU(Ref);
        tileW = Tile.X; tileH = Tile.Y;
    }

    if (TileCount <= 0) TileCount = 1;
    const float usedW = TileCount * tileW;

    // Just size the collider; no sprite tiles needed for the fallback
    ApplyCollisionSizing(/*UsedWidthUU=*/usedW, /*TileHeightUU=*/tileH);
}

FVector APlatformStrip::GetTileCenterWorld(int32 TileIndex, float HoverZ) const
{
    // clamp just in case
    if (BuiltCount <= 0) return GetActorLocation();
    TileIndex = FMath::Clamp(TileIndex, 0, BuiltCount - 1);

    const float x = BuiltLeftX + float(TileIndex) * BuiltTileW; // center of that tile
    const FVector local(x, 0.f, HoverZ);
    return GetActorTransform().TransformPosition(local);
}

bool APlatformStrip::GetRandomSpawnPoint(int32& OutTileIdx, FVector& OutWorld, float HoverZ, int32 ExcludeEdgeTiles) const
{
    if (BuiltCount <= (ExcludeEdgeTiles * 2)) return false;

    const int32 lo = ExcludeEdgeTiles;
    const int32 hi = BuiltCount - 1 - ExcludeEdgeTiles;

    OutTileIdx = FMath::RandRange(lo, hi);
    OutWorld = GetTileCenterWorld(OutTileIdx, HoverZ);
    return true;
}




