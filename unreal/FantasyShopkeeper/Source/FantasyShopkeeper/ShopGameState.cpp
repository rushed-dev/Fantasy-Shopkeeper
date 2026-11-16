#include "ShopGameState.h"

#include "ShopCustomer.h"
#include "ShopPickupItem.h"
#include "ShopShelfSlot.h"
#include "ShopInteractionComponent.h"
#include "ShopTill.h"
#include "ShopSupplyCrate.h"
#include "ShopBrewingStation.h"

void AShopGameState::BeginPlay()
{
    Super::BeginPlay();
    Cash = FMath::Max<int64>(0, StartingCash);
}

bool AShopGameState::ConfirmNoSupplies()
{
    if (!HasAuthority() || !CanOrderSupplies()) return false;
    bSuppliesConfirmed = true;
    CheckpointMessage = TEXT("No additional supplies ordered. Rest at the order book.");
    return true;
}

bool AShopGameState::CanReceiveOrder(AShopBrewingStation* Station, const TMap<FName, int32>& Additional) const
{
    if (!IsValid(Station) || Station->GetWorld() != GetWorld()
        || (!PendingDelivery.IsEmpty() && DeliveryStation.Get() != Station)) return false;
    TMap<FName, int32> Combined = PendingDelivery;
    if (!MorningDelivery.IsEmpty()) return false;
    for (const auto& Entry : Additional)
    {
        if (Entry.Value <= 0 || int64(Combined.FindRef(Entry.Key)) + Entry.Value > MAX_int32) return false;
        Combined.FindOrAdd(Entry.Key) += Entry.Value;
    }
    return Station->CanReceiveIngredients(Combined);
}

bool AShopGameState::TryBuySupplies(AShopSupplyCrate* Crate)
{
    if (!HasAuthority() || !CanOrderSupplies() || !IsValid(Crate) || Crate->IsActorBeingDestroyed()
        || Crate->GetWorld() != GetWorld() || Crate->bOrderSubmitted) return false;
    if (Crate->GetPackCount() == 0)
    {
        if (!ConfirmNoSupplies()) return false;
        Crate->bOrderSubmitted = true;
        return true;
    }
    TMap<FName, int32> Delivery;
    int64 Cost = 0;
    if (!Crate->GetOrder(Delivery, Cost) || Cash < Cost || SupplySpending > MAX_int64 - Cost
        || !IsValid(Crate->Station) || Crate->Station->GetWorld() != GetWorld()
        || !CanReceiveOrder(Crate->Station, Delivery)) return false;
    TGuardValue<bool> TransactionGuard(bChangingEconomy, true);
    // Snapshot the paid order; later changes to the crate cannot change a delivery.
    for (const auto& Entry : Delivery) PendingDelivery.FindOrAdd(Entry.Key) += Entry.Value;
    Crate->bOrderSubmitted = true;
    DeliveryStation = Crate->Station;
    Cash -= Cost;
    SupplySpending += Cost;
    bSuppliesConfirmed = true;
    CheckpointMessage = TEXT("Order paid. Supplies arrive next morning.");
    return true;
}

bool AShopGameState::TryCompleteSale(AShopTill* Till)
{
    if (!IsValid(Till) || Till->GetWorld() != GetWorld() || Till->IsActorBeingDestroyed() || !Till->CanCommitSale())
    {
        return false;
    }
    AShopCustomer* Customer = Till->QuotedCustomer.Get();
    AShopPickupItem* Item = Till->QuotedItem.Get();
    if (bChangingEconomy || !HasAuthority() || !IsValid(Customer) || Customer->GetWorld() != GetWorld()
        || Customer->IsActorBeingDestroyed() || Customer->HasCompletedSale() || Customer->RequestedProductId.IsNone()
        || Customer->PriceBits <= 0 || Cash > MAX_int64 - Customer->PriceBits || Takings > MAX_int64 - Customer->PriceBits
        || !IsValid(Item) || Item->IsActorBeingDestroyed() || Item->GetWorld() != GetWorld()
        || Item->ProductId != Customer->RequestedProductId
        || Item->GetCarrier() || Customer->CounterItem.Get() != Item
        || Customer->Activity != EShopCustomerActivity::ReadyToPay || !Item->GetCurrentSlot()
        || Item->GetCurrentSlot()->GetCheckoutTill() != Till)
    {
        return false;
    }
    // Destruction callbacks can attempt another economic action before cash commits.
    TGuardValue<bool> SaleGuard(bChangingEconomy, true);
    const int32 Payment = Till->QuotedPrice;
    Customer->PreparePurchasePresentation(Item);
    if (!Item->Destroy())
    {
        if (IsValid(Customer))
        {
            Customer->FinishPurchasePresentation(false);
        }
        return false;
    }
    Cash += Payment;
    Takings += Payment;
    Customer->PaidBits = Payment;
    Customer->Activity = EShopCustomerActivity::Completed;
    Till->Phase = EShopTillPhase::Complete;
    if (IsValid(Customer))
    {
        Customer->FinishPurchasePresentation(true);
    }
    return true;
}


bool AShopGameState::TryBuyStorageUpgrade(AShopBrewingStation* Station)
{
    if (!HasAuthority() || !CanOrderSupplies() || !IsValid(Station) || Station->IsActorBeingDestroyed()
        || Station->GetWorld() != GetWorld() || Station->HasStorageUpgrade() || Station->bBrewing
        || Station->CapacityForUpgrade(true) <= 0 || Cash < StorageUpgradePrice
        || UpgradeSpending > MAX_int64 - StorageUpgradePrice) return false;
    TGuardValue<bool> Guard(bChangingEconomy, true);
    Cash -= StorageUpgradePrice;
    UpgradeSpending += StorageUpgradePrice;
    Station->bStorageUpgraded = true;
    CheckpointMessage = TEXT("Extra ingredient drawers fitted. Saved with the next morning.");
    return true;
}

bool AShopGameState::TryBuyBatchUpgrade(AShopBrewingStation* Station)
{
    if (!HasAuthority() || !CanOrderSupplies() || !IsValid(Station) || Station->IsActorBeingDestroyed()
        || Station->GetWorld() != GetWorld() || Station->HasBatchUpgrade() || Station->bBrewing
        || !IsValid(Station->GetBatchOutput(1)) || !IsValid(Station->GetBatchOutput(2))
        || Cash < BatchUpgradePrice || UpgradeSpending > MAX_int64 - BatchUpgradePrice) return false;
    TGuardValue<bool> Guard(bChangingEconomy, true);
    Cash -= BatchUpgradePrice;
    UpgradeSpending += BatchUpgradePrice;
    Station->bBatchUpgraded = true;
    Station->BatchSize = 3;
    Station->RefreshBatchOutputs();
    CheckpointMessage = TEXT("Batch bottling kit fitted. Saved with the next morning.");
    return true;
}
