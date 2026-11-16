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
    CheckpointMessage = TEXT("No supplies ordered. N: Next morning.");
    return true;
}

bool AShopGameState::TryBuySupplies(AShopSupplyCrate* Crate)
{
    if (!HasAuthority() || !CanOrderSupplies() || !IsValid(Crate) || Crate->IsActorBeingDestroyed()
        || Crate->GetWorld() != GetWorld()) return false;
    if (Crate->GetPackCount() == 0) return ConfirmNoSupplies();
    TMap<FName, int32> Delivery;
    int64 Cost = 0;
    if (!Crate->GetOrder(Delivery, Cost) || Cash < Cost || SupplySpending > MAX_int64 - Cost
        || !IsValid(Crate->Station) || Crate->Station->GetWorld() != GetWorld()
        || !Crate->Station->CanReceiveIngredients(Delivery)) return false;
    TGuardValue<bool> TransactionGuard(bChangingEconomy, true);
    // Snapshot the paid order; later changes to the crate cannot change a delivery.
    PendingDelivery = Delivery;
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
        || Item->GetCarrier() != Customer->ShopInteraction || Customer->ShopInteraction->GetHeldItem() != Item
        || Customer->Activity != EShopCustomerActivity::ReadyToPay || Item->GetCurrentSlot())
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
