#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ShopInteractionComponent.generated.h"

class AShopPickupItem;
class UCameraComponent;

UCLASS(ClassGroup = (Shop), meta = (BlueprintSpawnableComponent))
class FANTASYSHOPKEEPER_API UShopInteractionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UShopInteractionComponent();

    UFUNCTION(BlueprintCallable, Category = "Shop|Interaction")
    bool Interact();

    UFUNCTION(BlueprintCallable, Category = "Shop|Interaction")
    bool SetDownHeldItem();

    UFUNCTION(BlueprintPure, Category = "Shop|Interaction")
    AActor* GetInteractionTarget() const;

    UFUNCTION(BlueprintPure, Category = "Shop|Interaction")
    AShopPickupItem* GetHeldItem() const;

    UFUNCTION(BlueprintPure, Category = "Shop|Interaction")
    bool CanReach(const AActor* Target) const;

    USceneComponent* GetCarryPoint();
    void SetCarryAnchor(USceneComponent* Anchor);
    void ReleaseHeldItem();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop|Interaction", meta = (ClampMin = "1.0", Units = "cm"))
    float InteractionDistance = 200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop|Interaction")
    FVector CarryOffset = FVector(70.0, 20.0, -20.0);

protected:
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    friend class AShopPickupItem;

    UCameraComponent* GetCamera() const;

    UPROPERTY(Transient)
    TObjectPtr<AShopPickupItem> HeldItem;

    UPROPERTY(Transient)
    TObjectPtr<USceneComponent> CarryPoint;

    UPROPERTY(Transient)
    TObjectPtr<USceneComponent> CarryAnchor;
};
