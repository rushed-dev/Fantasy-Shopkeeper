#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ShopInteractable.generated.h"

class UShopInteractionComponent;

UINTERFACE(BlueprintType)
class FANTASYSHOPKEEPER_API UShopInteractable : public UInterface
{
    GENERATED_BODY()
};

class FANTASYSHOPKEEPER_API IShopInteractable
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Shop|Interaction")
    bool Interact(UShopInteractionComponent* Interactor);
};
