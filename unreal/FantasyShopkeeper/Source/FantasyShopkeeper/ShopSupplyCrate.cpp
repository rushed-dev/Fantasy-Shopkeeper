#include "ShopSupplyCrate.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "ShopBrewingStation.h"
#include "ShopGameState.h"
#include "ShopInteractionComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

AShopSupplyCrate::AShopSupplyCrate()
{
    FShopSupplyOffer Bundle;
    Bundle.Ingredients = Delivery;
    Offers.Add(Bundle);
    FShopSupplyOffer Leaves;
    Leaves.PriceBits = 6;
    Leaves.Ingredients = {{TEXT("Moonleaf"), 6}};
    Offers.Add(Leaves);
    FShopSupplyOffer Water;
    Water.PriceBits = 6;
    Water.Ingredients = {{TEXT("Spring Water"), 3}};
    Offers.Add(Water);
    PrimaryActorTick.bCanEverTick = false;
    CrateMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CrateMesh"));
    RootComponent = CrateMesh;
    CrateMesh->SetCollisionProfileName(TEXT("BlockAll"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (Cube.Succeeded())
    {
        CrateMesh->SetStaticMesh(Cube.Object);
        CrateMesh->SetRelativeScale3D(FVector(0.6, 0.45, 0.4));
    }
}

bool AShopSupplyCrate::GetOrder(TMap<FName, int32>& Quantities, int64& Cost) const
{
    Quantities.Empty();
    Cost = 0;
    if (PackCount < 0 || PackCount > 10 || PriceBits <= 0 || Delivery.IsEmpty()) return false;
    for (const auto& Entry : Delivery)
    {
        const int64 Amount = int64(Entry.Value) * PackCount;
        if (Entry.Key.IsNone() || Entry.Value <= 0 || Amount > MAX_int32) return false;
        if (PackCount > 0) Quantities.Add(Entry.Key, int32(Amount));
    }
    Cost = int64(PriceBits) * PackCount;
    return true;
}

bool AShopSupplyCrate::ChangeQuantity(UShopInteractionComponent* Interactor, int32 Delta)
{
    const AShopGameState* State = GetWorld()->GetGameState<AShopGameState>();
    if (!HasAuthority() || !State || !State->CanOrderSupplies() || !IsValid(Interactor)
        || Interactor->GetHeldItem() || !Interactor->CanReach(this) || (Delta != 1 && Delta != -1)) return false;
    const int32 Next = FMath::Clamp(PackCount + Delta, 0, 10);
    if (Next == PackCount) return false;
    PackCount = Next;
    return true;
}

FText AShopSupplyCrate::GetBlockReason() const
{
    const AShopGameState* State = GetWorld()->GetGameState<AShopGameState>();
    if (!State || !State->IsDayInitialized() || State->GetDayPhase() != EShopDayPhase::Evening)
        return FText::FromString(TEXT("Order in the evening | Paid supplies arrive next morning"));
    if (State->HasConfirmedSupplies()) return FText::FromString(TEXT("Order confirmed | Delivery next morning"));
    if (PackCount == 0) return FText::GetEmpty();
    TMap<FName, int32> Quantities;
    int64 Cost = 0;
    if (!IsValid(Station) || Station->GetWorld() != GetWorld() || !GetOrder(Quantities, Cost))
        return FText::FromString(TEXT("Supply order is not configured correctly"));
    if (!Station->CanReceiveIngredients(Quantities))
        return FText::FromString(TEXT("Order exceeds ingredient storage | Reduce quantity"));
    if (State->GetCash() < Cost) return FText::FromString(TEXT("Not enough cash for this order | Reduce quantity"));
    return FText::GetEmpty();
}

bool AShopSupplyCrate::CycleOffer(UShopInteractionComponent* Interactor)
{
    if (!HasAuthority() || !HasActorBegunPlay() || IsActorBeingDestroyed() || !IsValid(Interactor)
        || Interactor->GetHeldItem() || !Interactor->CanReach(this))
    {
        return false;
    }
    const AShopGameState* State = GetWorld()->GetGameState<AShopGameState>();
    if (!State || !State->CanOrderSupplies()) return false;
    int32 Index = Offers.IndexOfByPredicate([this](const FShopSupplyOffer& Offer)
    {
        return Offer.PriceBits == PriceBits && Offer.Ingredients.OrderIndependentCompareEqual(Delivery);
    });
    for (int32 Attempt = 0; Attempt < Offers.Num(); ++Attempt)
    {
        Index = (Index + 1) % Offers.Num();
        const FShopSupplyOffer& Offer = Offers[Index];
        if (Offer.PriceBits <= 0 || Offer.Ingredients.IsEmpty())
        {
            continue;
        }
        bool bValid = true;
        for (const TPair<FName, int32>& Entry : Offer.Ingredients)
        {
            bValid = bValid && !Entry.Key.IsNone() && Entry.Value > 0;
        }
        if (bValid && (Offer.PriceBits != PriceBits || !Offer.Ingredients.OrderIndependentCompareEqual(Delivery)))
        {
            PriceBits = Offer.PriceBits;
            Delivery = Offer.Ingredients;
            return true;
        }
    }
    return false;
}

bool AShopSupplyCrate::Interact_Implementation(UShopInteractionComponent* Interactor)
{
    if (!HasAuthority() || !HasActorBegunPlay() || IsActorBeingDestroyed() || !IsValid(Interactor)
        || Interactor->GetHeldItem() || !Interactor->CanReach(this) || !GetBlockReason().IsEmpty())
    {
        return false;
    }
    AShopGameState* State = GetWorld()->GetGameState<AShopGameState>();
    if (!State || !State->TryBuySupplies(this))
    {
        return false;
    }
    // Delivery audio plays next morning, after the checkpoint commits.
    return true;
}

FText AShopSupplyCrate::GetInteractionPrompt(UShopInteractionComponent* Interactor) const
{
    if (IsValid(Interactor) && Interactor->GetHeldItem())
    {
        return NSLOCTEXT("Shop", "SupplyHandsFull", "Put down the held item to buy supplies");
    }
    const AShopGameState* State = GetWorld()->GetGameState<AShopGameState>();
    const FText BlockReason = GetBlockReason();
    if (!State || State->GetDayPhase() != EShopDayPhase::Evening || State->HasConfirmedSupplies()) return BlockReason;
    TMap<FName, int32> Quantities;
    int64 Cost = 0;
    GetOrder(Quantities, Cost);
    FString Contents;
    TArray<FName> Keys;
    Delivery.GetKeys(Keys);
    Keys.Sort(FNameLexicalLess());
    for (const FName Key : Keys)
        Contents += FString::Printf(TEXT(" %s %d/%d (+%lld)"), *Key.ToString(),
            IsValid(Station) ? Station->GetIngredientQuantity(Key) : 0,
            IsValid(Station) ? Station->IngredientCapacity : 0, int64(Delivery[Key]) * PackCount);
    const FString Action = BlockReason.IsEmpty()
        ? (PackCount == 0 ? TEXT("E Confirm no order") : TEXT("E Pay for tomorrow")) : BlockReason.ToString();
    return FText::FromString(FString::Printf(TEXT("%s | %d packs / %lld bits\n%s\nR Pack | F Less | T More"),
        *Action, PackCount, Cost, *Contents));
}
