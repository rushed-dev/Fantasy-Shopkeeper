#include "ShopBrewingStation.h"

#include "Components/ArrowComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "ShopBrewingRecipe.h"
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
        || Delivery.IsEmpty() || IngredientCapacity <= 0)
    {
        return false;
    }
    for (const TPair<FName, int32>& Entry : Delivery)
    {
        const int32 Current = GetIngredientQuantity(Entry.Key);
        if (Entry.Key.IsNone() || Entry.Value <= 0 || Current < 0
            || int64(Current) + Entry.Value > IngredientCapacity)
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
    if (OutputSlot->GetOccupyingItem())
    {
        return NSLOCTEXT("Shop", "BrewOutputBlocked", "Clear the brewing output slot");
    }
    for (const TPair<FName, int32>& Cost : Recipe->IngredientCosts)
    {
        if (GetIngredientQuantity(Cost.Key) < Cost.Value)
        {
            return FText::Format(NSLOCTEXT("Shop", "MissingIngredient", "Need {0} ({1}/{2})"),
                FText::FromName(Cost.Key), FText::AsNumber(GetIngredientQuantity(Cost.Key)), FText::AsNumber(Cost.Value));
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
        Stock += FString::Printf(TEXT(" | %s %d/%d"), *Cost.Key.ToString(), GetIngredientQuantity(Cost.Key), Cost.Value);
    }
    return FText::Format(NSLOCTEXT("Shop", "BrewPrompt", "E  Brew {0}{1}{2}"), Recipe->DisplayName, FText::FromString(Stock), RecipeHint);
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
    if (!HasAuthority() || bBrewing || !IsValid(Interactor) || Interactor->GetHeldItem() || !Interactor->CanReach(this)
        || !HasActorBegunPlay() || !GetBlockReason().IsEmpty())
    {
        return false;
    }

    // Construction scripts can call back into the station while an item spawns.
    TGuardValue<bool> BrewingGuard(bBrewing, true);
    UShopBrewingRecipe* BatchRecipe = Recipe;
    AShopShelfSlot* BatchOutput = OutputSlot;
    const TMap<FName, int32> Costs = Recipe->IngredientCosts;
    const FName ProductId = Recipe->ProductId;
    const FText ProductName = Recipe->DisplayName;
    const TSubclassOf<AShopPickupItem> OutputClass = Recipe->OutputItemClass;
    const FTransform OutputTransform(OutputSlot->PlacementPoint->GetComponentQuat(), OutputSlot->PlacementPoint->GetComponentLocation());
    AShopPickupItem* Product = GetWorld()->SpawnActorDeferred<AShopPickupItem>(Recipe->OutputItemClass, OutputTransform);
    if (!IsValid(Product))
    {
        return false;
    }
    Product->FinishSpawning(OutputTransform);

    // Commit ingredients only after a live product has acquired its output slot.
    if (!IsValid(Product))
    {
        return false;
    }
    Product->ProductId = ProductId;
    Product->DisplayName = ProductName;
    if (IsActorBeingDestroyed() || !IsValid(BatchRecipe) || Recipe != BatchRecipe || OutputSlot != BatchOutput
        || BatchRecipe->ProductId != ProductId || !BatchRecipe->DisplayName.EqualTo(ProductName)
        || BatchRecipe->OutputItemClass != OutputClass || !BatchRecipe->IngredientCosts.OrderIndependentCompareEqual(Costs)
        || !GetBlockReason().IsEmpty() || !Product->PlaceAtSlot(BatchOutput))
    {
        Product->Destroy();
        return false;
    }
    for (const TPair<FName, int32>& Cost : Costs)
    {
        Ingredients.FindChecked(Cost.Key) -= Cost.Value;
    }
    UGameplayStatics::PlaySoundAtLocation(this, BatchRecipe->BrewSound, GetActorLocation(), 0.4f);
    return true;
}
