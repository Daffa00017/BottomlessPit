// Include necessary Unreal headers
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Delegates/DelegateCombinations.h"
#include "CPP_GM_BottomlessPit.generated.h"

// Declare your delegate outside the class declaration
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnScoreUpdated, float, NewScore);

/**
 * GameMode class for BottomlessPit
 */
UCLASS()
class BOTTOMLESSPIT_API ACPP_GM_BottomlessPit : public AGameModeBase
{
    GENERATED_BODY()

public:
    // Constructor
    ACPP_GM_BottomlessPit();

    // Called when the game starts
    virtual void BeginPlay() override;

    /** Getter for score */
    UFUNCTION(BlueprintCallable, Category = "Scoring")
    float GetScore() const { return PlayerScore; }

    // Start scoring
    UFUNCTION(BlueprintCallable, Category = "Scoring")
    void StartScoring();

    // Stop scoring
    UFUNCTION(BlueprintCallable, Category = "Scoring")
    void StopScoring();

    UFUNCTION(BlueprintCallable, Category = "Scoring")
    void AddScore(float Amount);

    /** Delegate broadcast whenever score changes */
    UPROPERTY(BlueprintAssignable, Category = "Scoring")
    FOnScoreUpdated OnScoreUpdated;

    /** Internal function to update score */
    void UpdateScore(float Amount);


    // How far the player must fall before earning another chunk of score
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoring")
    float StepSize;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoring")
    float ScoreUpdateInterval = 0.3;

    // How many points per step
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoring")
    int32 PointsPerStep;

    // Keep track of the "next target depth"
    float NextScoreThresholdZ;

protected:
    /** Timer handle for depth checking */
    FTimerHandle DepthCheckTimerHandle;

    /** Score multiplier (points per unit fallen) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoring")
    float ScoreMultiplier;

    /** Current score */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoring")
    float PlayerScore;

    /** The lowest Z value reached */
    float LowestZ;

    /** Reference to tracked player pawn */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoring")
    AActor* TrackedActorRef;

    /** Function called on timer */
    void CheckPlayerDepth();
};



