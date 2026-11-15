#include "ShopInteractionComponent.h"

#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "ShopInteractable.h"
#include "ShopPickupItem.h"

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
    return Hit.GetActor();
}

bool UShopInteractionComponent::CanReach(const AActor* Target) const
{
    return IsValid(Target) && Target->GetWorld() == GetWorld() && GetInteractionTarget() == Target;
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
