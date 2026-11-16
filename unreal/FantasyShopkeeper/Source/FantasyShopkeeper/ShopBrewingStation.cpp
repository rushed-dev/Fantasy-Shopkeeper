#include "ShopBrewingStation.h"

#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "ShopBrewingRecipe.h"
#include "ShopGameState.h"
#include "ShopInteractionComponent.h"
#include "ShopPickupItem.h"
#include "ShopShelfSlot.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

AShopBrewingStation::AShopBrewingStation()
{
    PrimaryActorTick.bCanEverTick = false;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("BenchRoot"));
    VesselMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VesselMesh"));
    VesselMesh->SetupAttachment(RootComponent);
    VesselMesh->SetRelativeLocation(FVector(0, 0, 12));
    VesselMesh->SetRelativeScale3D(FVector(0.45, 0.45, 0.24));
    VesselMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    VesselMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    VesselMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    VesselMesh->SetGenerateOverlapEvents(false);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (Cylinder.Succeeded())
    {
        VesselMesh->SetStaticMesh(Cylinder.Object);
        for (int32 Index = 0; Index < 2; ++Index)
        {
            UStaticMeshComponent* Container = CreateDefaultSubobject<UStaticMeshComponent>(Index == 0 ? TEXT("MoonleafContainer") : TEXT("WaterContainer"));
            if (Index == 0)
            {
                MoonleafContainer = Container;
            }
            else
            {
                WaterContainer = Container;
            }
            Container->SetupAttachment(RootComponent);
            Container->SetStaticMesh(Cylinder.Object);
            Container->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Container->SetRelativeLocation(FVector(Index == 0 ? -36 : 36, 0, 10));
            Container->SetRelativeScale3D(FVector(0.16, 0.16, 0.2));
        }
    }
}

void AShopBrewingStation::BeginPlay()
{
    Super::BeginPlay();
    Ingredients = StartingIngredients;
    // Stable identities exist before checkpoint restoration, even before purchase.
    for (int32 Index = 0; Index < 2; ++Index)
    {
        FActorSpawnParameters Params;
        Params.Name = FName(*FString::Printf(TEXT("%s_BatchOutput%d"), *GetName(), Index + 1));
        Params.Owner = this;
        AShopShelfSlot* Slot = GetWorld()->SpawnActor<AShopShelfSlot>(
            GetActorTransform().TransformPosition(FVector(Index == 0 ? -18 : 18, -42, 0)), GetActorRotation(), Params);
        if (Slot)
        {
            Slot->bAvailableForSale = false;
            Slot->InteractionBox->SetBoxExtent(FVector(12, 12, 14));
            Slot->SlotMarker->SetRelativeScale3D(FVector(0.24, 0.24, 0.01));
        }
        ExtraOutputs.Add(Slot);
    }
    RefreshBatchOutputs();
    if (IsValid(OutputSlot))
    {
        OutputSlot->bAvailableForSale = false;
    }
}

int32 AShopBrewingStation::GetIngredientQuantity(FName IngredientId) const
{
    return Ingredients.FindRef(IngredientId);
}

bool AShopBrewingStation::CanReceiveIngredients(const TMap<FName, int32>& Delivery) const
{
    if (!HasAuthority() || !HasActorBegunPlay() || IsActorBeingDestroyed() || bBrewing
        || Delivery.IsEmpty() || GetIngredientCapacity() <= 0)
    {
        return false;
    }
    for (const TPair<FName, int32>& Entry : Delivery)
    {
        const int32 Current = GetIngredientQuantity(Entry.Key);
        if (Entry.Key.IsNone() || !StartingIngredients.Contains(Entry.Key) || Entry.Value <= 0 || Current < 0
            || int64(Current) + Entry.Value > GetIngredientCapacity())
        {
            return false;
        }
    }
    return true;
}

void AShopBrewingStation::ReceiveIngredients(const TMap<FName, int32>& Delivery)
{
    for (const TPair<FName, int32>& Entry : Delivery)
    {
        Ingredients.FindOrAdd(Entry.Key) += Entry.Value;
    }
}

