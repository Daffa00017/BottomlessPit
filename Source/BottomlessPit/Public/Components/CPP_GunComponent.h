// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CPP_GunComponent.generated.h"


UCLASS(ClassGroup = (Custom), Blueprintable, meta = (BlueprintSpawnableComponent))
class BOTTOMLESSPIT_API UCPP_GunComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCPP_GunComponent();

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gun")
    TSubclassOf<AActor> GunClass;

    UPROPERTY(BlueprintReadOnly, Category = "Gun")
    AActor* CurrentGunActor = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gun")
    float FireRate = 8.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gun|Fire")
    float RoundsPerMinute = 600.f;   // 600 RPM = 0.1s between shots

    UFUNCTION(BlueprintCallable, Category = "Gun")
    void SpawnAndAttachGun(USceneComponent* AttachTo, FName Socket = NAME_None);

    UFUNCTION(BlueprintCallable, Category = "Gun")
    void StartFire();

    UFUNCTION(BlueprintCallable, Category = "Gun")
    void StopFire();

    UFUNCTION(BlueprintCallable, Category = "Gun") 
    void FireOnce();   // semi-auto tap

    UFUNCTION(BlueprintNativeEvent) void FireEvent(); // BP can override
    virtual void FireEvent_Implementation();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

private:
    FTimerHandle FireTimer;
    void FireTick();

    double LastShotTime = -1.0;

    FORCEINLINE float ShotInterval() const
    {
        return (RoundsPerMinute > 0.f) ? (60.f / RoundsPerMinute) : 0.f;
    }
		
};



