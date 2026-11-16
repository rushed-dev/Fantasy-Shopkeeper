#include "ShopCustomerFlow.h"

#include "Engine/World.h"
#include "ShopCustomer.h"
#include "ShopTill.h"
#include "ShopGameState.h"

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
        Customers.Add(ActiveCustomer);
        if (!CustomerClass)
        {
            CustomerClass = ActiveCustomer->GetClass();
        }
    }
}

bool AShopCustomerFlow::StartNextVisit()
{
    const AShopGameState* State = GetWorld()->GetGameState<AShopGameState>();
    if (State && State->IsDayInitialized() && (!State->CanStartVisit() || StartedVisits >= VisitsPerDay))
    {
        return false;
    }
    if (!HasAuthority() || !HasActorBegunPlay() || IsActorBeingDestroyed() || bStartingVisit
        || GetCustomersInShop() >= FMath::Clamp(MaxConcurrentCustomers, 1, 3) || !IsValid(Till) || Till->IsActorBeingDestroyed()
        || Till->GetWorld() != GetWorld() || (IsValid(Till->Customer) && !Customers.Contains(Till->Customer)))
    {
        return false;
    }
    TGuardValue<bool> SpawnGuard(bStartingVisit, true);
    FShopCustomerOrder Order;
    const bool bHasOrder = !Orders.IsEmpty();
    if (bHasOrder)
    {
        Order = Orders[StartedVisits % Orders.Num()];
        if (Order.ProductId.IsNone() || Order.PriceBits <= 0 || Order.TenderBits < Order.PriceBits)
        {
            return false;
        }
    }
    const TSubclassOf<AShopCustomer> VisitClass = Order.CustomerClass ? Order.CustomerClass : CustomerClass;
    if (!VisitClass || VisitClass->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists)
        || Entrance.ContainsNaN() || Checkout.ContainsNaN() || TillCheckoutOffset.ContainsNaN())
    {
        return false;
    }
    const FVector EntryLocation = GetActorTransform().TransformPosition(Entrance);
    const FVector CheckoutLocation = GetQueuePosition(Customers.Num());
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
    Customer->ConfigureVisitPersonality((State ? State->GetDay() - 1 : 0) % 3 + StartedVisits % 3);
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
        || (Customers.IsEmpty() && !Till->AssignCustomer(Customer)))
    {
        Customer->Destroy();
        return false;
    }
    Customers.Add(Customer);
    ++StartedVisits;
    RefreshQueue();
    WaitTime = 0;
    return true;
}

void AShopCustomerFlow::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!HasAuthority() || !IsValid(Till) || Till->GetWorld() != GetWorld()) return;
    bool bRemoved = false;
    for (int32 Index = Customers.Num() - 1; Index >= 0; --Index)
    {
        AShopCustomer* Customer = Customers[Index];
        if (IsValid(Customer) && Customer->HasResolvedVisit() && !CountedVisits.Contains(Customer))
        {
            ++CompletedVisits;
            CountedVisits.Add(Customer);
        }
        if (!IsValid(Customer) || Customer->HasLeftShop())
        {
            if (Till->Customer == Customer) Till->AssignCustomer(nullptr);
            Customers.RemoveAt(Index);
            bRemoved |= IsValid(Customer);
            DepartureTimes.Remove(Customer);
            CountedVisits.Remove(Customer);
            if (IsValid(Customer)) Customer->Destroy();
            continue;
        }
        if (!Customer->HasResolvedVisit()) continue;
        if (Customer->Activity == EShopCustomerActivity::Completed)
        {
            float& Time = DepartureTimes.FindOrAdd(Customer);
            Time += FMath::Max(0.f, DeltaSeconds);
            if (Time >= FMath::Max(0.f, DepartureDelay) * Customer->GetDeparturePace())
                Customer->BeginDeparture(GetActorTransform().TransformPosition(Entrance));
        }
    }
    RefreshQueue();
    if (AShopGameState* State = GetWorld()->GetGameState<AShopGameState>()) State->UpdateDay();
    if (bRemoved) return;
    if (GetCustomersInShop() < FMath::Clamp(MaxConcurrentCustomers, 1, 3))
    {
        WaitTime += FMath::Max(0.f, DeltaSeconds);
        if (WaitTime >= FMath::Max(0.f, VisitInterval))
        {
            WaitTime = 0;
            StartNextVisit();
        }
    }
}

