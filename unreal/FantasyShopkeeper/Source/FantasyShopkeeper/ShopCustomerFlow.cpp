#include "ShopCustomerFlow.h"

#include "Engine/World.h"
#include "ShopCustomer.h"
#include "ShopTill.h"

AShopCustomerFlow::AShopCustomerFlow()
{
    PrimaryActorTick.bCanEverTick = true;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("CustomerFlowRoot"));
}

AShopCustomer* AShopCustomerFlow::GetActiveCustomer() const
{
    return IsValid(ActiveCustomer) ? ActiveCustomer.Get() : nullptr;
}

void AShopCustomerFlow::BeginPlay()
{
    Super::BeginPlay();
    if (IsValid(Till) && Till->GetWorld() == GetWorld() && IsValid(Till->Customer))
    {
        ActiveCustomer = Till->Customer;
        if (!CustomerClass)
        {
            CustomerClass = ActiveCustomer->GetClass();
        }
    }
}

bool AShopCustomerFlow::StartNextVisit()
{
    if (!HasAuthority() || !HasActorBegunPlay() || IsActorBeingDestroyed() || bStartingVisit
        || IsValid(ActiveCustomer) || !IsValid(Till) || Till->IsActorBeingDestroyed()
        || Till->GetWorld() != GetWorld() || IsValid(Till->Customer))
    {
        return false;
    }
    TGuardValue<bool> SpawnGuard(bStartingVisit, true);
    FShopCustomerOrder Order;
    const bool bHasOrder = !Orders.IsEmpty();
    if (bHasOrder)
    {
        Order = Orders[CompletedVisits % Orders.Num()];
        if (Order.ProductId.IsNone() || Order.PriceBits <= 0 || Order.TenderBits < Order.PriceBits)
        {
            return false;
        }
    }
    const TSubclassOf<AShopCustomer> VisitClass = Order.CustomerClass ? Order.CustomerClass : CustomerClass;
    if (!VisitClass || VisitClass->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists)
        || Entrance.ContainsNaN() || Checkout.ContainsNaN())
    {
        return false;
    }
    const FVector EntryLocation = GetActorTransform().TransformPosition(Entrance);
    const FVector CheckoutLocation = GetActorTransform().TransformPosition(Checkout);
    const FTransform SpawnTransform((CheckoutLocation - EntryLocation).Rotation(), EntryLocation);
    AShopCustomer* Customer = GetWorld()->SpawnActorDeferred<AShopCustomer>(VisitClass, SpawnTransform,
        nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding);
    if (!IsValid(Customer))
    {
        return false;
    }
    if (bHasOrder)
    {
        Customer->RequestedProductId = Order.ProductId;
        Customer->RequestedProductName = Order.ProductName;
        Customer->PriceBits = Order.PriceBits;
        Customer->TenderBits = Order.TenderBits;
    }
    Customer->FinishSpawning(SpawnTransform);
    if (!IsValid(Customer))
    {
        return false;
    }
    if (!Customer->HasActorBegunPlay())
    {
        Customer->DispatchBeginPlay();
    }
    if (!IsValid(Till) || IsActorBeingDestroyed() || !Customer->BeginVisit(CheckoutLocation)
        || !Till->AssignCustomer(Customer))
    {
        Customer->Destroy();
        return false;
    }
    ActiveCustomer = Customer;
    bOwnsActiveCustomer = true;
    bVisitCounted = false;
    WaitTime = 0;
    return true;
}

void AShopCustomerFlow::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!HasAuthority() || !IsValid(Till) || Till->GetWorld() != GetWorld())
    {
        return;
    }
    if (!IsValid(ActiveCustomer))
    {
        WaitTime += DeltaSeconds;
        if (WaitTime >= FMath::Max(0.f, VisitInterval))
        {
            WaitTime = 0;
            StartNextVisit();
        }
        return;
    }
    if (!ActiveCustomer->HasCompletedSale())
    {
        WaitTime = 0;
        return;
    }
    if (!bVisitCounted)
    {
        CompletedVisits = FMath::Min(CompletedVisits, MAX_int32 - 1) + 1;
        bVisitCounted = true;
    }
    if (ActiveCustomer->HasLeftShop())
    {
        AShopCustomer* Departed = ActiveCustomer;
        ActiveCustomer = nullptr;
        bOwnsActiveCustomer = false;
        if (Till->Customer == Departed)
        {
            Till->AssignCustomer(nullptr);
        }
        Departed->Destroy();
        WaitTime = 0;
        return;
    }
    if (ActiveCustomer->Activity == EShopCustomerActivity::Completed)
    {
        WaitTime += DeltaSeconds;
        if (WaitTime >= FMath::Max(0.f, DepartureDelay))
        {
            ActiveCustomer->BeginDeparture(GetActorTransform().TransformPosition(Entrance));
            WaitTime = 0;
        }
    }
}

void AShopCustomerFlow::EndPlay(const EEndPlayReason::Type Reason)
{
    if (bOwnsActiveCustomer && IsValid(ActiveCustomer))
    {
        if (IsValid(Till) && Till->Customer == ActiveCustomer)
        {
            Till->AssignCustomer(nullptr);
        }
        ActiveCustomer->Destroy();
    }
    Super::EndPlay(Reason);
}

FText AShopCustomerFlow::GetStatusText() const
{
    if (!IsValid(ActiveCustomer))
    {
        return NSLOCTEXT("Shop", "NextCustomerOnWay", "Next customer on the way");
    }
    if (ActiveCustomer->Activity == EShopCustomerActivity::Arriving)
    {
        return NSLOCTEXT("Shop", "CustomerEnteringStatus", "A customer is coming in");
    }
    if (ActiveCustomer->Activity == EShopCustomerActivity::ReadyToPay)
    {
        return NSLOCTEXT("Shop", "CustomerTillStatus", "Customer ready at the till");
    }
    if (ActiveCustomer->HasCompletedSale())
    {
        return NSLOCTEXT("Shop", "CustomerLeavingStatus", "Customer heading home");
    }
    return FText::Format(NSLOCTEXT("Shop", "CustomerOrderStatus", "Customer wants {0} ({1} bits)"),
        ActiveCustomer->RequestedProductName, FText::AsNumber(ActiveCustomer->PriceBits));
}
