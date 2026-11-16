#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShopInteractable.h"
#include "ShopTillControl.generated.h"

class AShopTill;
class UBoxComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

UENUM()
enum class EShopTillControl : uint8
{
    AcceptPayment,
    AddCoin,
    RemoveCoin,
    Confirm
};

/** A reachable, world-space stamp, coin stack or payment dish. */
UCLASS()
class FANTASYSHOPKEEPER_API AShopTillControl : public AActor, public IShopInteractable
{
    GENERATED_BODY()

public:
    AShopTillControl();
    virtual bool Interact_Implementation(UShopInteractionComponent* Interactor) override;
    FText GetInteractionPrompt() const;
    AShopTill* GetTill() const { return Till.Get(); }

    UPROPERTY(Transient)
    TObjectPtr<AShopTill> Till;
    EShopTillControl Action = EShopTillControl::AcceptPayment;
    int32 Value = 0;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UBoxComponent> HitBox;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> Visual;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UTextRenderComponent> Label;
};
