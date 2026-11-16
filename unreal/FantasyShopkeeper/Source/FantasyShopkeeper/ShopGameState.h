#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "ShopGameState.generated.h"

class AShopCustomer;
class AShopPickupItem;
class AShopTill;
class AShopSupplyCrate;
class AShopBrewingStation;
class UShopMorningSave;

UENUM(BlueprintType)
enum class EShopDayPhase : uint8 { Preparation, Trading, Closing, Evening };

UCLASS(Blueprintable)
class FANTASYSHOPKEEPER_API AShopGameState : public AGameStateBase
{
    GENERATED_BODY()

public:
    void InitializeDay(const FString& SavePath);
    bool OpenShop();
    bool CloseShop();
    bool AdvanceMorning();
    bool RestoreMorning();
    void UpdateDay();
    bool CanStartVisit() const;
    bool ConfirmNoSupplies();
    bool CanReceiveOrder(AShopBrewingStation* Station, const TMap<FName, int32>& Additional) const;
    int32 GetPendingIngredientQuantity(FName Ingredient) const { return PendingDelivery.FindRef(Ingredient); }
    bool HasConfirmedSupplies() const { return bSuppliesConfirmed; }
    bool CanOrderSupplies() const { return bDayInitialized && DayPhase == EShopDayPhase::Evening && !bChangingEconomy; }
    bool IsDayInitialized() const { return bDayInitialized; }
    bool IsChangingEconomy() const { return bChangingEconomy; }
    EShopDayPhase GetDayPhase() const { return DayPhase; }
    int32 GetDay() const { return Day; }
    int64 GetTakings() const { return Takings; }
    int64 GetSupplySpending() const { return SupplySpending; }
    FString GetDayText() const;
    FString GetCheckpointMessage() const { return CheckpointMessage; }

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
    bool bDayInitialized = false;
    bool bCheckpointBlocked = false;
    bool bSuppliesConfirmed = false;
    UPROPERTY(Transient) TWeakObjectPtr<AShopBrewingStation> DeliveryStation;
    UPROPERTY(Transient) TMap<FName, int32> PendingDelivery;
    EShopDayPhase DayPhase = EShopDayPhase::Preparation;
    int32 Day = 1;
    int64 Takings = 0;
    int64 SupplySpending = 0;
    FString CheckpointPath;
    FString CheckpointMessage;
    UShopMorningSave* CaptureMorning(int32 MorningDay) const;
    bool ValidateMorning(const UShopMorningSave* Save) const;
    bool WriteMorning(UShopMorningSave* Save);
};
