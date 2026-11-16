#include "ShopDoorSign.h"
#include "ShopGameState.h"
#include "ShopInteractionComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/MaterialInterface.h"

AShopDoorSign::AShopDoorSign()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.1f;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SignRoot"));
    Board = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Board"));
    Board->SetupAttachment(RootComponent);
    Board->SetRelativeScale3D(FVector(0.04, 0.6, 0.38));
    Board->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Board->SetCollisionResponseToAllChannels(ECR_Ignore);
    Board->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    Board->SetCanEverAffectNavigation(false);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Wood(TEXT("/Game/Shop/Art/Materials/M_DarkWood.M_DarkWood"));
    Board->SetStaticMesh(Cube.Object);
    Board->SetMaterial(0, Wood.Object);
    Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
    Label->SetupAttachment(RootComponent);
    Label->SetRelativeLocation(FVector(2.1, 0, 0));
    Label->SetHorizontalAlignment(EHTA_Center);
    Label->SetVerticalAlignment(EVRTA_TextCenter);
    Label->SetWorldSize(8);
    Label->SetTextRenderColor(FColor(245, 225, 170));
    Label->SetText(FText::FromString(TEXT("CLOSED")));
}

bool AShopDoorSign::Interact_Implementation(UShopInteractionComponent* Interactor)
{
    AShopGameState* State = GetWorld()->GetGameState<AShopGameState>();
    if (!HasAuthority() || !State || !IsValid(Interactor) || Interactor->GetHeldItem() || !Interactor->CanReach(this)) return false;
    return State->GetDayPhase() == EShopDayPhase::Preparation ? State->OpenShop() : State->CloseShop();
}

FText AShopDoorSign::GetInteractionPrompt() const
{
    const AShopGameState* State = GetWorld()->GetGameState<AShopGameState>();
    if (!State) return FText::GetEmpty();
    switch (State->GetDayPhase())
    {
    case EShopDayPhase::Preparation:
        if (State->HasDelivery()) return FText::FromString(TEXT("Unpack the delivery by the entrance before opening"));
        return FText::FromString(TEXT("E  Turn sign to OPEN"));
    case EShopDayPhase::Trading: return FText::FromString(TEXT("E  Turn sign to CLOSED | Finish the current visit"));
    case EShopDayPhase::Closing: return FText::FromString(TEXT("CLOSED | Waiting for the current customer to leave"));
    default: return FText::FromString(TEXT("CLOSED | Order supplies for tomorrow"));
    }
}

void AShopDoorSign::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const AShopGameState* State = GetWorld()->GetGameState<AShopGameState>();
    Label->SetText(FText::FromString(State && State->GetDayPhase() == EShopDayPhase::Trading ? TEXT("OPEN") : TEXT("CLOSED")));
}
