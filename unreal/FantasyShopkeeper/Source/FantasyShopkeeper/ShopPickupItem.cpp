#include "ShopPickupItem.h"

#include "Components/StaticMeshComponent.h"
#include "Components/ArrowComponent.h"
#include "ShopInteractionComponent.h"
#include "ShopShelfSlot.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Camera/CameraComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"

AShopPickupItem::AShopPickupItem()
{
    PrimaryActorTick.bCanEverTick = false;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("ItemRoot"));
    RootComponent->SetMobility(EComponentMobility::Movable);
    ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
    ItemMesh->SetupAttachment(RootComponent);
    ItemMesh->SetMobility(EComponentMobility::Movable);
    ItemMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    ItemMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    ItemMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    ItemMesh->SetGenerateOverlapEvents(false);
    ItemMesh->SetRelativeScale3D(FVector(0.2));
    ItemMesh->SetRelativeLocation(FVector(0.0, 0.0, 10.0));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> Placeholder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (Placeholder.Succeeded())
    {
        ItemMesh->SetStaticMesh(Placeholder.Object);
    }
}

AShopShelfSlot* AShopPickupItem::GetCurrentSlot() const
{
    return IsValid(CurrentSlot) ? CurrentSlot.Get() : nullptr;
}

UShopInteractionComponent* AShopPickupItem::GetCarrier() const
{
    return IsValid(Carrier) ? Carrier.Get() : nullptr;
}

bool AShopPickupItem::Interact_Implementation(UShopInteractionComponent* Interactor)
{
    return PickUp(Interactor);
}

bool AShopPickupItem::PickUp(UShopInteractionComponent* Interactor)
{
    if (!IsValid(Interactor) || Interactor->GetHeldItem() || GetCarrier())
    {
        return false;
    }

    AShopShelfSlot* PreviousSlot = GetCurrentSlot();
    if (PreviousSlot && PreviousSlot->GetOccupyingItem() != this)
    {
        return false;
    }

    // A shelf's interaction box may be the first hit when retrieving its item.
    if (!Interactor->CanReach(this) && !(PreviousSlot && Interactor->CanReach(PreviousSlot)))
    {
        return false;
    }

    USceneComponent* CarryPoint = Interactor->GetCarryPoint();
    if (!IsValid(CarryPoint))
    {
        return false;
    }

    ItemMesh->SetSimulatePhysics(false);
    const FAttachmentTransformRules Rules(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::KeepWorld, false);
    if (!AttachToComponent(CarryPoint, Rules))
    {
        return false;
    }

    if (PreviousSlot)
    {
        PreviousSlot->OccupyingItem = nullptr;
    }
    CurrentSlot = nullptr;
    Carrier = Interactor;
    Interactor->HeldItem = this;
    SetActorEnableCollision(false);
    UGameplayStatics::PlaySoundAtLocation(this, PickupSound, GetActorLocation(), 0.22f);
    return true;
}

bool AShopPickupItem::PlaceInSlot(UShopInteractionComponent* Interactor, AShopShelfSlot* DestinationSlot)
{
    if (!IsValid(Interactor) || GetCarrier() != Interactor || Interactor->GetHeldItem() != this
        || !IsValid(DestinationSlot) || DestinationSlot->GetOccupyingItem() || !Interactor->CanReach(DestinationSlot))
    {
        return false;
    }

    if (!PlaceAtSlot(DestinationSlot))
    {
        return false;
    }
    UGameplayStatics::PlaySoundAtLocation(this, PlacementSound, GetActorLocation(), 0.25f);
    return true;
}

bool AShopPickupItem::PlaceAtSlot(AShopShelfSlot* DestinationSlot)
{
    if (!IsValid(DestinationSlot) || DestinationSlot->GetWorld() != GetWorld()
        || DestinationSlot->GetOccupyingItem() || GetCurrentSlot())
    {
        return false;
    }

    const FAttachmentTransformRules Rules(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::KeepWorld, false);
    if (!AttachToComponent(DestinationSlot->PlacementPoint, Rules))
    {
        return false;
    }

    DestinationSlot->OccupyingItem = this;
    CurrentSlot = DestinationSlot;
    if (IsValid(Carrier) && Carrier->HeldItem == this)
    {
        Carrier->HeldItem = nullptr;
    }
    Carrier = nullptr;
    SetActorEnableCollision(true);
    return true;
}

bool AShopPickupItem::TrySetDown(UShopInteractionComponent* Interactor)
{
    if (!HasAuthority() || !HasActorBegunPlay() || IsActorBeingDestroyed() || !IsValid(Interactor)
        || GetCarrier() != Interactor || Interactor->GetHeldItem() != this || !IsValid(ItemMesh)
        || !ItemMesh->GetStaticMesh())
    {
        return false;
    }
    const UCameraComponent* Camera = Interactor->GetCamera();
    if (!Camera)
    {
        return false;
    }
    FCollisionQueryParams Params(SCENE_QUERY_STAT(ShopSetDown), false, Camera->GetOwner());
    Params.AddIgnoredActor(this);
    FHitResult Hit;
    const FVector Start = Camera->GetComponentLocation();
    GetWorld()->LineTraceSingleByChannel(Hit, Start,
        Start + Camera->GetForwardVector() * FMath::Max(0.f, Interactor->InteractionDistance), ECC_Visibility, Params);
    if (!Hit.bBlockingHit || !Hit.GetComponent() || Hit.ImpactNormal.Z < 0.9
        || Hit.GetComponent()->Mobility != EComponentMobility::Static
        || Hit.GetComponent()->GetCollisionResponseToChannel(ECC_Pawn) != ECR_Block)
    {
        return false;
    }

    const FTransform MeshToActor = ItemMesh->GetComponentTransform().GetRelativeTransform(GetActorTransform());
    FTransform Placement(FRotator(0, Camera->GetComponentRotation().Yaw, 0), Hit.ImpactPoint, GetActorScale3D());
    FBox Bounds = ItemMesh->GetStaticMesh()->GetBounds().GetBox().TransformBy(MeshToActor * Placement);
    const FVector SurfaceOffset(0, 0, Hit.ImpactPoint.Z - Bounds.Min.Z + 0.2);
    Placement.AddToTranslation(SurfaceOffset);
    Bounds = Bounds.ShiftBy(SurfaceOffset);
    if (GetWorld()->OverlapBlockingTestByChannel(Bounds.GetCenter(), FQuat::Identity, ECC_Visibility,
        FCollisionShape::MakeBox(Bounds.GetExtent()), Params))
    {
        return false;
    }
    if (!SetActorTransform(Placement, false, nullptr, ETeleportType::TeleportPhysics))
    {
        return false;
    }
    ReleaseFromCarrier();
    UGameplayStatics::PlaySoundAtLocation(this, PlacementSound, GetActorLocation(), 0.25f);
    return true;
}

void AShopPickupItem::ReleaseFromCarrier()
{
    if (IsValid(Carrier) && Carrier->HeldItem == this)
    {
        Carrier->HeldItem = nullptr;
    }
    Carrier = nullptr;
    DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    SetActorEnableCollision(true);
}

void AShopPickupItem::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (AShopShelfSlot* Slot = GetCurrentSlot(); Slot && Slot->OccupyingItem == this)
    {
        Slot->OccupyingItem = nullptr;
    }
    CurrentSlot = nullptr;
    ReleaseFromCarrier();
    Super::EndPlay(EndPlayReason);
}
