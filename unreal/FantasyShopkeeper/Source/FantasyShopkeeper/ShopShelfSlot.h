#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShopInteractable.h"
#include "ShopShelfSlot.generated.h"

class AShopPickupItem;
class UBoxComponent;
class UArrowComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class FANTASYSHOPKEEPER_API AShopShelfSlot : public AActor, public IShopInteractable
{
    GENERATED_BODY()

public:
    AShopShelfSlot();

    virtual bool Interact_Implementation(UShopInteractionComponent* Interactor) override;

    UFUNCTION(BlueprintPure, Category = "Shop|Shelf")
    AShopPickupItem* GetOccupyingItem() const;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop|Shelf")
    bool bAvailableForSale = true;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shop|Shelf")
    TObjectPtr<UBoxComponent> InteractionBox;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shop|Shelf")
    TObjectPtr<UArrowComponent> PlacementPoint;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shop|Shelf")
    TObjectPtr<UStaticMeshComponent> SlotMarker;

protected:
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    friend class AShopPickupItem;

    UPROPERTY(Transient)
    TObjectPtr<AShopPickupItem> OccupyingItem;
};
