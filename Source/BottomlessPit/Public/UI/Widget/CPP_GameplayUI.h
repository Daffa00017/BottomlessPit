#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CPP_GameplayUI.generated.h"

UCLASS()
class BOTTOMLESSPIT_API UCPP_GameplayUI : public UUserWidget
{
    GENERATED_BODY()

	void TickWhilePaused(float DeltaTime);

protected:
	
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
};


