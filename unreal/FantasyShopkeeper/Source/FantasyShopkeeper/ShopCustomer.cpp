#include "ShopCustomer.h"

#include "AIController.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavigationPath.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "ShopGameState.h"
#include "ShopInteractionComponent.h"
#include "ShopPickupItem.h"
#include "ShopShelfSlot.h"

AShopCustomer::AShopCustomer()
{
    PrimaryActorTick.bCanEverTick = true;
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
    AIControllerClass = AAIController::StaticClass();
    GetCapsuleComponent()->InitCapsuleSize(30, 88);
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
    GetCharacterMovement()->MaxWalkSpeed = 130;
    GetCharacterMovement()->GetNavMovementProperties()->bUseAccelerationForPaths = true;
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0, 360, 0);
    bUseControllerRotationYaw = false;
    GetMesh()->SetRelativeLocation(FVector(0, 0, -88));
    GetMesh()->SetRelativeRotation(FRotator(0, -90, 0));
    GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
    InteractionBox->SetupAttachment(RootComponent);
    InteractionBox->SetBoxExtent(FVector(31, 31, 88));
    InteractionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    InteractionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
    InteractionBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    InteractionBox->SetGenerateOverlapEvents(false);
    InteractionBox->SetCanEverAffectNavigation(false);
    InteractionEyes = CreateDefaultSubobject<UCameraComponent>(TEXT("InteractionEyes"));
    InteractionEyes->SetupAttachment(RootComponent);
    InteractionEyes->SetRelativeLocation(FVector(0, 0, 56));
    CarryAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("CustomerCarryAnchor"));
    CarryAnchor->SetupAttachment(RootComponent);
    CarryAnchor->SetRelativeLocation(FVector(35, 15, 0));
    PurchasedItemVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PurchasedItemVisual"));
    PurchasedItemVisual->SetupAttachment(CarryAnchor);
    PurchasedItemVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PurchasedItemVisual->SetGenerateOverlapEvents(false);
    PurchasedItemVisual->SetCanEverAffectNavigation(false);
    PurchasedItemVisual->SetVisibility(false);
    ShopInteraction = CreateDefaultSubobject<UShopInteractionComponent>(TEXT("ShopInteraction"));
}

void AShopCustomer::BeginPlay()
{
    Super::BeginPlay();
    CheckoutLocation = GetActorLocation();
    ShopInteraction->SetCarryAnchor(CarryAnchor);
}

void AShopCustomer::EndPlay(const EEndPlayReason::Type Reason)
{
    ShopInteraction->ReleaseHeldItem();
    Super::EndPlay(Reason);
}

bool AShopCustomer::IsEligibleStock(const AShopPickupItem* Item) const
{
    if (!IsValid(Item) || Item->IsActorBeingDestroyed() || Item->GetWorld() != GetWorld()
        || RequestedProductId.IsNone() || Item->ProductId != RequestedProductId || Item->GetCarrier())
    {
        return false;
    }
    const AShopShelfSlot* Slot = Item->GetCurrentSlot();
    return IsValid(Slot) && Slot->bAvailableForSale && Slot->GetOccupyingItem() == Item;
}

AShopPickupItem* AShopCustomer::FindSaleItem() const
{
    if (!GetWorld())
    {
        return nullptr;
    }
    AShopPickupItem* Nearest = nullptr;
    double BestDistance = TNumericLimits<double>::Max();
    for (TActorIterator<AShopShelfSlot> It(GetWorld()); It; ++It)
    {
        AShopPickupItem* Item = It->GetOccupyingItem();
        const double Distance = IsValid(Item) ? FVector::DistSquared(GetActorLocation(), Item->GetActorLocation()) : BestDistance;
        if (IsEligibleStock(Item) && Distance < BestDistance)
        {
            Nearest = Item;
            BestDistance = Distance;
        }
    }
    return Nearest;
}

bool AShopCustomer::MoveTo(const FVector& Destination)
{
    const UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    if (!Navigation || !Navigation->GetDefaultNavDataInstance())
    {
        return false;
    }
    AAIController* AI = Cast<AAIController>(GetController());
    return AI && AI->MoveToLocation(Destination, 12, false, true, false, false, nullptr, false)
        != EPathFollowingRequestResult::Failed;
}

