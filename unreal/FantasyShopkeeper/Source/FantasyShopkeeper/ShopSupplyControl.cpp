#include "ShopSupplyControl.h"
#include "ShopSupplyCrate.h"
#include "ShopGameState.h"
#include "ShopInteractionComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

AShopSupplyControl::AShopSupplyControl()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.1f;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(RootComponent);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Parcel(TEXT("/Game/Shop/Art/Meshes/SM_SupplyCrate.SM_SupplyCrate"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Paper(TEXT("/Game/Shop/Art/Materials/M_CreamLabel.M_CreamLabel"));
    ParcelMesh = Parcel.Object;
    Mesh->SetStaticMesh(Cube.Object);
    Mesh->SetMaterial(0, Paper.Object);
    Mesh->SetRelativeScale3D(FVector(0.14, 0.12, 0.025));
    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    Mesh->SetCanEverAffectNavigation(false);
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Wood(TEXT("/Game/Shop/Art/Materials/M_DarkWood.M_DarkWood"));
    Lid = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SealedLid"));
    Lid->SetupAttachment(RootComponent);
    Lid->SetStaticMesh(Cube.Object);
    Lid->SetMaterial(0, Wood.Object);
    Lid->SetRelativeLocation(FVector(0, 0, 40));
    Lid->SetRelativeScale3D(FVector(0.5, 0.37, 0.17));
    ShippingTag = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShippingTag"));
    ShippingTag->SetupAttachment(RootComponent);
    ShippingTag->SetStaticMesh(Cube.Object);
    ShippingTag->SetMaterial(0, Paper.Object);
    ShippingTag->SetRelativeLocation(FVector(0, -18.8, 24));
    ShippingTag->SetRelativeScale3D(FVector(0.4, 0.01, 0.24));
    for (UStaticMeshComponent* Detail : {Lid.Get(), ShippingTag.Get()})
    {
        Detail->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Detail->SetCanEverAffectNavigation(false);
        Detail->SetVisibility(false);
    }
    Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
    Label->SetupAttachment(RootComponent);
    Label->SetRelativeLocation(FVector(0, 0, 1.4));
    Label->SetRelativeRotation(FRotator(90, -90, 0));
    Label->SetHorizontalAlignment(EHTA_Center);
    Label->SetVerticalAlignment(EVRTA_TextCenter);
    Label->SetWorldSize(2.8f);
    Label->SetTextRenderColor(FColor(32, 36, 24));
}
void AShopSupplyControl::Configure(AShopSupplyCrate* InDesk, EShopSupplyControl InAction)
{
    Desk = InDesk;
    Action = InAction;
    Lid->SetVisibility(Action == EShopSupplyControl::Delivery);
    ShippingTag->SetVisibility(Action == EShopSupplyControl::Delivery);
    const TCHAR* Names[] = {TEXT("PACK"), TEXT("LESS"), TEXT("MORE"), TEXT("REST"), TEXT("DELIVERY")};
    Label->SetText(FText::FromString(Names[int32(Action)]));
    if (Action == EShopSupplyControl::Delivery)
    {
        Mesh->SetStaticMesh(ParcelMesh);
        Mesh->SetMaterial(0, nullptr);
        Mesh->SetRelativeScale3D(FVector(0.7));
        Label->SetRelativeLocation(FVector(0, -19.4, 24));
        Label->SetRelativeRotation(FRotator(0, -90, 0));
        Label->SetWorldSize(3.5f);
        Label->SetTextRenderColor(FColor(32, 36, 24));
    }
    Tick(0);
}
void AShopSupplyControl::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const AShopGameState* State = GetWorld()->GetGameState<AShopGameState>();
    if (Action == EShopSupplyControl::Delivery)
    {
        const bool bVisible = State && State->HasDelivery();
        SetActorHiddenInGame(!bVisible);
        SetActorEnableCollision(bVisible);
        if (bVisible) Label->SetText(FText::FromString(FString::Printf(TEXT("PAID DELIVERY\nMoonleaf %d\nWater %d\nUnpack"), State->GetDeliveredQuantity(TEXT("Moonleaf")), State->GetDeliveredQuantity(TEXT("Spring Water")))));
    }
}
FText AShopSupplyControl::GetInteractionPrompt() const
{
    if (Action == EShopSupplyControl::Delivery) return FText::FromString(TEXT("E  Unpack paid supplies into storage"));
    if (Action == EShopSupplyControl::Rest) return FText::FromString(TEXT("E  Rest until morning | Confirm an order first"));
    return FText::FromString(TEXT("E  Turn the marked book tab"));
}
bool AShopSupplyControl::Interact_Implementation(UShopInteractionComponent* Interactor)
{
    AShopGameState* State = GetWorld()->GetGameState<AShopGameState>();
    if (!HasAuthority() || !State || State->IsOvernight() || !IsValid(Interactor)
        || Interactor->GetHeldItem() || !Interactor->CanReach(this)) return false;
    if (Action == EShopSupplyControl::Delivery)
    {
        if (!State->UnpackDelivery()) return false;
        if (Desk) UGameplayStatics::PlaySoundAtLocation(this, Desk->DeliverySound, GetActorLocation(), 0.4f);
        Tick(0);
        return true;
    }
    if (!IsValid(Desk)) return false;
    switch (Action)
    {
    case EShopSupplyControl::Pack: return Desk->CycleOffer(Interactor);
    case EShopSupplyControl::Less: return Desk->ChangeQuantity(Interactor, -1);
    case EShopSupplyControl::More: return Desk->ChangeQuantity(Interactor, 1);
    case EShopSupplyControl::Rest: return State->BeginOvernight();
    default: return false;
    }
}
