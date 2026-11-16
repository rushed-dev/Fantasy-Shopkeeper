#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShopInteractable.h"
#include "ShopBrewingUpgrade.generated.h"
class AShopBrewingStation;
class UStaticMeshComponent;
class UTextRenderComponent;

// A bench-mounted price plaque becomes the one/three-bottle selector after purchase.
UCLASS()
class FANTASYSHOPKEEPER_API AShopBrewingUpgrade : public AActor, public IShopInteractable
{
    GENERATED_BODY()
public:
    AShopBrewingUpgrade();
    void Configure(AShopBrewingStation* InStation);
    virtual void Tick(float DeltaSeconds) override;
    virtual bool Interact_Implementation(UShopInteractionComponent* Interactor) override;
    FText GetInteractionPrompt() const;
private:
    UPROPERTY() TObjectPtr<AShopBrewingStation> Station;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Rack;
    UPROPERTY() TObjectPtr<UTextRenderComponent> Label;
};
