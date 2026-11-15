#include "ShopTill.h"
#include "Algo/AllOf.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "ShopCustomer.h"
#include "ShopGameState.h"
#include "ShopInteractionComponent.h"
#include "ShopPickupItem.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/BoxComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

AShopTill::AShopTill()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.1f;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("TillRoot"));
    TillMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TillMesh"));
    TillMesh->SetupAttachment(RootComponent);
    TillMesh->SetRelativeLocation(FVector(0, 0, 10));
    TillMesh->SetRelativeScale3D(FVector(0.55, 0.32, 0.2));
    TillMesh->SetCollisionProfileName(TEXT("BlockAll"));
    Ticket = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Ticket"));
    Ticket->SetupAttachment(RootComponent);
    Ticket->SetRelativeLocation(FVector(0, 38, 152));
    Ticket->SetRelativeRotation(FRotator(0, -90, 0));
    Ticket->SetHorizontalAlignment(EHTA_Center);
    Ticket->SetWorldSize(4.5f);
    Ticket->SetTextRenderColor(FColor(32, 43, 22));
    for (int32 Index = 0; Index < 3; ++Index)
    {
        UInstancedStaticMeshComponent* Change = CreateDefaultSubobject<UInstancedStaticMeshComponent>(*FString::Printf(TEXT("ChangeCoins%d"), Index));
        Change->SetupAttachment(RootComponent);
        Change->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Change->SetCanEverAffectNavigation(false);
        ChangeVisuals.Add(Change);
        UInstancedStaticMeshComponent* Tender = CreateDefaultSubobject<UInstancedStaticMeshComponent>(*FString::Printf(TEXT("TenderCoins%d"), Index));
        Tender->SetupAttachment(RootComponent);
        Tender->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Tender->SetCanEverAffectNavigation(false);
        TenderVisuals.Add(Tender);
    }
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
    if (!HasAuthority() || IsActorBeingDestroyed() || !IsValid(Interactor) || Interactor->GetHeldItem() || !Interactor->CanReach(this))
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
        Feedback = NSLOCTEXT("Shop", "TillOrderChanged", "Order changed. Start checkout again.");
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
    RefreshPresentation();
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
    DisplayedChange = -1;
    RefreshPresentation();
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
    DisplayedChange = -1;
    RefreshPresentation();
    return true;
}

bool AShopTill::CanCommitSale() const
{
    return Phase == EShopTillPhase::MakingChange && IsQuoteValid()
        && GetChangeTotal() == int64(QuotedTender) - QuotedPrice;
}

bool AShopTill::ConfirmSale(UShopInteractionComponent* Interactor)
{
    if (!HasAuthority() || IsActorBeingDestroyed() || !IsValid(Interactor) || Interactor->GetHeldItem() || !Interactor->CanReach(this))
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
    const bool Result = BeginCheckout(Interactor);
    RefreshPresentation();
    return Result;
}

FText AShopTill::GetInteractionPrompt() const
{
    if (Phase == EShopTillPhase::Complete)
    {
        return NSLOCTEXT("Shop", "TillReviewReceipt", "Sale complete");
    }
    if (IsValid(Customer) && Customer->Activity == EShopCustomerActivity::ReadyToPay)
    {
        return NSLOCTEXT("Shop", "TillOpenCheckout", "E  Start checkout | Enter total on the counter keys");
    }
    return NSLOCTEXT("Shop", "TillWaiting", "Till | Waiting for the customer to return");
}


void AShopTill::BeginPlay()
{
    Super::BeginPlay();
    Ticket->SetVisibility(bPhysicalCounter);
    if (bPhysicalCounter)
    {
        BuildControls();
    }
    RefreshPresentation();
}

void AShopTill::EndPlay(const EEndPlayReason::Type Reason)
{
    for (AShopTillControl* Control : Controls)
    {
        if (IsValid(Control))
        {
            Control->Destroy();
        }
    }
    Controls.Empty();
    Super::EndPlay(Reason);
}

void AShopTill::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    RefreshPresentation();
}

