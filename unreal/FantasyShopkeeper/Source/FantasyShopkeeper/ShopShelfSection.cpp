#include "ShopShelfSection.h"

#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "ShopInteractionComponent.h"
#include "ShopPickupItem.h"
#include "ShopShelfSlot.h"
#include "Engine/StaticMesh.h"

AShopShelfSection::AShopShelfSection()
{
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SectionRoot"));
    InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
    InteractionBox->SetupAttachment(RootComponent);
    InteractionBox->SetBoxExtent(FVector(47, 24, 18));
    InteractionBox->SetRelativeLocation(FVector(0, 0, 14));
    InteractionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    InteractionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
    InteractionBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    InteractionBox->SetGenerateOverlapEvents(false);
    InteractionBox->SetCanEverAffectNavigation(false);
    PositionMarkers = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("PositionMarkers"));
    PositionMarkers->SetupAttachment(RootComponent);
    PositionMarkers->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PositionMarkers->SetCastShadow(false);
    PositionMarkers->SetCanEverAffectNavigation(false);
    ProductLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("ProductLabel"));
    ProductLabel->SetupAttachment(RootComponent);
    ProductLabel->SetRelativeLocation(FVector(0, -26, -8));
    ProductLabel->SetRelativeRotation(FRotator(0, -90, 0));
    ProductLabel->SetHorizontalAlignment(EHTA_Center);
    ProductLabel->SetWorldSize(5);
    ProductLabel->SetTextRenderColor(FColor(240, 224, 177));
}

FVector AShopShelfSection::GetSlotOffset(int32 Index) const
{
    return FVector((Index % 4 - 1.5) * 22, Index < 4 ? -10 : 10, 0);
}

void AShopShelfSection::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    if (!PositionMarkers->GetStaticMesh())
    {
        PositionMarkers->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Shop/Art/Meshes/SM_StockMarkerRing")));
    }
    PositionMarkers->ClearInstances();
    for (int32 Index = 0; Index < 8; ++Index)
    {
        PositionMarkers->AddInstance(FTransform(GetSlotOffset(Index) + FVector(0, 0, 0.1)));
    }
    RefreshLabel();
}

void AShopShelfSection::BeginPlay()
{
    Super::BeginPlay();
    if (!HasAuthority())
    {
        return;
    }
    for (int32 Index = 0; Index < 8; ++Index)
    {
        const FTransform Transform(GetActorQuat(), GetActorTransform().TransformPosition(GetSlotOffset(Index)));
        AShopShelfSlot* Slot = GetWorld()->SpawnActorDeferred<AShopShelfSlot>(AShopShelfSlot::StaticClass(), Transform, this);
        if (!Slot)
        {
            continue;
        }
        Slot->Section = this;
        Slot->bAvailableForSale = bAvailableForSale;
        Slot->FinishSpawning(Transform);
        if (!Slot->HasActorBegunPlay())
        {
            Slot->DispatchBeginPlay();
        }
        Slot->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);
        Slot->SlotMarker->SetVisibility(false);
        Slot->InteractionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Slots.Add(Slot);
    }
    RefreshLabel();
}

AShopShelfSlot* AShopShelfSection::GetSlot(int32 Index) const
{
    return Slots.IsValidIndex(Index) && IsValid(Slots[Index]) ? Slots[Index].Get() : nullptr;
}

AShopShelfSlot* AShopShelfSection::GetNextFreeSlot() const
{
    for (AShopShelfSlot* Slot : Slots)
    {
        if (IsValid(Slot) && !Slot->GetOccupyingItem())
        {
            return Slot;
        }
    }
    return nullptr;
}

AShopShelfSlot* AShopShelfSection::GetFirstStockedSlot() const
{
    for (AShopShelfSlot* Slot : Slots)
    {
        if (IsValid(Slot) && Slot->GetOccupyingItem())
        {
            return Slot;
        }
    }
    return nullptr;
}

FName AShopShelfSection::GetLockedProductId() const
{
    const AShopShelfSlot* Slot = GetFirstStockedSlot();
    return Slot ? Slot->GetOccupyingItem()->ProductId : NAME_None;
}

int32 AShopShelfSection::GetStockCount() const
{
    int32 Count = 0;
    for (const AShopShelfSlot* Slot : Slots)
    {
        Count += IsValid(Slot) && Slot->GetOccupyingItem() ? 1 : 0;
    }
    return Count;
}

bool AShopShelfSection::CanAccept(const AShopPickupItem* Item) const
{
    if (Slots.Num() != 8 || bChangingStock || IsActorBeingDestroyed() || !IsValid(Item) || Item->IsActorBeingDestroyed()
        || Item->GetWorld() != GetWorld() || Item->ProductId.IsNone() || !GetNextFreeSlot())
    {
        return false;
    }
    const FName Product = GetLockedProductId();
    return Product.IsNone() || Product == Item->ProductId;
}

bool AShopShelfSection::Interact_Implementation(UShopInteractionComponent* Interactor)
{
    if (!HasAuthority() || bChangingStock || !IsValid(Interactor) || !Interactor->CanReach(this))
    {
        return false;
    }
    if (AShopPickupItem* Item = Interactor->GetHeldItem())
    {
        return CanAccept(Item) && Item->PlaceInSlot(Interactor, GetNextFreeSlot());
    }
    AShopShelfSlot* Slot = GetFirstStockedSlot();
    return Slot && Slot->GetOccupyingItem()->PickUp(Interactor);
}

FText AShopShelfSection::GetInteractionPrompt(UShopInteractionComponent* Interactor) const
{
    const AShopShelfSlot* Stock = GetFirstStockedSlot();
    const FText Name = Stock ? Stock->GetOccupyingItem()->DisplayName : NSLOCTEXT("Shop", "EmptySection", "Empty shelf");
    if (IsValid(Interactor) && Interactor->GetHeldItem())
    {
        const AShopPickupItem* Held = Interactor->GetHeldItem();
        if (Held->ProductId.IsNone())
        {
            return NSLOCTEXT("Shop", "UnclassifiedStock", "This item needs a product type before stocking");
        }
        if (Stock && Held->ProductId != GetLockedProductId())
        {
            return FText::Format(NSLOCTEXT("Shop", "SectionWrongProduct", "This shelf holds {0}"), Name);
        }
        if (!GetNextFreeSlot())
        {
            return NSLOCTEXT("Shop", "SectionFull", "Shelf full (8/8)");
        }
        return FText::Format(NSLOCTEXT("Shop", "StockSection", "E  Stock {0} ({1}/8)"), Held->DisplayName, FText::AsNumber(GetStockCount()));
    }
    return FText::Format(NSLOCTEXT("Shop", "RetrieveSection", "{0} ({1}/8) | E  Take next bottle"), Name, FText::AsNumber(GetStockCount()));
}

void AShopShelfSection::RefreshLabel()
{
    const AShopShelfSlot* Stock = GetFirstStockedSlot();
    const FText Name = Stock ? Stock->GetOccupyingItem()->DisplayName : NSLOCTEXT("Shop", "EmptySection", "Empty shelf");
    ProductLabel->SetText(FText::Format(NSLOCTEXT("Shop", "SectionLabel", "{0}  {1}/8"), Name, FText::AsNumber(GetStockCount())));
}

void AShopShelfSection::EndPlay(const EEndPlayReason::Type Reason)
{
    for (AShopShelfSlot* Slot : Slots)
    {
        if (IsValid(Slot))
        {
            Slot->Destroy();
        }
    }
    Slots.Empty();
    Super::EndPlay(Reason);
}
