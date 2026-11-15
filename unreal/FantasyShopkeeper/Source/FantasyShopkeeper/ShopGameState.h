#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "ShopGameState.generated.h"

class AShopCustomer;
class AShopPickupItem;
class AShopTill;
class AShopSupplyCrate;

UCLASS(Blueprintable)
class FANTASYSHOPKEEPER_API AShopGameState : public AGameStateBase
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category = "Shop|Money")
    int64 GetCash() const { return Cash; }

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shop|Money", meta = (ClampMin = "0"))
    int64 StartingCash = 0;

protected:
    virtual void BeginPlay() override;

private:
    friend class AShopTill;
    friend class AShopSupplyCrate;
    bool TryCompleteSale(AShopTill* Till);
    bool TryBuySupplies(AShopSupplyCrate* Crate);

    UPROPERTY(Transient)
    int64 Cash = 0;

    bool bChangingEconomy = false;
};
