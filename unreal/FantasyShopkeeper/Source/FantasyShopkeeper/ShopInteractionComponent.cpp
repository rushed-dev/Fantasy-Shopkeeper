#include "ShopInteractionComponent.h"

#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "ShopInteractable.h"
#include "ShopPickupItem.h"
#include "ShopShelfSlot.h"
#include "ShopShelfSection.h"
#include "ShopTill.h"
#include "ShopTillControl.h"

namespace
{
    AActor* SectionTarget(AActor* Actor)
    {
        AShopShelfSlot* Slot = Cast<AShopShelfSlot>(Actor);
        if (const AShopPickupItem* Item = Cast<AShopPickupItem>(Actor))
        {
            Slot = Item->GetCurrentSlot();
        }
        if (Slot && Slot->GetSection())
        {
            return Slot->GetSection();
        }
        return Actor;
    }
}

UShopInteractionComponent::UShopInteractionComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

UCameraComponent* UShopInteractionComponent::GetCamera() const
{
    const APlayerController* Controller = Cast<APlayerController>(GetOwner());
    const APawn* Pawn = Cast<APawn>(GetOwner());
    if (Controller)
    {
        Pawn = Controller->GetPawn();
    }
    if (!IsValid(Pawn))
    {
        return nullptr;
    }

    TInlineComponentArray<UCameraComponent*> Cameras(Pawn);
    for (UCameraComponent* Camera : Cameras)
    {
        if (Camera->IsActive())
        {
            return Camera;
        }
    }
    return nullptr;
}

AActor* UShopInteractionComponent::GetInteractionTarget() const
{
    const UCameraComponent* Camera = GetCamera();
    if (!Camera || !GetWorld())
    {
        return nullptr;
    }

    FCollisionQueryParams Params(SCENE_QUERY_STAT(ShopInteraction), false, Camera->GetOwner());
    Params.AddIgnoredActor(GetHeldItem());
    FHitResult Hit;
    const FVector Start = Camera->GetComponentLocation();
    GetWorld()->LineTraceSingleByChannel(Hit, Start,
        Start + Camera->GetForwardVector() * FMath::Max(0.0f, InteractionDistance), ECC_Visibility, Params);
    return SectionTarget(Hit.GetActor());
}

bool UShopInteractionComponent::CanReach(const AActor* Target) const
{
    if (!IsValid(Target) || Target->GetWorld() != GetWorld())
    {
        return false;
    }
    AActor* Resolved = SectionTarget(const_cast<AActor*>(Target));
    const UCameraComponent* Camera = GetCamera();
    AActor* Hit = GetInteractionTarget();
    if (const AShopTillControl* Control = Cast<AShopTillControl>(Hit))
    {
        if (Control->GetTill() == Target)
        {
            return true;
        }
    }
    return Hit == Resolved && (Resolved == Target
        || (Camera && FVector::Dist(Camera->GetComponentLocation(), Target->GetActorLocation()) <= InteractionDistance));
}

AShopPickupItem* UShopInteractionComponent::GetHeldItem() const
{
    return IsValid(HeldItem) ? HeldItem.Get() : nullptr;
}

bool UShopInteractionComponent::Interact()
{
    AActor* Target = GetInteractionTarget();
    return IsValid(Target) && Target->Implements<UShopInteractable>()
        && IShopInteractable::Execute_Interact(Target, this);
}

bool UShopInteractionComponent::SetDownHeldItem()
{
    AShopPickupItem* Item = GetHeldItem();
    return Item && Item->TrySetDown(this);
}

USceneComponent* UShopInteractionComponent::GetCarryPoint()
{
    if (IsValid(CarryAnchor))
    {
        return CarryAnchor;
    }
    UCameraComponent* Camera = GetCamera();
    if (!Camera)
    {
        return nullptr;
    }

    if (!IsValid(CarryPoint) || CarryPoint->GetOwner() != Camera->GetOwner())
    {
        CarryPoint = NewObject<USceneComponent>(Camera->GetOwner(), TEXT("NativeCarryPoint"));
        CarryPoint->SetupAttachment(Camera);
        CarryPoint->RegisterComponent();
    }
    CarryPoint->SetRelativeLocation(CarryOffset);
    return CarryPoint;
}

void UShopInteractionComponent::ReleaseHeldItem()
{
    if (AShopPickupItem* Item = GetHeldItem())
    {
        Item->ReleaseFromCarrier();
    }
}

void UShopInteractionComponent::SetCarryAnchor(USceneComponent* Anchor)
{
    if (!GetHeldItem() && (!Anchor || Anchor->GetWorld() == GetWorld()))
    {
        CarryAnchor = Anchor;
    }
}

void UShopInteractionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ReleaseHeldItem();
    Super::EndPlay(EndPlayReason);
}
