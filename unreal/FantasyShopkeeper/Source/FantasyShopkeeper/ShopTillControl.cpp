#include "ShopTillControl.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "ShopInteractionComponent.h"
#include "ShopTill.h"
#include "ShopCustomer.h"

AShopTillControl::AShopTillControl()
{
    HitBox = CreateDefaultSubobject<UBoxComponent>(TEXT("HitBox"));
    RootComponent = HitBox;
    HitBox->SetBoxExtent(FVector(5, 5, 3));
    HitBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    HitBox->SetCollisionResponseToAllChannels(ECR_Ignore);
    HitBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    HitBox->SetGenerateOverlapEvents(false);
    HitBox->SetCanEverAffectNavigation(false);
    Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
    Visual->SetupAttachment(RootComponent);
    Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Visual->SetCanEverAffectNavigation(false);
    Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
    Label->SetupAttachment(RootComponent);
    Label->SetRelativeLocation(FVector(0, 0, 1.6));
    Label->SetRelativeRotation(FRotator(90, -90, 0));
    Label->SetHorizontalAlignment(EHTA_Center);
    Label->SetVerticalAlignment(EVRTA_TextCenter);
    Label->SetWorldSize(4);
    Label->SetTextRenderColor(FColor(245, 225, 170));
}

bool AShopTillControl::Interact_Implementation(UShopInteractionComponent* Interactor)
{
    return IsValid(Till) && IsValid(Interactor) && Interactor->CanReach(this)
        && Till->UseControl(Interactor, Action, Value);
}

FText AShopTillControl::GetInteractionPrompt() const
{
    if (!IsValid(Till) || !IsValid(Till->Customer)) return FText::FromString(TEXT("Ledger | No customer at checkout"));
    if (Action == EShopTillControl::AddCoin || Action == EShopTillControl::RemoveCoin)
    {
        return FText::Format(Action == EShopTillControl::AddCoin
            ? NSLOCTEXT("Shop", "TakeTillCoin", "E  Put a {0}-bit coin in the change mat")
            : NSLOCTEXT("Shop", "ReturnTillCoin", "E  Return a {0}-bit coin to the till"), FText::AsNumber(Value));
    }
    if (Action == EShopTillControl::AcceptPayment)
    {
        return NSLOCTEXT("Shop", "TakePouch", "Payment already on the counter | Count the change");
    }
    if (Action == EShopTillControl::Confirm)
    {
        return NSLOCTEXT("Shop", "SealSale", "E  Stamp PAID with the counted change");
    }
    return FText::GetEmpty();
}
