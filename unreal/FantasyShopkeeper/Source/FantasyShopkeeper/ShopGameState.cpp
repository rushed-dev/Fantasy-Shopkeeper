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

bool AShopGameState::TryBuySupplies(AShopSupplyCrate* Crate)
{
    if (bChangingEconomy || !HasAuthority() || !IsValid(Crate) || Crate->IsActorBeingDestroyed()
        || Crate->GetWorld() != GetWorld() || Crate->PriceBits <= 0 || Cash < Crate->PriceBits
        || !IsValid(Crate->Station) || Crate->Station->GetWorld() != GetWorld()
        || !Crate->Station->CanReceiveIngredients(Crate->Delivery))
    {
        return false;
    }
    TGuardValue<bool> TransactionGuard(bChangingEconomy, true);
    Crate->Station->ReceiveIngredients(Crate->Delivery);
    Cash -= Crate->PriceBits;
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
        || Customer->PriceBits <= 0 || Cash > MAX_int64 - Customer->PriceBits
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
    Customer->PaidBits = Payment;
    Customer->Activity = EShopCustomerActivity::Completed;
    Till->Phase = EShopTillPhase::Complete;
    if (IsValid(Customer))
    {
        Customer->FinishPurchasePresentation(true);
    }
    return true;
}
