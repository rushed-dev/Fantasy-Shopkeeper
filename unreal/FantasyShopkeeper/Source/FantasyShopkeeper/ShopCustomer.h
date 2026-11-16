#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ShopInteractable.h"
#include "ShopCustomer.generated.h"

class AShopPickupItem;
class UBoxComponent;
class UCameraComponent;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class EShopCustomerActivity : uint8
{
    Arriving,
    Browsing,
    Collecting,
    Returning,
    ReadyToPay,
    Completed,
    Leaving,
    Queued
};

UCLASS(Blueprintable)
class FANTASYSHOPKEEPER_API AShopCustomer : public ACharacter, public IShopInteractable
{
    GENERATED_BODY()

public:
    AShopCustomer();
    void ConfigureRegular(int32 Index, int32 Day);
    FName RegularId;
    FText GetCustomerName() const;
    void ConfigureVisitPersonality(int32 Variation);
    void AdvanceConversation(float DeltaSeconds);
    FText GetSpokenLine() const { return SpokenLine; }
    float GetSpeechOpacity() const;
    float GetDeparturePace() const { return 0.85f + 0.15f * DialogueVariation; }

    bool HasGoodsOnCounter() const;
    UPROPERTY(Transient) TWeakObjectPtr<AShopPickupItem> CounterItem;
    virtual void Tick(float DeltaSeconds) override;
    virtual bool Interact_Implementation(UShopInteractionComponent* Interactor) override;

    UFUNCTION(BlueprintCallable, Category = "Shop|Customer")
    bool TryCollectItem(AShopPickupItem* Item);

    bool BeginVisit(const FVector& Checkout);
    void SetQueueDestination(const FVector& Destination, bool bFront);
    bool BeginDeparture(const FVector& Exit);
    bool LeaveWithoutPurchase(const FVector& Exit);
    bool HasResolvedVisit() const { return HasCompletedSale() || bLeavingWithoutPurchase; }
    bool HasLeftShop() const { return bLeftShop; }

    UFUNCTION(BlueprintPure, Category = "Shop|Sale")
    AShopPickupItem* FindSaleItem() const;

    UFUNCTION(BlueprintPure, Category = "Shop|Sale")
    bool HasCompletedSale() const { return PaidBits > 0; }

    UFUNCTION(BlueprintPure, Category = "Shop|Sale")
    FText GetInteractionPrompt() const;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop|Sale")
    FName RequestedProductId = TEXT("DawnTonic");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop|Sale")
    FText RequestedProductName = NSLOCTEXT("Shop", "DawnTonicRequest", "Dawn Tonic");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop|Sale", meta = (ClampMin = "1"))
    int32 PriceBits = 12;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop|Sale", meta = (ClampMin = "1"))
    int32 TenderBits = 20;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shop|Customer")
    TObjectPtr<UShopInteractionComponent> ShopInteraction;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shop|Customer")
    TObjectPtr<UCameraComponent> InteractionEyes;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shop|Customer")
    TObjectPtr<USceneComponent> CarryAnchor;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shop|Customer")
    TObjectPtr<UStaticMeshComponent> PurchasedItemVisual;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shop|Sale")
    TObjectPtr<UBoxComponent> InteractionBox;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shop|Customer")
    EShopCustomerActivity Activity = EShopCustomerActivity::Browsing;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
    friend class AShopGameState;
    bool IsEligibleStock(const AShopPickupItem* Item) const;
    bool BeginCollecting();
    void ReturnToCheckout();
    bool MoveTo(const FVector& Destination);
    void PreparePurchasePresentation(const AShopPickupItem* Item);
    void FinishPurchasePresentation(bool bCompleted);

    UPROPERTY(Transient)
    int32 PaidBits = 0;

    UPROPERTY(Transient)
    TObjectPtr<AShopPickupItem> TargetItem;

    UPROPERTY() TObjectPtr<UStaticMeshComponent> RegularBadge;
    int32 VisitDay = 1;
    bool bCheckoutAllowed = true;
    FVector CheckoutLocation = FVector::ZeroVector;
    FVector CollectionLocation = FVector::ZeroVector;
    float RetryAfter = 0;
    float JourneyTime = 0;
    FVector VisitDestination = FVector::ZeroVector;
    int32 DialogueVariation = 0;
    bool bArrivalQueued = false;
    bool bPurchaseReactionQueued = false;
    float SpeechDelay = 0;
    float SpeechRemaining = 0;
    float SpeechDuration = 0;
    FText PendingSpeech;
    FText SpokenLine;
    void QueueSpeech(const FText& Line, float Delay);
    bool bLeftShop = false;
    bool bLeavingWithoutPurchase = false;
};