int32 AShopCustomerFlow::GetCustomersInShop() const
{
    int32 Count = 0;
    for (const AShopCustomer* Customer : Customers) if (IsValid(Customer)) ++Count;
    return Count;
}

AShopCustomer* AShopCustomerFlow::GetCustomerAt(int32 Index) const
{
    return Customers.IsValidIndex(Index) && IsValid(Customers[Index]) ? Customers[Index].Get() : nullptr;
}

AShopCustomer* AShopCustomerFlow::GetSpeakingCustomer() const
{
    if (GetActiveCustomer() && ActiveCustomer->HasCompletedSale() && !ActiveCustomer->GetSpokenLine().IsEmpty()) return ActiveCustomer;
    for (int32 Index = Customers.Num() - 1; Index >= 0; --Index)
        if (GetCustomerAt(Index) && !Customers[Index]->GetSpokenLine().IsEmpty()) return Customers[Index];
    return nullptr;
}

FVector AShopCustomerFlow::GetQueuePosition(int32 Index) const
{
    const FVector Offset(0, FMath::Max(80.f, QueueSpacing) * Index, 0);
    return bCheckoutFollowsTill && IsValid(Till)
        ? Till->GetActorTransform().TransformPosition(TillCheckoutOffset + Offset)
        : GetActorTransform().TransformPosition(Checkout + Offset);
}

void AShopCustomerFlow::RefreshQueue()
{
    ActiveCustomer = GetCustomerAt(0);
    if (IsValid(Till) && Till->Customer != ActiveCustomer
        && (!IsValid(Till->Customer) || Customers.Contains(Till->Customer))) Till->AssignCustomer(ActiveCustomer);
    for (int32 Index = 0; Index < Customers.Num(); ++Index)
        if (IsValid(Customers[Index])) Customers[Index]->SetQueueDestination(GetQueuePosition(Index), Index == 0);
}

bool AShopCustomerFlow::EndUnservedVisit()
{
    return HasAuthority() && IsValid(ActiveCustomer)
        && ActiveCustomer->LeaveWithoutPurchase(GetActorTransform().TransformPosition(Entrance));
}

void AShopCustomerFlow::ResetMorning()
{
    if (IsValid(Till))
    {
        Till->AssignCustomer(nullptr);
    }
    for (AShopCustomer* Customer : Customers) if (IsValid(Customer)) Customer->Destroy();
    Customers.Empty();
    CountedVisits.Empty();
    DepartureTimes.Empty();
    ActiveCustomer = nullptr;
    CompletedVisits = StartedVisits = 0;
    WaitTime = 0;
}

void AShopCustomerFlow::EndPlay(const EEndPlayReason::Type Reason)
{
    ResetMorning();
    Super::EndPlay(Reason);
}

FText AShopCustomerFlow::GetStatusText() const
{
    if (!IsValid(ActiveCustomer))
    {
        const AShopGameState* State = GetWorld()->GetGameState<AShopGameState>();
        if (State && State->IsDayInitialized() && !State->CanStartVisit())
        {
            return FText::GetEmpty();
        }
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
    if (ActiveCustomer->HasResolvedVisit())
    {
        return NSLOCTEXT("Shop", "CustomerLeavingStatus", "Customer heading home");
    }
    return FText::Format(NSLOCTEXT("Shop", "CustomerOrderStatus", "Customer wants {0} ({1} bits) | X: End visit before pickup"),
        ActiveCustomer->RequestedProductName, FText::AsNumber(ActiveCustomer->PriceBits));
}
