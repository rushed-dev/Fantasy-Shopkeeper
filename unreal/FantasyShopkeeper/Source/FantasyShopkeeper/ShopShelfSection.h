#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShopInteractable.h"
#include "ShopShelfSection.generated.h"

class AShopShelfSlot;
class AShopPickupItem;
class UBoxComponent;
class UInstancedStaticMeshComponent;
class UTextRenderComponent;

UCLASS(Blueprintable)
class FANTASYSHOPKEEPER_API AShopShelfSection : public AActor, public IShopInteractable
{
    GENERATED_BODY()

public:
    AShopShelfSection();
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual bool Interact_Implementation(UShopInteractionComponent* Interactor) override;
    AShopShelfSlot* GetSlot(int32 Index) const;
    AShopShelfSlot* GetNextFreeSlot() const;
    AShopShelfSlot* GetFirstStockedSlot() const;
    FName GetLockedProductId() const;
    int32 GetStockCount() const;
    bool CanAccept(const AShopPickupItem* Item) const;
    FText GetInteractionPrompt(UShopInteractionComponent* Interactor) const;
    FVector GetSlotOffset(int32 Index) const;
    void RefreshLabel();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop|Shelf")
    bool bAvailableForSale = true;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shop|Shelf")
    TObjectPtr<UBoxComponent> InteractionBox;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shop|Shelf")
    TObjectPtr<UInstancedStaticMeshComponent> PositionMarkers;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shop|Shelf")
    TObjectPtr<UTextRenderComponent> ProductLabel;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
    friend class AShopPickupItem;
    bool bChangingStock = false;

    UPROPERTY(Transient)
    TArray<TObjectPtr<AShopShelfSlot>> Slots;
};
