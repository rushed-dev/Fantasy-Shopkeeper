#include "ShopTill.h"
#include "Algo/AllOf.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "ShopCustomer.h"
#include "ShopGameState.h"
#include "ShopInteractionComponent.h"
#include "ShopPickupItem.h"
#include "ShopPlayerController.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

AShopTill::AShopTill()
{
    PrimaryActorTick.bCanEverTick = false;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("TillRoot"));
    TillMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TillMesh"));
    TillMesh->SetupAttachment(RootComponent);
    TillMesh->SetRelativeLocation(FVector(0, 0, 10));
    TillMesh->SetRelativeScale3D(FVector(0.55, 0.32, 0.2));
    TillMesh->SetCollisionProfileName(TEXT("BlockAll"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (Cube.Succeeded())
    {
        TillMesh->SetStaticMesh(Cube.Object);
    }
}

bool AShopTill::IsQuoteValid() const
{
    return QuotedCustomer.IsValid() && Customer == QuotedCustomer.Get() && QuotedItem.IsValid()
        && !Customer->IsActorBeingDestroyed() && Customer->GetWorld() == GetWorld()
        && Customer->Activity == EShopCustomerActivity::ReadyToPay && !Customer->HasCompletedSale()
        && FVector::Dist2D(Customer->GetActorLocation(), GetActorLocation()) <= 200
        && Customer->RequestedProductId == QuotedProductId && Customer->PriceBits == QuotedPrice
        && Customer->TenderBits == QuotedTender && QuotedPrice > 0 && QuotedTender >= QuotedPrice
        && QuotedItem->ProductId == QuotedProductId && !QuotedItem->IsActorBeingDestroyed()
        && QuotedItem->GetCarrier() == Customer->ShopInteraction && !QuotedItem->GetCurrentSlot()
        && Customer->ShopInteraction->GetHeldItem() == QuotedItem.Get();
}

void AShopTill::ResetTransaction()
{
    Phase = EShopTillPhase::Idle;
    QuotedCustomer = nullptr;
    QuotedItem = nullptr;
    TrayCoins.Empty();
    EnteredTotalText.Empty();
    QuotedPrice = 0;
    QuotedTender = 0;
    Feedback = FText::GetEmpty();
}

bool AShopTill::AssignCustomer(AShopCustomer* NewCustomer)
{
    if (!HasAuthority() || IsActorBeingDestroyed() || (NewCustomer && (!IsValid(NewCustomer)
        || NewCustomer->GetWorld() != GetWorld() || NewCustomer->IsActorBeingDestroyed())))
    {
        return false;
    }
    if (Customer != NewCustomer)
    {
        ResetTransaction();
        Customer = NewCustomer;
    }
    return true;
}

bool AShopTill::BeginCheckout(UShopInteractionComponent* Interactor)
{
    if (!HasAuthority() || !IsValid(Interactor) || Interactor->GetHeldItem() || !Interactor->CanReach(this))
    {
        return false;
    }
    if (Phase == EShopTillPhase::Complete && IsValid(Customer) && Customer == QuotedCustomer.Get())
    {
        return true;
    }
    if (IsQuoteValid())
    {
        return true;
    }
    ResetTransaction();
    if (!IsValid(Customer) || Customer->Activity != EShopCustomerActivity::ReadyToPay
        || Customer->PriceBits <= 0 || Customer->TenderBits < Customer->PriceBits)
    {
        Feedback = NSLOCTEXT("Shop", "TillCustomerNotReady", "Waiting for a customer with a valid payment");
        return false;
    }
    QuotedCustomer = Customer;
    QuotedItem = Customer->ShopInteraction->GetHeldItem();
    QuotedPrice = Customer->PriceBits;
    QuotedTender = Customer->TenderBits;
    QuotedProductId = Customer->RequestedProductId;
    QuotedProductName = Customer->RequestedProductName;
    if (!IsQuoteValid())
    {
        ResetTransaction();
        return false;
    }
    Phase = EShopTillPhase::EnteringTotal;
    return true;
}

bool AShopTill::SubmitTotal(const FString& Text)
{
    if (Phase != EShopTillPhase::EnteringTotal || !IsQuoteValid())
    {
        Feedback = NSLOCTEXT("Shop", "TillOrderChanged", "Order changed. Close and reopen the till.");
        return false;
    }
    EnteredTotalText = Text;
    const FString Trimmed = Text.TrimStartAndEnd();
    int64 Total = 0;
    const bool DigitsOnly = !Trimmed.IsEmpty()
        && Algo::AllOf(Trimmed, [](TCHAR Character) { return Character >= TEXT('0') && Character <= TEXT('9'); });
    if (!DigitsOnly || !LexTryParseString(Total, *Trimmed) || Total != QuotedPrice)
    {
        Feedback = NSLOCTEXT("Shop", "TillWrongTotal", "Check the item price and enter the correct total in bits.");
        return false;
    }
    Phase = EShopTillPhase::MakingChange;
    Feedback = NSLOCTEXT("Shop", "TillPaymentReceived", "Payment received. Count the customer's change into the tray.");
    return true;
}

int64 AShopTill::GetChangeTotal() const
{
    return int64(GetCoinCount(1)) + int64(GetCoinCount(10)) * 10 + int64(GetCoinCount(100)) * 100;
}

bool AShopTill::AddChangeCoin(int32 Denomination)
{
    if (Phase != EShopTillPhase::MakingChange || !IsQuoteValid()
        || (Denomination != 1 && Denomination != 10 && Denomination != 100)
        || GetChangeTotal() > MAX_int32 - Denomination)
    {
        return false;
    }
    ++TrayCoins.FindOrAdd(Denomination);
    UGameplayStatics::PlaySound2D(this, CoinSound, 0.14f);
    Feedback = FText::GetEmpty();
    return true;
}

bool AShopTill::RemoveChangeCoin(int32 Denomination)
{
    int32* Count = TrayCoins.Find(Denomination);
    if (Phase != EShopTillPhase::MakingChange || !IsQuoteValid() || !Count || *Count <= 0)
    {
        return false;
    }
    --*Count;
    UGameplayStatics::PlaySound2D(this, CoinSound, 0.14f, 0.9f);
    Feedback = FText::GetEmpty();
    return true;
}

bool AShopTill::CanCommitSale() const
{
    return Phase == EShopTillPhase::MakingChange && IsQuoteValid()
        && GetChangeTotal() == int64(QuotedTender) - QuotedPrice;
}

bool AShopTill::ConfirmSale(UShopInteractionComponent* Interactor)
{
    if (!HasAuthority() || !IsValid(Interactor) || Interactor->GetHeldItem() || !Interactor->CanReach(this))
    {
        return false;
    }
    if (!CanCommitSale())
    {
        Feedback = NSLOCTEXT("Shop", "TillCannotConfirm", "Check the order, payment and change tray before confirming.");
        return false;
    }
    AShopGameState* State = GetWorld()->GetGameState<AShopGameState>();
    if (!IsValid(State) || !State->TryCompleteSale(this))
    {
        Feedback = NSLOCTEXT("Shop", "TillPaymentFailed", "Payment could not be completed. The order is still pending.");
        return false;
    }
    Feedback = FText::Format(NSLOCTEXT("Shop", "TillReceipt", "Sale complete. Received {0} bits, returned {1} bits."),
        FText::AsNumber(QuotedTender), FText::AsNumber(GetChangeTotal()));
    UGameplayStatics::PlaySound2D(this, SaleSound, 0.22f);
    return true;
}

bool AShopTill::Interact_Implementation(UShopInteractionComponent* Interactor)
{
    AShopPlayerController* Controller = IsValid(Interactor) ? Cast<AShopPlayerController>(Interactor->GetOwner()) : nullptr;
    return Controller && BeginCheckout(Interactor) && Controller->OpenTill(this);
}

FText AShopTill::GetInteractionPrompt() const
{
    if (Phase == EShopTillPhase::Complete)
    {
        return NSLOCTEXT("Shop", "TillReviewReceipt", "E  Review receipt");
    }
    if (IsValid(Customer) && Customer->Activity == EShopCustomerActivity::ReadyToPay)
    {
        return NSLOCTEXT("Shop", "TillOpenCheckout", "E  Open till");
    }
    return NSLOCTEXT("Shop", "TillWaiting", "Till | Waiting for the customer to return");
}
