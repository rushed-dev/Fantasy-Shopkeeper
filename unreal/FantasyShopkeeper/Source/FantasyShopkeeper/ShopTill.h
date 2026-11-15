#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShopInteractable.h"
#include "ShopTill.generated.h"

class AShopCustomer;
class AShopPickupItem;
class UStaticMeshComponent;
class USoundBase;

UENUM(BlueprintType)
enum class EShopTillPhase : uint8
{
    Idle,
    EnteringTotal,
    MakingChange,
    Complete
};

UCLASS(Blueprintable)
class FANTASYSHOPKEEPER_API AShopTill : public AActor, public IShopInteractable
{
    GENERATED_BODY()

public:
    AShopTill();
    virtual bool Interact_Implementation(UShopInteractionComponent* Interactor) override;

    UFUNCTION(BlueprintCallable, Category = "Shop|Till")
    bool BeginCheckout(UShopInteractionComponent* Interactor);

    UFUNCTION(BlueprintCallable, Category = "Shop|Till")
    bool SubmitTotal(const FString& Text);

    UFUNCTION(BlueprintCallable, Category = "Shop|Till")
    bool AddChangeCoin(int32 Denomination);

    UFUNCTION(BlueprintCallable, Category = "Shop|Till")
    bool RemoveChangeCoin(int32 Denomination);

    UFUNCTION(BlueprintCallable, Category = "Shop|Till")
    bool ConfirmSale(UShopInteractionComponent* Interactor);

    bool AssignCustomer(AShopCustomer* NewCustomer);

    int64 GetChangeTotal() const;
    int32 GetCoinCount(int32 Denomination) const { return TrayCoins.FindRef(Denomination); }
    FText GetInteractionPrompt() const;
    EShopTillPhase GetPhase() const { return Phase; }
    const FText& GetFeedback() const { return Feedback; }
    const FText& GetProductName() const { return QuotedProductName; }
    int32 GetQuotedPrice() const { return QuotedPrice; }
    int32 GetTender() const { return QuotedTender; }

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Shop|Till")
    TObjectPtr<AShopCustomer> Customer;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shop|Till")
    TObjectPtr<UStaticMeshComponent> TillMesh;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop|Audio")
    TObjectPtr<USoundBase> CoinSound;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop|Audio")
    TObjectPtr<USoundBase> SaleSound;

    // Keep unsubmitted text when the player closes and reopens the panel.
    UPROPERTY(Transient)
    FString EnteredTotalText;

private:
    friend class AShopGameState;
    bool IsQuoteValid() const;
    bool CanCommitSale() const;
    void ResetTransaction();

    UPROPERTY(Transient)
    EShopTillPhase Phase = EShopTillPhase::Idle;

    UPROPERTY(Transient)
    TWeakObjectPtr<AShopCustomer> QuotedCustomer;

    UPROPERTY(Transient)
    TWeakObjectPtr<AShopPickupItem> QuotedItem;

    UPROPERTY(Transient)
    TMap<int32, int32> TrayCoins;

    UPROPERTY(Transient)
    FName QuotedProductId;

    UPROPERTY(Transient)
    FText QuotedProductName;

    UPROPERTY(Transient)
    FText Feedback;

    int32 QuotedPrice = 0;
    int32 QuotedTender = 0;
};
