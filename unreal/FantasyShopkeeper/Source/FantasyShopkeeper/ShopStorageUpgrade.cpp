#include "ShopStorageUpgrade.h"
#include "ShopBrewingStation.h"
#include "ShopGameState.h"
#include "ShopInteractionComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AShopStorageUpgrade::AShopStorageUpgrade()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.1f;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Wood(TEXT("/Game/Shop/Art/Materials/M_DarkWood.M_DarkWood"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Brass(TEXT("/Game/Shop/Art/Materials/M_Brass.M_Brass"));
    Cabinet = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Cabinet"));
    ExtraDrawers = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ExtraDrawers"));
    for (UStaticMeshComponent* Box : {Cabinet.Get(), ExtraDrawers.Get()})
    {
        Box->SetupAttachment(RootComponent);
        Box->SetStaticMesh(Cube.Object);
        Box->SetMaterial(0, Wood.Object);
        Box->SetRelativeScale3D(FVector(0.55, 0.4, 0.6));
        Box->SetCanEverAffectNavigation(false);
        Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        Box->SetCollisionResponseToAllChannels(ECR_Ignore);
        Box->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    }
    Cabinet->SetRelativeLocation(FVector(0, 0, 30));
    ExtraDrawers->SetRelativeLocation(FVector(0, 0, 90));
    // Brass pulls give the extra storage a visible purpose, using the existing kit.
    for (int32 Index = 0; Index < 6; ++Index)
    {
        // Leave the upper base drawer clear for the fitted price plaque.
        if (Index == 2) continue;
        UStaticMeshComponent* Pull = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("DrawerPull%d"), Index));
        Pull->SetupAttachment(Index < 3 ? Cabinet.Get() : ExtraDrawers.Get());
        Pull->SetStaticMesh(Cube.Object);
        Pull->SetMaterial(0, Brass.Object);
        Pull->SetRelativeLocation(FVector(0, -52, -32 + (Index % 3) * 32));
        Pull->SetRelativeScale3D(FVector(0.22, 0.06, 0.06));
        Pull->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Pull->SetCanEverAffectNavigation(false);
    }
    Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("StorageLabel"));
    Label->SetupAttachment(RootComponent);
    Label->SetRelativeLocation(FVector(0, -20.5, 43));
    Label->SetRelativeRotation(FRotator(0, -90, 0));
    Label->SetHorizontalAlignment(EHTA_Center);
    Label->SetVerticalAlignment(EVRTA_TextCenter);
    Label->SetWorldSize(3.5f);
    Label->SetTextRenderColor(FColor(245, 225, 170));
}
void AShopStorageUpgrade::Configure(AShopBrewingStation* InStation) { Station = InStation; Tick(0); }
bool AShopStorageUpgrade::IsExpansionVisible() const { return ExtraDrawers->IsVisible(); }
void AShopStorageUpgrade::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const bool bOwned = IsValid(Station) && Station->HasStorageUpgrade();
    ExtraDrawers->SetVisibility(bOwned, true);
    ExtraDrawers->SetCollisionEnabled(bOwned ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
    Label->SetText(FText::FromString(bOwned
        ? FString::Printf(TEXT("EXTRA STORAGE\n%d of each ingredient\nPAID"), Station->GetIngredientCapacity())
        : FString::Printf(TEXT("EXTRA DRAWERS\n%d to %d of each\n%d bits\nFit in the evening"), Station ? Station->GetIngredientCapacity() : 0,
            Station ? Station->CapacityForUpgrade(true) : 0, AShopGameState::StorageUpgradePrice)));
}
FText AShopStorageUpgrade::GetInteractionPrompt() const
{
    const AShopGameState* State = GetWorld()->GetGameState<AShopGameState>();
    if (!IsValid(Station)) return FText::FromString(TEXT("Storage is not connected to a brewing station"));
    if (Station->HasStorageUpgrade()) return FText::FromString(FString::Printf(TEXT("Expanded storage | %d of each ingredient"), Station->GetIngredientCapacity()));
    if (!State || !State->CanOrderSupplies()) return FText::FromString(TEXT("Extra ingredient drawers | Buy in the evening"));
    if (State->GetCash() < AShopGameState::StorageUpgradePrice) return FText::FromString(TEXT("Extra drawers cost 24 bits | Not enough cash"));
    return FText::FromString(TEXT("E  Buy extra ingredient drawers for 24 bits | Doubles capacity"));
}
bool AShopStorageUpgrade::Interact_Implementation(UShopInteractionComponent* Interactor)
{
    AShopGameState* State = GetWorld()->GetGameState<AShopGameState>();
    if (!HasAuthority() || !State || !IsValid(Interactor) || Interactor->GetHeldItem() || !Interactor->CanReach(this)
        || !State->TryBuyStorageUpgrade(Station)) return false;
    Tick(0);
    return true;
}
