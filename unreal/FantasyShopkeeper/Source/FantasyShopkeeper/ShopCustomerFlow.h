#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShopCustomerFlow.generated.h"

class AShopCustomer;
class AShopTill;

USTRUCT(BlueprintType)
struct FShopCustomerOrder
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Order")
    FName ProductId = TEXT("DawnTonic");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Order")
    FText ProductName = NSLOCTEXT("Shop", "DawnTonicOrder", "Dawn Tonic");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Order", meta = (ClampMin = "1"))
    int32 PriceBits = 12;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Order", meta = (ClampMin = "1"))
    int32 TenderBits = 20;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Order")
    TSubclassOf<AShopCustomer> CustomerClass;
};

// One visit at a time keeps a waiting order and its checkout bound together.
UCLASS(Blueprintable)
class FANTASYSHOPKEEPER_API AShopCustomerFlow : public AActor
{
    GENERATED_BODY()

public:
    AShopCustomerFlow();
    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintCallable, Category = "Shop|Customers")
    bool StartNextVisit();
    void ResetMorning();
    bool EndUnservedVisit();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop|Customers", meta = (ClampMin = "1", ClampMax = "100"))
    int32 VisitsPerDay = 3;

    int32 GetStartedVisits() const { return StartedVisits; }

    UFUNCTION(BlueprintPure, Category = "Shop|Customers")
    AShopCustomer* GetActiveCustomer() const;

    UFUNCTION(BlueprintPure, Category = "Shop|Customers")
    int32 GetCompletedVisits() const { return CompletedVisits; }

    FText GetStatusText() const;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Shop|Customers")
    TObjectPtr<AShopTill> Till;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop|Customers")
    TSubclassOf<AShopCustomer> CustomerClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop|Customers")
    TArray<FShopCustomerOrder> Orders;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop|Customers", meta = (MakeEditWidget = "true"))
    FVector Entrance = FVector(0, 0, 88);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop|Customers", meta = (MakeEditWidget = "true"))
    FVector Checkout = FVector(250, 0, 88);

    // Keep checkout aligned when the counter is moved without rerunning an installer.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop|Customers")
    bool bCheckoutFollowsTill = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop|Customers")
    FVector TillCheckoutOffset = FVector(0, 100, 88);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop|Customers", meta = (ClampMin = "0"))
    float VisitInterval = 5;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop|Customers", meta = (ClampMin = "0"))
    float DepartureDelay = 2;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:

    UPROPERTY(Transient)
    TObjectPtr<AShopCustomer> ActiveCustomer;

    int32 CompletedVisits = 0;
    int32 StartedVisits = 0;
    float WaitTime = 0;
    bool bVisitCounted = false;
    bool bStartingVisit = false;
    bool bOwnsActiveCustomer = false;
};
