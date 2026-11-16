#include "ShopBrewingUpgrade.h"
#include "ShopBrewingStation.h"
#include "ShopGameState.h"
#include "ShopInteractionComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AShopBrewingUpgrade::AShopBrewingUpgrade()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.1f;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Wood(TEXT("/Game/Shop/Art/Materials/M_DarkWood.M_DarkWood"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Brass(TEXT("/Game/Shop/Art/Materials/M_Brass.M_Brass"));
    Rack = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BottleRack"));
    Rack->SetupAttachment(RootComponent);
    Rack->SetStaticMesh(Cube.Object);
    Rack->SetMaterial(0, Brass.Object);
    Rack->SetRelativeLocation(FVector(0, 0, -1.5));
    Rack->SetRelativeScale3D(FVector(0.68, 0.28, 0.025));
    Rack->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Rack->SetCanEverAffectNavigation(false);
    UStaticMeshComponent* Plaque = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SelectorPlaque"));
    Plaque->SetupAttachment(RootComponent);
    Plaque->SetStaticMesh(Cube.Object);
    Plaque->SetMaterial(0, Wood.Object);
    Plaque->SetRelativeLocation(FVector(0, -15, -12));
    Plaque->SetRelativeScale3D(FVector(0.64, 0.025, 0.2));
    Plaque->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Plaque->SetCollisionResponseToAllChannels(ECR_Ignore);
    Plaque->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    Plaque->SetCanEverAffectNavigation(false);
    Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("BatchLabel"));
    Label->SetupAttachment(RootComponent);
    Label->SetRelativeLocation(FVector(0, -16.5, -12));
    Label->SetRelativeRotation(FRotator(0, -90, 0));
    Label->SetHorizontalAlignment(EHTA_Center);
    Label->SetVerticalAlignment(EVRTA_TextCenter);
    Label->SetWorldSize(3.3f);
    Label->SetTextRenderColor(FColor(245, 225, 170));
}

void AShopBrewingUpgrade::Configure(AShopBrewingStation* InStation) { Station = InStation; Tick(0); }

void AShopBrewingUpgrade::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const bool bOwned = IsValid(Station) && Station->HasBatchUpgrade();
    Rack->SetVisibility(bOwned);
    Label->SetText(FText::FromString(bOwned
        ? FString::Printf(TEXT("BATCH BOTTLING KIT\n%d bottle%s per brew\nE  Turn selector"), Station->GetBatchSize(), Station->GetBatchSize() == 1 ? TEXT("") : TEXT("s"))
        : FString::Printf(TEXT("BATCH BOTTLING KIT\nBrew three at a time\n%d bits / Fit in the evening"), AShopGameState::BatchUpgradePrice)));
}

FText AShopBrewingUpgrade::GetInteractionPrompt() const
{
    const AShopGameState* State = GetWorld()->GetGameState<AShopGameState>();
    if (!IsValid(Station)) return FText::FromString(TEXT("No brewing station connected"));
    if (Station->HasBatchUpgrade()) return FText::FromString(FString::Printf(TEXT("E  Select %d bottle%s per brew | Ingredients charged per bottle"), Station->GetBatchSize() == 3 ? 1 : 3, Station->GetBatchSize() == 3 ? TEXT("") : TEXT("s")));
    if (!State || !State->CanOrderSupplies()) return FText::FromString(TEXT("Batch bottling kit | Buy in the evening for 36 bits"));
    if (State->GetCash() < AShopGameState::BatchUpgradePrice) return FText::FromString(TEXT("Batch bottling kit costs 36 bits | Not enough cash"));
    return FText::FromString(TEXT("E  Buy batch bottling kit for 36 bits | Brew one or three"));
}

bool AShopBrewingUpgrade::Interact_Implementation(UShopInteractionComponent* Interactor)
{
    AShopGameState* State = GetWorld()->GetGameState<AShopGameState>();
    if (!HasAuthority() || !State || !IsValid(Station) || !IsValid(Interactor) || Interactor->GetHeldItem() || !Interactor->CanReach(this)) return false;
    const bool bChanged = Station->HasBatchUpgrade() ? Station->CycleBatchSize(Interactor) : State->TryBuyBatchUpgrade(Station);
    if (bChanged) Tick(0);
    return bChanged;
}
