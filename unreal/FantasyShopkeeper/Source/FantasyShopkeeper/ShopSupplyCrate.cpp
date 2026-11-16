#include "ShopSupplyCrate.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "ShopBrewingStation.h"
#include "ShopCustomerFlow.h"
#include "ShopSupplyControl.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInterface.h"
#include "EngineUtils.h"
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
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.1f;
    CrateMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CrateMesh"));
    RootComponent = CrateMesh;
    CrateMesh->SetCollisionProfileName(TEXT("BlockAll"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (Cube.Succeeded())
    {
        CrateMesh->SetStaticMesh(Cube.Object);
        CrateMesh->SetRelativeScale3D(FVector(0.6, 0.45, 0.4));
    }
    Cover = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BookCover"));
    Cover->SetupAttachment(RootComponent);
    Cover->SetStaticMesh(Cube.Object);
    Cover->SetRelativeLocation(FVector(0, 0, 67));
    Cover->SetRelativeScale3D(FVector(0.72, 0.64, 0.035));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Leather(TEXT("/Game/Shop/Art/Materials/M_DarkWood.M_DarkWood"));
    Cover->SetMaterial(0, Leather.Object);
    Cover->SetCanEverAffectNavigation(false);
    Book = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("OrderBook"));
    Book->SetupAttachment(RootComponent);
    Book->SetStaticMesh(Cube.Object);
    Book->SetRelativeLocation(FVector(0, 0, 69.5));
    Book->SetRelativeScale3D(FVector(0.68, 0.60, 0.025));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Paper(TEXT("/Game/Shop/Art/Materials/M_CreamLabel.M_CreamLabel"));
    Book->SetMaterial(0, Paper.Object);
    Book->SetCanEverAffectNavigation(false);
    Page = CreateDefaultSubobject<UTextRenderComponent>(TEXT("OrderPage"));
    Page->SetupAttachment(RootComponent);
    Page->SetRelativeLocation(FVector(0, 27, 70.9));
    Page->SetRelativeRotation(FRotator(90, -90, 0));
    Page->SetHorizontalAlignment(EHTA_Center);
    Page->SetVerticalAlignment(EVRTA_TextTop);
    Page->SetWorldSize(4.f);
    Page->SetTextRenderColor(FColor(32, 36, 24));

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
    bOrderSubmitted = false;
    return true;
}

FText AShopSupplyCrate::GetBlockReason() const
{
    const AShopGameState* State = GetWorld()->GetGameState<AShopGameState>();
    if (!State || !State->IsDayInitialized() || State->GetDayPhase() != EShopDayPhase::Evening)
        return FText::FromString(TEXT("Order in the evening | Paid supplies arrive next morning"));
    if (bOrderSubmitted) return FText::FromString(TEXT("Order confirmed | Select another pack or quantity to order more"));
    if (PackCount == 0) return FText::GetEmpty();
    TMap<FName, int32> Quantities;
    int64 Cost = 0;
    if (!IsValid(Station) || Station->GetWorld() != GetWorld() || !GetOrder(Quantities, Cost))
        return FText::FromString(TEXT("Supply order is not configured correctly"));
    if (!State->CanReceiveOrder(Station, Quantities))
        return FText::FromString(TEXT("Orders exceed ingredient storage | Reduce quantity"));
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
            bOrderSubmitted = false;
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
    if (!State || State->GetDayPhase() != EShopDayPhase::Evening) return BlockReason;
    return BlockReason.IsEmpty() ? FText::FromString(TEXT("E  Sign order | Book tabs: pack, less, more, rest")) : BlockReason;

}


void AShopSupplyCrate::BeginPlay()
{
    Super::BeginPlay();
    for (int32 Index = 0; Index < 4; ++Index)
    {
        AShopSupplyControl* Control = GetWorld()->SpawnActor<AShopSupplyControl>(
            GetActorTransform().TransformPosition(FVector(25.5 - Index * 17, -36, 70)), GetActorRotation());
        if (!Control) continue;
        Control->SetOwner(this);
        Control->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
        Control->Configure(this, EShopSupplyControl(Index));
    }
}
FString AShopSupplyCrate::GetPlanningText() const
{
    const AShopGameState* State = GetWorld()->GetGameState<AShopGameState>();
    if (!State) return TEXT("SUPPLIER ORDER BOOK");
    int32 Visitors = 0;
    for (TActorIterator<AShopCustomerFlow> It(GetWorld()); It; ++It) { Visitors = It->VisitsPerDay; break; }
    TMap<FName, int32> Quantities;
    int64 Cost = 0;
    GetOrder(Quantities, Cost);
    const int32 Leaves = Station ? Station->GetIngredientQuantity(TEXT("Moonleaf")) : 0;
    const int32 Water = Station ? Station->GetIngredientQuantity(TEXT("Spring Water")) : 0;
    return FString::Printf(TEXT("TOMORROW - %d visitors\nTakings %lld / Supplies %lld\nPurse %lld / Upgrades %lld\n             Stored  Ordered\nMoonleaf     %d       %d\nWater        %d       %d\n\nAdd %d leaf / %d water\n%d packs : %lld bits\n%s"),
        Visitors, State->GetTakings(), State->GetSupplySpending(), State->GetCash(), State->GetUpgradeSpending(), Leaves,
        State->GetPendingIngredientQuantity(TEXT("Moonleaf")), Water, State->GetPendingIngredientQuantity(TEXT("Spring Water")),
        Quantities.FindRef(TEXT("Moonleaf")), Quantities.FindRef(TEXT("Spring Water")), PackCount, Cost,
        State->GetDayPhase() != EShopDayPhase::Evening ? TEXT("Orders taken in the evening")
            : bOrderSubmitted ? TEXT("PAID - change tabs to add more") : TEXT("E : sign & pay this order"));
}
void AShopSupplyCrate::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    Page->SetText(FText::FromString(GetPlanningText()));
}
