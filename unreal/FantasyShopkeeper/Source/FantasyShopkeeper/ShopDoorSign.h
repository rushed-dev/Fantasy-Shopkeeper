#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShopInteractable.h"
#include "ShopDoorSign.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;

UCLASS(Blueprintable)
class FANTASYSHOPKEEPER_API AShopDoorSign : public AActor, public IShopInteractable
{
    GENERATED_BODY()
public:
    AShopDoorSign();
    virtual bool Interact_Implementation(UShopInteractionComponent* Interactor) override;
    virtual void Tick(float DeltaSeconds) override;
    FText GetInteractionPrompt() const;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Board;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Label;
};