bool AShopCustomer::BeginCollecting()
{
    UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    if (!Navigation || !GetController() || PriceBits <= 0)
    {
        return false;
    }
    const FVector Offsets[] = {FVector(0, -100, 0), FVector(0, 100, 0), FVector(-100, 0, 0), FVector(100, 0, 0)};
    double BestLength = TNumericLimits<double>::Max();
    for (TActorIterator<AShopShelfSlot> It(GetWorld()); It; ++It)
    {
        AShopPickupItem* Item = It->GetOccupyingItem();
        if (!IsEligibleStock(Item))
        {
            continue;
        }
        for (const FVector& Offset : Offsets)
        {
            FNavLocation Approach;
            const FVector Candidate(Item->GetActorLocation().X + Offset.X, Item->GetActorLocation().Y + Offset.Y, CheckoutLocation.Z - 88);
            if (!Navigation->ProjectPointToNavigation(Candidate, Approach, FVector(35, 35, 100)))
            {
                continue;
            }
            const FVector Eyes = Approach.Location + FVector(0, 0, 144);
            const FVector Target = Item->GetActorLocation() + FVector(0, 0, 10);
            FHitResult Hit;
            FCollisionQueryParams Params(SCENE_QUERY_STAT(CustomerStock), false, this);
            GetWorld()->LineTraceSingleByChannel(Hit, Eyes, Target, ECC_Visibility, Params);
            if (FVector::Dist(Eyes, Target) > ShopInteraction->InteractionDistance
                || (Hit.GetActor() != Item && Hit.GetActor() != *It))
            {
                continue;
            }
            UNavigationPath* Path = Navigation->FindPathToLocationSynchronously(GetWorld(), GetActorLocation(), Approach.Location, this);
            if (Path && Path->IsValid() && !Path->IsPartial() && Path->GetPathLength() < BestLength)
            {
                TargetItem = Item;
                CollectionLocation = Approach.Location;
                BestLength = Path->GetPathLength();
            }
        }
    }
    if (!IsValid(TargetItem) || !MoveTo(CollectionLocation))
    {
        TargetItem = nullptr;
        return false;
    }
    Activity = EShopCustomerActivity::Collecting;
    JourneyTime = 0;
    return true;
}

bool AShopCustomer::TryCollectItem(AShopPickupItem* Item)
{
    if (!HasAuthority() || !HasActorBegunPlay() || IsActorBeingDestroyed()
        || HasCompletedSale() || PriceBits <= 0 || ShopInteraction->GetHeldItem() || !IsEligibleStock(Item))
    {
        return false;
    }
    InteractionEyes->SetWorldRotation((Item->GetActorLocation() + FVector(0, 0, 10) - InteractionEyes->GetComponentLocation()).Rotation());
    if (!Item->PickUp(ShopInteraction))
    {
        return false;
    }
    TargetItem = nullptr;
    ReturnToCheckout();
    return true;
}

void AShopCustomer::ReturnToCheckout()
{
    Activity = EShopCustomerActivity::Returning;
    JourneyTime = 0;
    MoveTo(CheckoutLocation);
}

void AShopCustomer::PreparePurchasePresentation(const AShopPickupItem* Item)
{
    if (!IsValid(PurchasedItemVisual) || !IsValid(Item) || !IsValid(Item->ItemMesh))
    {
        return;
    }
    PurchasedItemVisual->SetVisibility(false);
    PurchasedItemVisual->EmptyOverrideMaterials();
    PurchasedItemVisual->SetStaticMesh(Item->ItemMesh->GetStaticMesh());
    PurchasedItemVisual->SetRelativeTransform(Item->ItemMesh->GetComponentTransform().GetRelativeTransform(CarryAnchor->GetComponentTransform()));
    for (int32 Index = 0; Index < Item->ItemMesh->GetNumMaterials(); ++Index)
    {
        PurchasedItemVisual->SetMaterial(Index, Item->ItemMesh->GetMaterial(Index));
    }
}

void AShopCustomer::FinishPurchasePresentation(bool bCompleted)
{
    if (!IsValid(PurchasedItemVisual))
    {
        return;
    }
    PurchasedItemVisual->SetVisibility(bCompleted);
    if (!bCompleted)
    {
        PurchasedItemVisual->SetStaticMesh(nullptr);
        PurchasedItemVisual->EmptyOverrideMaterials();
    }
}

bool AShopCustomer::BeginVisit(const FVector& Checkout)
{
    if (!HasAuthority() || !HasActorBegunPlay() || HasCompletedSale() || ShopInteraction->GetHeldItem()
        || IsActorBeingDestroyed() || Checkout.ContainsNaN())
    {
        return false;
    }
    CheckoutLocation = Checkout;
    VisitDestination = Checkout;
    Activity = EShopCustomerActivity::Arriving;
    JourneyTime = 0;
    bLeftShop = false;
    MoveTo(VisitDestination);
    return true;
}

