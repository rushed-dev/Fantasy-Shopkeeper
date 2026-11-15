#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ShopPlayerController.generated.h"

class UShopInteractionComponent;
class UInputAction;
class UInputMappingContext;
class AShopTill;

UCLASS(Blueprintable)
class FANTASYSHOPKEEPER_API AShopPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    AShopPlayerController();
    virtual bool InputKey(const FInputKeyEventArgs& Params) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shop")
    TObjectPtr<UShopInteractionComponent> ShopInteraction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shop|Input")
    TObjectPtr<UInputMappingContext> MovementMappingContext;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shop|Input")
    TObjectPtr<UInputMappingContext> MouseLookMappingContext;

protected:
    virtual void SetupInputComponent() override;
    virtual void OnUnPossess() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    void HandleInteract();
    void HandleNextRecipe();
    void HandleSetDown();

    UPROPERTY(Transient)
    TObjectPtr<UInputAction> InteractAction;

    UPROPERTY(Transient)
    TObjectPtr<UInputAction> NextRecipeAction;

    UPROPERTY(Transient)
    TObjectPtr<UInputAction> SetDownAction;

    UPROPERTY(Transient)
    TObjectPtr<UInputMappingContext> InteractionMappingContext;

};