void AShopTill::BuildControls()
{
    auto Add = [this](EShopTillControl Action, int32 Value, FVector Position, FVector Extent, UStaticMesh* Mesh, const FString& Text)
    {
        const FTransform Transform(GetActorQuat(), GetActorTransform().TransformPosition(Position));
        AShopTillControl* Control = GetWorld()->SpawnActor<AShopTillControl>(AShopTillControl::StaticClass(), Transform);
        if (!Control)
        {
            return;
        }
        Control->SetOwner(this);
        Control->Till = this;
        Control->Action = Action;
        Control->Value = Value;
        Control->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);
        Control->HitBox->SetBoxExtent(Extent);
        Control->Visual->SetStaticMesh(Mesh);
        Control->Label->SetText(FText::FromString(Text));
        if (Action == EShopTillControl::AddCoin)
        {
            UInstancedStaticMeshComponent* Stack = NewObject<UInstancedStaticMeshComponent>(Control);
            Control->AddInstanceComponent(Stack);
            Stack->SetupAttachment(Control->GetRootComponent());
            Stack->SetStaticMesh(Mesh);
            Stack->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Stack->SetCanEverAffectNavigation(false);
            Stack->RegisterComponent();
            for (int32 Coin = 1; Coin < 7; ++Coin)
            {
                Stack->AddInstance(FTransform(FVector(0, 0, Coin * 0.55)));
            }
            Control->Label->SetRelativeLocation(FVector(0, -12, 0));
        }
        Controls.Add(Control);
    };
    for (int32 Digit = 1; Digit <= 9; ++Digit)
    {
        Add(EShopTillControl::Digit, Digit, FVector(-34 + ((Digit - 1) % 3) * 12, -26 + ((Digit - 1) / 3) * 12, 103),
            FVector(5, 5, 3), KeyMesh, FString::FromInt(Digit));
    }
    Add(EShopTillControl::Digit, 0, FVector(-22, 10, 103), FVector(5, 5, 3), KeyMesh, TEXT("0"));
    Add(EShopTillControl::Clear, 0, FVector(-34, 10, 103), FVector(5, 5, 3), KeyMesh, TEXT("C"));
    Add(EShopTillControl::Confirm, 0, FVector(55, -34, 103), FVector(13, 7, 3), KeyMesh, TEXT("SALE"));
    Add(EShopTillControl::AcceptPayment, 0, FVector(50, 24, 112), FVector(14, 12, 14), PouchMesh, TEXT(""));
    if (AShopTillControl* Pouch = FindControl(EShopTillControl::AcceptPayment))
    {
        Pouch->Visual->SetRelativeLocation(FVector(0, 0, -12));
    }
    if (AShopTillControl* Confirm = FindControl(EShopTillControl::Confirm))
    {
        Confirm->Visual->SetRelativeScale3D(FVector(2.6, 1.4, 1));
    }
    const int32 Denominations[] = {1, 10, 100};
    for (int32 Index = 0; Index < 3; ++Index)
    {
        UStaticMesh* Mesh = CoinMeshes.IsValidIndex(Index) ? CoinMeshes[Index].Get() : nullptr;
        Add(EShopTillControl::AddCoin, Denominations[Index], FVector(-65, -24 + Index * 24, 103), FVector(9, 9, 4), Mesh,
            FString::FromInt(Denominations[Index]));
        Add(EShopTillControl::RemoveCoin, Denominations[Index], FVector(12 + Index * 18, -8, 103), FVector(8, 12, 8), nullptr, TEXT(""));
        ChangeVisuals[Index]->SetStaticMesh(Mesh);
        TenderVisuals[Index]->SetStaticMesh(Mesh);
    }
}

AShopTillControl* AShopTill::FindControl(EShopTillControl Action, int32 Value) const
{
    for (AShopTillControl* Control : Controls)
    {
        if (IsValid(Control) && Control->Action == Action && Control->Value == Value)
        {
            return Control;
        }
    }
    return nullptr;
}

bool AShopTill::UseControl(UShopInteractionComponent* Interactor, EShopTillControl Action, int32 Value)
{
    if (!HasAuthority() || IsActorBeingDestroyed() || !IsValid(Interactor) || Interactor->GetHeldItem() || !Interactor->CanReach(this)
        || !BeginCheckout(Interactor))
    {
        return false;
    }
    bool Result = false;
    if (Action == EShopTillControl::Digit && Phase == EShopTillPhase::EnteringTotal && Value >= 0 && Value <= 9)
    {
        if (EnteredTotalText.Len() < 10)
        {
            EnteredTotalText += FString::FromInt(Value);
            Feedback = FText::GetEmpty();
            Result = true;
        }
    }
    else if (Action == EShopTillControl::Clear && Phase == EShopTillPhase::EnteringTotal)
    {
        EnteredTotalText.Empty();
        Feedback = FText::GetEmpty();
        Result = true;
    }
    else if (Action == EShopTillControl::AcceptPayment)
    {
        Result = SubmitTotal(EnteredTotalText);
    }
    else if (Action == EShopTillControl::AddCoin)
    {
        Result = AddChangeCoin(Value);
    }
    else if (Action == EShopTillControl::RemoveCoin)
    {
        Result = RemoveChangeCoin(Value);
    }
    else if (Action == EShopTillControl::Confirm)
    {
        Result = ConfirmSale(Interactor);
    }
    RefreshPresentation();
    return Result;
}

