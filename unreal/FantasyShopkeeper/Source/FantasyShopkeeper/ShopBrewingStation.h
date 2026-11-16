#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShopInteractable.h"
#include "ShopBrewingStation.generated.h"

class UShopBrewingRecipe;
class AShopShelfSlot;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class FANTASYSHOPKEEPER_API AShopBrewingStation : public AActor, public IShopInteractable
{
    GENERATED_BODY()

public:
    AShopBrewingStation();
    bool HasBatchUpgrade() const { return bBatchUpgraded; }
    int32 GetBatchSize() const { return BatchSize; }
    AShopShelfSlot* GetBatchOutput(int32 Index) const;
    void RefreshBatchOutputs();
    bool HasStorageUpgrade() const { return bStorageUpgraded; }
    int32 GetIngredientCapacity() const { return CapacityForUpgrade(bStorageUpgraded); }
    int32 CapacityForUpgrade(bool bUpgraded) const;

    virtual bool Interact_Implementation(UShopInteractionComponent* Interactor) override;

    UFUNCTION(BlueprintCallable, Category = "Shop|Brewing")
    bool TryBrew(UShopInteractionComponent* Interactor);

    UFUNCTION(BlueprintCallable, Category = "Shop|Brewing")
    bool CycleRecipe(UShopInteractionComponent* Interactor);

    UFUNCTION(BlueprintPure, Category = "Shop|Brewing")
    int32 GetIngredientQuantity(FName IngredientId) const;

    UFUNCTION(BlueprintPure, Category = "Shop|Brewing")
    FText GetInteractionPrompt(UShopInteractionComponent* Interactor) const;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop|Brewing")
    TObjectPtr<UShopBrewingRecipe> Recipe;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop|Brewing")
    TArray<TObjectPtr<UShopBrewingRecipe>> AvailableRecipes;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Shop|Brewing")
    TObjectPtr<AShopShelfSlot> OutputSlot;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop|Brewing")
    TMap<FName, int32> StartingIngredients;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop|Brewing", meta = (ClampMin = "1"))
    int32 IngredientCapacity = 30;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shop|Brewing")
    TObjectPtr<UStaticMeshComponent> VesselMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shop|Brewing")
    TObjectPtr<UStaticMeshComponent> MoonleafContainer;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shop|Brewing")
    TObjectPtr<UStaticMeshComponent> WaterContainer;

protected:
    virtual void BeginPlay() override;

private:
    friend class AShopGameState;
    friend class AShopSupplyCrate;
    friend class AShopBrewingUpgrade;
    bool CycleBatchSize(UShopInteractionComponent* Interactor);
    bool CanReceiveIngredients(const TMap<FName, int32>& Delivery) const;
    void ReceiveIngredients(const TMap<FName, int32>& Delivery);
    FText GetBlockReason() const;

    UPROPERTY(Transient)
    TMap<FName, int32> Ingredients;

    UPROPERTY(Transient) TArray<TObjectPtr<AShopShelfSlot>> ExtraOutputs;
    bool bBatchUpgraded = false;
    int32 BatchSize = 1;
    bool bBrewing = false;
    bool bStorageUpgraded = false;
};
