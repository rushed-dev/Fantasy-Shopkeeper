#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShopInteractable.h"
#include "ShopTillControl.h"
#include "ShopTill.generated.h"

class AShopCustomer;
class AShopShelfSlot;
class AShopPickupItem;
class UStaticMeshComponent;
class USoundBase;
class UInstancedStaticMeshComponent;
class UTextRenderComponent;
class UStaticMesh;

UENUM(BlueprintType)
enum class EShopTillPhase : uint8
{
    Idle,
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
    bool AddChangeCoin(int32 Denomination);

    UFUNCTION(BlueprintCallable, Category = "Shop|Till")
    bool RemoveChangeCoin(int32 Denomination);

    UFUNCTION(BlueprintCallable, Category = "Shop|Till")
    bool ConfirmSale(UShopInteractionComponent* Interactor);

    bool PresentCustomerGoods();
    AShopPickupItem* GetCounterItem() const { return QuotedItem.Get(); }
    bool AssignCustomer(AShopCustomer* NewCustomer);
    bool UseControl(UShopInteractionComponent* Interactor, EShopTillControl Action, int32 Value = 0);
    AShopTillControl* FindControl(EShopTillControl Action, int32 Value = 0) const;
    int32 GetVisibleChangeCoins(int32 Denomination) const;
    void RefreshPresentation();
    void AdvancePresentation(float DeltaSeconds);
    bool IsPresentingGoods() const;
    int32 GetPaymentClinkCount() const { return PaymentClinks; }

    UPROPERTY(EditAnywhere, Category = "Shop|Till")
    bool bPhysicalCounter = false;

    UPROPERTY(EditAnywhere, Category = "Shop|Till")
    TArray<TObjectPtr<UStaticMesh>> CoinMeshes;

    UPROPERTY(EditAnywhere, Category = "Shop|Till")
    TObjectPtr<UStaticMesh> PouchMesh;

    UPROPERTY(EditAnywhere, Category = "Shop|Till")
    TObjectPtr<UStaticMesh> KeyMesh;

    UPROPERTY(VisibleAnywhere, Category = "Shop|Till")
    TObjectPtr<UTextRenderComponent> Ticket;
    UPROPERTY(VisibleAnywhere, Category = "Shop|Till") TObjectPtr<UTextRenderComponent> PaymentNote;
    UPROPERTY(VisibleAnywhere, Category = "Shop|Till") TObjectPtr<UTextRenderComponent> ChangeNote;

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

private:
    friend class AShopGameState;
    void FinishGoodsPresentation();
    void PlayCoinAt(int32 Denomination, bool bReturning, const FVector& LocalPosition);
    UPROPERTY() TObjectPtr<UStaticMeshComponent> GoodsVisual;
    UPROPERTY(Transient) TWeakObjectPtr<AShopPickupItem> PresentedItem;
    FTransform GoodsStart;
    FTransform GoodsEnd;
    bool bOriginalGoodsVisible = true;
    float PresentationTime = 2.f;
    int32 PaymentClinks = 0;
    float StampTime = 1.f;
    float ChangeDropTime = 1.f;
    int32 ChangeDropLane = INDEX_NONE;

    bool IsQuoteValid() const;
    bool CanCommitSale() const;
    void ResetTransaction();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void Tick(float DeltaSeconds) override;
    void BuildControls();
    UPROPERTY(Transient) TObjectPtr<AShopShelfSlot> CheckoutSlot;

    UPROPERTY(Transient)
    TArray<TObjectPtr<AShopTillControl>> Controls;

    UPROPERTY(VisibleAnywhere)
    TArray<TObjectPtr<UInstancedStaticMeshComponent>> ChangeVisuals;

    UPROPERTY(VisibleAnywhere)
    TArray<TObjectPtr<UInstancedStaticMeshComponent>> TenderVisuals;

    int64 DisplayedChange = -1;
    int32 DisplayedTender = -1;

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