int32 AShopTill::GetVisibleChangeCoins(int32 Denomination) const
{
    const int32 Index = Denomination == 1 ? 0 : Denomination == 10 ? 1 : Denomination == 100 ? 2 : INDEX_NONE;
    return Index == INDEX_NONE ? 0 : ChangeVisuals[Index]->GetInstanceCount();
}

void AShopTill::RefreshPresentation()
{
    if (!bPhysicalCounter)
    {
        return;
    }
    const bool bPaid = Phase == EShopTillPhase::MakingChange || Phase == EShopTillPhase::Complete;
    const int32 Tender = bPaid ? QuotedTender : 0;
    const int64 Change = GetChangeTotal();
    if (Change != DisplayedChange || Tender != DisplayedTender)
    {
        const int32 Denominations[] = {1, 10, 100};
        for (int32 Index = 0; Index < 3; ++Index)
        {
            UInstancedStaticMeshComponent* Coins = ChangeVisuals[Index];
            Coins->ClearInstances();
            for (int32 Coin = 0; Coin < FMath::Min(GetCoinCount(Denominations[Index]), 48); ++Coin)
            {
                Coins->AddInstance(FTransform(FVector(12 + Index * 18 + (Coin % 2 ? 3.2 : -3.2),
                    -8 + ((Coin / 2) % 2 ? 3.2 : -3.2), 101.5 + (Coin / 4) * 0.5)));
            }
            UInstancedStaticMeshComponent* Payment = TenderVisuals[Index];
            Payment->ClearInstances();
            const int32 Count = Index == 2 ? Tender / 100 : (Tender / Denominations[Index]) % 10;
            for (int32 Coin = 0; Coin < FMath::Min(Count, 48); ++Coin)
            {
                Payment->AddInstance(FTransform(FVector(46 + Index * 4, 24, 123 + Coin * 0.55)));
            }
            if (AShopTillControl* Lane = FindControl(EShopTillControl::RemoveCoin, Denominations[Index]))
            {
                Lane->Label->SetText(FText::Format(NSLOCTEXT("Shop", "CoinLaneCount", "{0} x {1}"),
                    FText::AsNumber(GetCoinCount(Denominations[Index])), FText::AsNumber(Denominations[Index])));
                Lane->Label->SetRelativeLocation(FVector(0, -17, 0));
            }
        }
        DisplayedChange = Change;
        DisplayedTender = Tender;
    }
    const FString Header = Phase == EShopTillPhase::Idle
        ? (IsValid(Customer) && Customer->Activity == EShopCustomerActivity::ReadyToPay
            ? FString::Printf(TEXT("%s  /  %d bits"), *Customer->RequestedProductName.ToString(), Customer->PriceBits)
            : TEXT("Waiting for customer"))
        : FString::Printf(TEXT("%s  /  %d bits"), *QuotedProductName.ToString(), QuotedPrice);
    FString Status = Feedback.ToString();
    // Keep feedback on the physical ticket inside its authored paper surface.
    TArray<FString> Words;
    Status.ParseIntoArrayWS(Words);
    Status.Empty();
    int32 LineLength = 0;
    for (const FString& Word : Words)
    {
        if (LineLength + Word.Len() > 58)
        {
            Status += TEXT("\n");
            LineLength = 0;
        }
        if (LineLength > 0)
        {
            Status += TEXT(" ");
            ++LineLength;
        }
        Status += Word;
        LineLength += Word.Len();
    }
    Ticket->SetText(FText::FromString(FString::Printf(TEXT("%s\nTotal %s   |   Payment %d\nChange %lld / %d bits\n%s"),
        *Header, EnteredTotalText.IsEmpty() ? TEXT("_") : *EnteredTotalText, Tender, Change,
        bPaid ? QuotedTender - QuotedPrice : 0, *Status)));
}
