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

FText AShopSupplyCrate::GetBlockReason() const
{
    if (!IsValid(Station) || Station->GetWorld() != GetWorld() || PriceBits <= 0 || Delivery.IsEmpty())
    {
        return NSLOCTEXT("Shop", "SupplyNotConfigured", "Supply crate needs a brewing station and a valid delivery");
    }
    if (!Station->CanReceiveIngredients(Delivery))
    {
        return NSLOCTEXT("Shop", "SupplyCapacity", "Ingredient storage is full or unavailable");
    }
    const AShopGameState* State = GetWorld()->GetGameState<AShopGameState>();
    if (!State || State->GetCash() < PriceBits)
    {
        return FText::Format(NSLOCTEXT("Shop", "SupplyTooExpensive", "Supplies cost {0} bits | Earn money at the till"), FText::AsNumber(PriceBits));
    }
    return FText::GetEmpty();
}

bool AShopSupplyCrate::CycleOffer(UShopInteractionComponent* Interactor)
{
    if (!HasAuthority() || !HasActorBegunPlay() || IsActorBeingDestroyed() || !IsValid(Interactor)
        || Interactor->GetHeldItem() || !Interactor->CanReach(this))
    {
        return false;
    }
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
    UGameplayStatics::PlaySoundAtLocation(this, DeliverySound, GetActorLocation(), 0.3f);
    return true;
}

FText AShopSupplyCrate::GetInteractionPrompt(UShopInteractionComponent* Interactor) const
{
    if (IsValid(Interactor) && Interactor->GetHeldItem())
    {
        return NSLOCTEXT("Shop", "SupplyHandsFull", "Put down the held item to buy supplies");
    }
    const FText BlockReason = GetBlockReason();
    FText ChoiceHint;
    if (Offers.Num() > 1)
    {
        ChoiceHint = NSLOCTEXT("Shop", "NextSupplyPack", " | R  Next supply pack");
    }
    if (!BlockReason.IsEmpty())
    {
        return FText::Format(NSLOCTEXT("Shop", "BlockedSupplyChoice", "{0}{1}"), BlockReason, ChoiceHint);
    }
    FString Contents;
    TArray<FName> Keys;
    Delivery.GetKeys(Keys);
    Keys.Sort(FNameLexicalLess());
    for (const FName Key : Keys)
    {
        Contents += FString::Printf(TEXT(" | %s +%d"), *Key.ToString(), Delivery.FindChecked(Key));
    }
    return FText::Format(NSLOCTEXT("Shop", "BuySupplies", "E  Buy supplies ({0} bits){1}{2}"),
        FText::AsNumber(PriceBits), FText::FromString(Contents), ChoiceHint);
}
