#include "ShopShelfSlot.h"

#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "ShopInteractionComponent.h"
#include "ShopPickupItem.h"
#include "UObject/ConstructorHelpers.h"

AShopShelfSlot::AShopShelfSlot()
{
    PrimaryActorTick.bCanEverTick = false;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SlotRoot"));
    RootComponent->SetMobility(EComponentMobility::Movable);

    PlacementPoint = CreateDefaultSubobject<UArrowComponent>(TEXT("PlacementPoint"));
    PlacementPoint->SetupAttachment(RootComponent);
    PlacementPoint->SetMobility(EComponentMobility::Movable);

    InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
    InteractionBox->SetupAttachment(RootComponent);
    InteractionBox->SetMobility(EComponentMobility::Movable);
    InteractionBox->SetBoxExtent(FVector(16.0, 16.0, 14.0));
    InteractionBox->SetRelativeLocation(FVector(0.0, 0.0, 14.0));
    InteractionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    InteractionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
    InteractionBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    InteractionBox->SetGenerateOverlapEvents(false);

    SlotMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SlotMarker"));
    SlotMarker->SetupAttachment(RootComponent);
    SlotMarker->SetMobility(EComponentMobility::Movable);
    SlotMarker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SlotMarker->SetRelativeLocation(FVector(0.0, 0.0, 0.5));
    SlotMarker->SetRelativeScale3D(FVector(0.32, 0.32, 0.01));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> MarkerMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (MarkerMesh.Succeeded())
    {
        SlotMarker->SetStaticMesh(MarkerMesh.Object);
    }
}

AShopPickupItem* AShopShelfSlot::GetOccupyingItem() const
{
    return IsValid(OccupyingItem) ? OccupyingItem.Get() : nullptr;
}

bool AShopShelfSlot::Interact_Implementation(UShopInteractionComponent* Interactor)
{
    if (!IsValid(Interactor) || !Interactor->CanReach(this))
    {
        return false;
    }
    if (AShopPickupItem* HeldItem = Interactor->GetHeldItem())
    {
        return HeldItem->PlaceInSlot(Interactor, this);
    }
    AShopPickupItem* Item = GetOccupyingItem();
    return Item && Item->PickUp(Interactor);
}

void AShopShelfSlot::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (AShopPickupItem* Item = GetOccupyingItem(); Item && Item->CurrentSlot == this)
    {
        Item->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
        Item->CurrentSlot = nullptr;
    }
    OccupyingItem = nullptr;
    Super::EndPlay(EndPlayReason);
}