FText AShopBrewingStation::GetBlockReason() const
{
    if (!IsValid(Recipe) || !Recipe->IsRecipeValid())
    {
        return NSLOCTEXT("Shop", "InvalidRecipe", "No valid recipe configured");
    }
    if (!IsValid(OutputSlot) || OutputSlot->GetWorld() != GetWorld())
    {
        return NSLOCTEXT("Shop", "MissingBrewOutput", "No output slot configured");
    }
    for (int32 Index = 0; Index < (bBatchUpgraded ? 3 : 1); ++Index)
    {
        const AShopShelfSlot* Slot = GetBatchOutput(Index);
        if (!IsValid(Slot) || Slot->GetWorld() != GetWorld() || Slot->IsActorBeingDestroyed() || Slot->GetOccupyingItem())
            return NSLOCTEXT("Shop", "BrewOutputBlocked", "Clear all brewing output positions");
    }
    for (const TPair<FName, int32>& Cost : Recipe->IngredientCosts)
    {
        if (int64(GetIngredientQuantity(Cost.Key)) < int64(Cost.Value) * BatchSize)
        {
            return FText::Format(NSLOCTEXT("Shop", "MissingIngredient", "Need {0} ({1}/{2})"),
                FText::FromName(Cost.Key), FText::AsNumber(GetIngredientQuantity(Cost.Key)), FText::AsNumber(int64(Cost.Value) * BatchSize));
        }
    }
    return FText::GetEmpty();
}

FText AShopBrewingStation::GetInteractionPrompt(UShopInteractionComponent* Interactor) const
{
    if (IsValid(Interactor) && Interactor->GetHeldItem())
    {
        return NSLOCTEXT("Shop", "BrewHandsFull", "Put down the held item to brew");
    }
    const FText BlockReason = GetBlockReason();
    FText RecipeHint;
    if (AvailableRecipes.Num() > 1)
    {
        RecipeHint = NSLOCTEXT("Shop", "NextRecipeHint", " | R  Next recipe");
    }
    if (!BlockReason.IsEmpty())
    {
        const FText Name = IsValid(Recipe) ? Recipe->DisplayName : FText::GetEmpty();
        return FText::Format(NSLOCTEXT("Shop", "BlockedRecipePrompt", "{0} | {1}{2}"), Name, BlockReason, RecipeHint);
    }
    FString Stock;
    for (const TPair<FName, int32>& Cost : Recipe->IngredientCosts)
    {
        Stock += FString::Printf(TEXT(" | %s %d/%d"), *Cost.Key.ToString(), GetIngredientQuantity(Cost.Key), Cost.Value * BatchSize);
    }
    return FText::Format(NSLOCTEXT("Shop", "BrewPrompt", "E  Brew {3} x {0}{1}{2}"), Recipe->DisplayName, FText::FromString(Stock), RecipeHint, FText::AsNumber(BatchSize));
}

bool AShopBrewingStation::CycleRecipe(UShopInteractionComponent* Interactor)
{
    if (!HasAuthority() || !HasActorBegunPlay() || IsActorBeingDestroyed() || bBrewing
        || !IsValid(Interactor) || Interactor->GetHeldItem() || !Interactor->CanReach(this))
    {
        return false;
    }
    int32 Index = AvailableRecipes.IndexOfByKey(Recipe);
    for (int32 Attempt = 0; Attempt < AvailableRecipes.Num(); ++Attempt)
    {
        Index = (Index + 1) % AvailableRecipes.Num();
        UShopBrewingRecipe* Candidate = AvailableRecipes[Index];
        if (IsValid(Candidate) && Candidate != Recipe && Candidate->IsRecipeValid())
        {
            Recipe = Candidate;
            return true;
        }
    }
    return false;
}

bool AShopBrewingStation::Interact_Implementation(UShopInteractionComponent* Interactor)
{
    return TryBrew(Interactor);
}