bool AShopCustomer::BeginDeparture(const FVector& Exit)
{
    if (!HasAuthority() || !HasCompletedSale() || ShopInteraction->GetHeldItem()
        || IsActorBeingDestroyed() || Exit.ContainsNaN())
    {
        return false;
    }
    VisitDestination = Exit;
    Activity = EShopCustomerActivity::Leaving;
    JourneyTime = 0;
    MoveTo(VisitDestination);
    return true;
}

void AShopCustomer::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (GetMesh()->GetSkeletalMeshAsset() && GetMesh()->DoesSocketExist(TEXT("hand_r")))
    {
        CarryAnchor->SetWorldLocation(GetMesh()->GetSocketLocation(TEXT("hand_r")) - FVector(0, 0, 20));
        CarryAnchor->SetWorldRotation(FRotator(0, GetActorRotation().Yaw, 0));
    }
    if (!HasAuthority())
    {
        return;
    }
    JourneyTime += DeltaSeconds;
    if (Activity == EShopCustomerActivity::Arriving || Activity == EShopCustomerActivity::Leaving)
    {
        if (FVector::Dist2D(GetActorLocation(), VisitDestination) < 25)
        {
            if (AAIController* AI = Cast<AAIController>(GetController()))
            {
                AI->StopMovement();
            }
            if (Activity == EShopCustomerActivity::Leaving)
            {
                bLeftShop = true;
            }
            else
            {
                Activity = EShopCustomerActivity::Browsing;
            }
        }
        else if (JourneyTime > 3)
        {
            MoveTo(VisitDestination);
            JourneyTime = 0;
        }
        return;
    }
    if (HasCompletedSale())
    {
        return;
    }
    if (Activity == EShopCustomerActivity::Returning)
    {
        if (!ShopInteraction->GetHeldItem())
        {
            Activity = EShopCustomerActivity::Browsing;
        }
        else if (FVector::Dist2D(GetActorLocation(), CheckoutLocation) < 20)
        {
            Activity = EShopCustomerActivity::ReadyToPay;
        }
        else if (JourneyTime > 3)
        {
            ReturnToCheckout();
        }
    }
    if (Activity == EShopCustomerActivity::ReadyToPay && !ShopInteraction->GetHeldItem())
    {
        Activity = EShopCustomerActivity::Browsing;
    }
    if (Activity == EShopCustomerActivity::Collecting)
    {
        if (!IsEligibleStock(TargetItem) || JourneyTime > 15)
        {
            if (AAIController* AI = Cast<AAIController>(GetController()))
            {
                AI->StopMovement();
            }
            TargetItem = nullptr;
            Activity = EShopCustomerActivity::Browsing;
            RetryAfter = 1;
        }
        else if (FVector::Dist2D(GetActorLocation(), CollectionLocation) < 20 && !TryCollectItem(TargetItem))
        {
            TargetItem = nullptr;
            Activity = EShopCustomerActivity::Browsing;
            RetryAfter = 1;
        }
    }
    RetryAfter -= DeltaSeconds;
    if (Activity == EShopCustomerActivity::Browsing && RetryAfter <= 0)
    {
        RetryAfter = 1;
        BeginCollecting();
    }
}

bool AShopCustomer::Interact_Implementation(UShopInteractionComponent* Interactor)
{
    return false;
}

FText AShopCustomer::GetInteractionPrompt() const
{
    if (Activity == EShopCustomerActivity::Arriving)
    {
        return NSLOCTEXT("Shop", "CustomerArriving", "Coming in to browse");
    }
    if (Activity == EShopCustomerActivity::Leaving)
    {
        return NSLOCTEXT("Shop", "CustomerLeaving", "Thank you! See you again.");
    }
    if (HasCompletedSale())
    {
        return FText::Format(NSLOCTEXT("Shop", "SaleComplete", "Sale complete (+{0} bits)"), FText::AsNumber(PaidBits));
    }
    if (PriceBits <= 0 || RequestedProductId.IsNone())
    {
        return NSLOCTEXT("Shop", "InvalidSale", "No valid order configured");
    }
    if (Activity == EShopCustomerActivity::Collecting || Activity == EShopCustomerActivity::Returning)
    {
        return NSLOCTEXT("Shop", "CustomerCollecting", "Collecting a potion | Meet me at checkout");
    }
    if (Activity == EShopCustomerActivity::ReadyToPay)
    {
        return FText::Format(NSLOCTEXT("Shop", "PayAtTill", "Ready to pay for {0} | Use the till"), RequestedProductName);
    }
    return FText::Format(NSLOCTEXT("Shop", "CustomerBrowsing", "Wants {0} for {1} bits | Stock a sale shelf"), RequestedProductName, FText::AsNumber(PriceBits));
}
