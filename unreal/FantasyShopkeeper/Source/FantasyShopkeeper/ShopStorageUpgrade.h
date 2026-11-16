#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShopInteractable.h"
#include "ShopStorageUpgrade.generated.h"
class AShopBrewingStation;
class UStaticMeshComponent;
class UTextRenderComponent;

UCLASS()
class FANTASYSHOPKEEPER_API AShopStorageUpgrade : public AActor, public IShopInteractable
{
    GENERATED_BODY()
public:
    AShopStorageUpgrade();
    void Configure(AShopBrewingStation* InStation);
    virtual void Tick(float DeltaSeconds) override;
    virtual bool Interact_Implementation(UShopInteractionComponent* Interactor) override;
    FText GetInteractionPrompt() const;
    bool IsExpansionVisible() const;
private:
    UPROPERTY() TObjectPtr<AShopBrewingStation> Station;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Cabinet;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> ExtraDrawers;
    UPROPERTY() TObjectPtr<UTextRenderComponent> Label;
};