bool AShopBrewingStation::TryBrew(UShopInteractionComponent* Interactor)
{
    const AShopGameState* State = GetWorld()->GetGameState<AShopGameState>();
    if (State && State->IsChangingEconomy()) return false;
    if (!HasAuthority() || bBrewing || !IsValid(Interactor) || Interactor->GetHeldItem() || !Interactor->CanReach(this)
        || !HasActorBegunPlay() || !GetBlockReason().IsEmpty())
    {
        return false;
    }

    // Construction scripts can call back into the station while an item spawns.
    TGuardValue<bool> BrewingGuard(bBrewing, true);
    UShopBrewingRecipe* BatchRecipe = Recipe;
    const int32 Count = BatchSize;
    TArray<AShopShelfSlot*> Outputs;
    for (int32 Index = 0; Index < Count; ++Index) Outputs.Add(GetBatchOutput(Index));
    const TMap<FName, int32> Costs = Recipe->IngredientCosts;
    const FName ProductId = Recipe->ProductId;
    const FText ProductName = Recipe->DisplayName;
    const TSubclassOf<AShopPickupItem> OutputClass = Recipe->OutputItemClass;
    TArray<AShopPickupItem*> Products;
    auto Rollback = [&Products]()
    {
        for (AShopPickupItem* Product : Products) if (IsValid(Product)) Product->Destroy();
        return false;
    };
    for (AShopShelfSlot* Slot : Outputs)
    {
        if (!IsValid(Slot)) return Rollback();
        const FTransform Transform(Slot->PlacementPoint->GetComponentQuat(), Slot->PlacementPoint->GetComponentLocation());
        AShopPickupItem* Product = GetWorld()->SpawnActorDeferred<AShopPickupItem>(OutputClass, Transform);
        if (!IsValid(Product)) return Rollback();
        Products.Add(Product);
        Product->FinishSpawning(Transform);
    }
    // Construction callbacks must not change the quoted recipe, batch or outputs.
    if (IsActorBeingDestroyed() || !IsValid(BatchRecipe) || Recipe != BatchRecipe || BatchSize != Count
        || BatchRecipe->ProductId != ProductId || !BatchRecipe->DisplayName.EqualTo(ProductName)
        || BatchRecipe->OutputItemClass != OutputClass || !BatchRecipe->IngredientCosts.OrderIndependentCompareEqual(Costs)
        || !GetBlockReason().IsEmpty()) return Rollback();
    for (int32 Index = 0; Index < Count; ++Index)
    {
        if (Outputs[Index] != GetBatchOutput(Index) || !IsValid(Products[Index])) return Rollback();
        Products[Index]->ProductId = ProductId;
        Products[Index]->DisplayName = ProductName;
        if (!Products[Index]->PlaceAtSlot(Outputs[Index])) return Rollback();
    }
    // Only fully placed batches consume ingredients. Each bottle retains ordinary ownership.
    for (const TPair<FName, int32>& Cost : Costs)
        Ingredients.FindChecked(Cost.Key) -= int32(int64(Cost.Value) * Count);
    UGameplayStatics::PlaySoundAtLocation(this, BatchRecipe->BrewSound, GetActorLocation(), 0.4f);
    return true;
}


int32 AShopBrewingStation::CapacityForUpgrade(bool bUpgraded) const
{
    if (IngredientCapacity <= 0 || (bUpgraded && IngredientCapacity > MAX_int32 / 2)) return 0;
    return bUpgraded ? IngredientCapacity * 2 : IngredientCapacity;
}

AShopShelfSlot* AShopBrewingStation::GetBatchOutput(int32 Index) const
{
    return Index == 0 ? OutputSlot.Get() : (ExtraOutputs.IsValidIndex(Index - 1) ? ExtraOutputs[Index - 1].Get() : nullptr);
}

void AShopBrewingStation::RefreshBatchOutputs()
{
    for (AShopShelfSlot* Slot : ExtraOutputs)
        if (IsValid(Slot))
        {
            Slot->SetActorHiddenInGame(!bBatchUpgraded);
            Slot->SetActorEnableCollision(bBatchUpgraded);
        }
}

bool AShopBrewingStation::CycleBatchSize(UShopInteractionComponent* Interactor)
{
    const AShopGameState* State = GetWorld()->GetGameState<AShopGameState>();
    if (!HasAuthority() || !HasActorBegunPlay() || IsActorBeingDestroyed() || bBrewing || !bBatchUpgraded
        || (State && (State->IsChangingEconomy() || State->IsOvernight())) || !IsValid(Interactor) || Interactor->GetHeldItem()) return false;
    BatchSize = BatchSize == 3 ? 1 : 3;
    return true;
}
