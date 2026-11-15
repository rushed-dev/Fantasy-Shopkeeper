#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShopInteractable.h"
#include "ShopPickupItem.generated.h"

class AShopShelfSlot;
class UStaticMeshComponent;
class USoundBase;

UCLASS(Blueprintable)
class FANTASYSHOPKEEPER_API AShopPickupItem : public AActor, public IShopInteractable
{
    GENERATED_BODY()

public:
    AShopPickupItem();

    virtual bool Interact_Implementation(UShopInteractionComponent* Interactor) override;

    UFUNCTION(BlueprintCallable, Category = "Shop|Item")
    bool PickUp(UShopInteractionComponent* Interactor);

    UFUNCTION(BlueprintCallable, Category = "Shop|Item")
    bool PlaceInSlot(UShopInteractionComponent* Interactor, AShopShelfSlot* DestinationSlot);

    UFUNCTION(BlueprintCallable, Category = "Shop|Item")
    bool TrySetDown(UShopInteractionComponent* Interactor);

    UFUNCTION(BlueprintPure, Category = "Shop|Item")
    AShopShelfSlot* GetCurrentSlot() const;

    UFUNCTION(BlueprintPure, Category = "Shop|Item")
    UShopInteractionComponent* GetCarrier() const;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shop|Item")
    TObjectPtr<UStaticMeshComponent> ItemMesh;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop|Item")
    FName ProductId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop|Item")
    FText DisplayName = NSLOCTEXT("Shop", "DefaultItemName", "item");

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shop|Audio")
    TObjectPtr<USoundBase> PickupSound;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shop|Audio")
    TObjectPtr<USoundBase> PlacementSound;

protected:
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    friend class AShopShelfSlot;
    friend class UShopInteractionComponent;
    friend class AShopBrewingStation;

    void ReleaseFromCarrier();
    bool PlaceAtSlot(AShopShelfSlot* DestinationSlot);

    UPROPERTY(Transient)
    TObjectPtr<AShopShelfSlot> CurrentSlot;

    UPROPERTY(Transient)
    TObjectPtr<UShopInteractionComponent> Carrier;
};
