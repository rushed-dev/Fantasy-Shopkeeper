#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShopInteractable.h"
#include "ShopSupplyControl.generated.h"
class AShopSupplyCrate;
class UStaticMeshComponent;
class UTextRenderComponent;
UENUM()
enum class EShopSupplyControl : uint8 { Pack, Less, More, Rest, Delivery };

// Reachable book tabs and the sealed morning parcel use the ordinary E action.
UCLASS()
class FANTASYSHOPKEEPER_API AShopSupplyControl : public AActor, public IShopInteractable
{
    GENERATED_BODY()
public:
    AShopSupplyControl();
    void Configure(AShopSupplyCrate* InDesk, EShopSupplyControl InAction);
    virtual void Tick(float DeltaSeconds) override;
    virtual bool Interact_Implementation(UShopInteractionComponent* Interactor) override;
    FText GetInteractionPrompt() const;
    AShopSupplyCrate* GetDesk() const { return Desk; }
    EShopSupplyControl GetAction() const { return Action; }
private:
    UPROPERTY() TObjectPtr<AShopSupplyCrate> Desk;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Mesh;
    UPROPERTY() TObjectPtr<UTextRenderComponent> Label;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Lid;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> ShippingTag;
    UPROPERTY() TObjectPtr<UStaticMesh> ParcelMesh;
    EShopSupplyControl Action = EShopSupplyControl::Pack;
};
