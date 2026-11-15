#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ShopGameMode.generated.h"

UCLASS(Blueprintable)
class FANTASYSHOPKEEPER_API AShopGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AShopGameMode();
    virtual void StartPlay() override;
};
