#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShopInteractable.h"
#include "ShopSupplyCrate.generated.h"

class AShopBrewingStation;
class UStaticMeshComponent;
class USoundBase;
class UTextRenderComponent;

USTRUCT(BlueprintType)
struct FShopSupplyOffer
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Supplies")
    int32 PriceBits = 12;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Supplies")
    TMap<FName, int32> Ingredients;
};

UCLASS(Blueprintable)
class FANTASYSHOPKEEPER_API AShopSupplyCrate : public AActor, public IShopInteractable
{
    GENERATED_BODY()

public:
    AShopSupplyCrate();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    FString GetPlanningText() const;
    bool GetNextDayNeeds(TMap<FName, int64>& Needs, int32& Visitors) const;
    FText GetOrderStatus() const { return GetBlockReason(); }
    bool CanSubmitOrder() const;
    bool ChangeQuantity(UShopInteractionComponent* Interactor, int32 Delta);
    void ResetOrder() { bOrderSubmitted = false; }
    bool HasSubmittedOrder() const { return bOrderSubmitted; }
    int32 GetPackCount() const { return PackCount; }
    bool GetOrder(TMap<FName, int32>& Quantities, int64& Cost) const;
    virtual bool Interact_Implementation(UShopInteractionComponent* Interactor) override;

    UFUNCTION(BlueprintCallable, Category = "Shop|Supplies")
    bool CycleOffer(UShopInteractionComponent* Interactor);

    UFUNCTION(BlueprintPure, Category = "Shop|Supplies")
    FText GetInteractionPrompt(UShopInteractionComponent* Interactor) const;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Shop|Supplies")
    TObjectPtr<AShopBrewingStation> Station;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop|Supplies", meta = (ClampMin = "1"))
    int32 PriceBits = 12;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop|Supplies")
    TMap<FName, int32> Delivery = {{TEXT("Moonleaf"), 6}, {TEXT("Spring Water"), 3}};

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop|Supplies")
    TArray<FShopSupplyOffer> Offers;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shop|Supplies")
    TObjectPtr<UStaticMeshComponent> CrateMesh;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop|Audio")
    TObjectPtr<USoundBase> DeliverySound;

private:
    friend class AShopGameState;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Cover;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Book;
    UPROPERTY() TObjectPtr<UTextRenderComponent> Page;
    bool bOrderSubmitted = false;
    int32 PackCount = 1;
    FText GetBlockReason() const;
};
