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
#include "ShopPlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "ShopInteractionComponent.h"
#include "ShopPickupItem.h"
#include "ShopShelfSlot.h"
#include "ShopShelfSection.h"

AShopCustomer::AShopCustomer()
{
    PrimaryActorTick.bCanEverTick = true;
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
    AIControllerClass = AAIController::StaticClass();
    GetCapsuleComponent()->InitCapsuleSize(30, 88);
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
    GetCharacterMovement()->MaxWalkSpeed = 130;
    GetCharacterMovement()->bUseRVOAvoidance = true;
    GetCharacterMovement()->AvoidanceConsiderationRadius = 180;
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
    RegularBadge = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RegularBadge"));
    RegularBadge->SetupAttachment(RootComponent);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> BadgeMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    RegularBadge->SetStaticMesh(BadgeMesh.Object);
    RegularBadge->SetRelativeLocation(FVector(14, -8, 36));
    RegularBadge->SetRelativeScale3D(FVector(0.13, 0.06, 0.18));
    RegularBadge->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    RegularBadge->SetCanEverAffectNavigation(false);
    RegularBadge->SetVisibility(false);
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
    return IsValid(Slot) && Slot->IsAvailableForSale() && Slot->GetOccupyingItem() == Item
        && (!Slot->GetSection() || Slot->GetSection()->GetFirstStockedSlot() == Slot);
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
                || (Hit.GetActor() != Item && Hit.GetActor() != *It && Hit.GetActor() != It->GetSection()))
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
        || HasResolvedVisit() || PriceBits <= 0 || ShopInteraction->GetHeldItem() || !IsEligibleStock(Item))
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
    PurchasedItemVisual->SetRelativeTransform(Item->ItemMesh->GetRelativeTransform());
    PurchasedItemVisual->SetRelativeScale3D(Item->ItemMesh->GetComponentScale());
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
    if (bCompleted && HasCompletedSale() && !bPurchaseReactionQueued)
    {
        bPurchaseReactionQueued = true;
        const FText Reactions[] = {
            NSLOCTEXT("Shop", "ThanksPractical", "Just what I needed. Much obliged."),
            NSLOCTEXT("Shop", "ThanksWarm", "Lovely. I'll make good use of this."),
            NSLOCTEXT("Shop", "ThanksQuiet", "That's my errand done. Thank you.")
        };
        QueueSpeech(Reactions[DialogueVariation], 0.1f + 0.08f * DialogueVariation);
    }
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
    if (!bArrivalQueued)
    {
        bArrivalQueued = true;
        const FText Requests[] = {
            NSLOCTEXT("Shop", "RequestPractical", "Morning! A bottle of {0}, please."),
            NSLOCTEXT("Shop", "RequestWarm", "Hello there. Have you any {0}?"),
            NSLOCTEXT("Shop", "RequestQuiet", "I've come for {0}, if you have some.")
        };
        FText Line = FText::Format(Requests[DialogueVariation], RequestedProductName);
        if (RegularId == TEXT("Ivo")) Line = FText::FromString(VisitDay == 1 ? TEXT("Ivo, courier. Dawn Tonic, please. The hills haven't got any shorter.") : TEXT("Morning again! A Dawn Tonic for the road, please."));
        if (RegularId == TEXT("Fen")) Line = FText::FromString(VisitDay == 1 ? TEXT("Fen, lamplighter. Moonlit Ink, please. I've left a lantern request in your book.") : TEXT("Hello again. Moonlit Ink for tonight's lanterns, please."));
        if (RegularId == TEXT("Nessa")) Line = FText::FromString(VisitDay == 1 ? TEXT("Nessa, the printer. A Dawn Tonic. These route cards won't print themselves.") : TEXT("Another Dawn Tonic, please. The Lantern Walk cards are nearly ready."));
        if (VisitDay >= 3 && RegularId == TEXT("Ivo"))
        {
            const AShopGameState* State = GetWorld()->GetGameState<AShopGameState>();
            Line = FText::FromString(State && State->GetLanternStory() == EShopLanternStory::Fulfilled
                ? TEXT("That lantern of Fen's is a fine sight. A Dawn Tonic for my route, please.")
                : TEXT("The bakery lent Fen a lantern. Payment in buns, apparently. Dawn Tonic, please."));
        }
        QueueSpeech(Line, 0.35f + 0.25f * DialogueVariation);
    }
    CheckoutLocation = Checkout;
    VisitDestination = Checkout;
    Activity = EShopCustomerActivity::Arriving;
    JourneyTime = 0;
    bLeftShop = false;
    MoveTo(VisitDestination);
    return true;
}

bool AShopCustomer::LeaveWithoutPurchase(const FVector& Exit)
{
    if (!HasAuthority() || HasResolvedVisit() || ShopInteraction->GetHeldItem()
        || (Activity != EShopCustomerActivity::Arriving && Activity != EShopCustomerActivity::Browsing
            && Activity != EShopCustomerActivity::Collecting) || Exit.ContainsNaN()) return false;
    TargetItem = nullptr;
    bLeavingWithoutPurchase = true;
    PendingSpeech = SpokenLine = FText::GetEmpty();
    SpeechRemaining = SpeechDelay = 0;
    VisitDestination = Exit;
    Activity = EShopCustomerActivity::Leaving;
    JourneyTime = 0;
    MoveTo(Exit);
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

bool AShopCustomer::HasGoodsOnCounter() const
{
    return CounterItem.IsValid() && CounterItem->GetCurrentSlot() && CounterItem->GetCurrentSlot()->GetCheckoutTill();
}

void AShopCustomer::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    AdvanceConversation(DeltaSeconds);
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
    if (HasResolvedVisit())
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
            Activity = bCheckoutAllowed ? EShopCustomerActivity::ReadyToPay : EShopCustomerActivity::Queued;
            if (AAIController* AI = Cast<AAIController>(GetController())) AI->StopMovement();
        }
        else if (JourneyTime > 3)
        {
            ReturnToCheckout();
        }
    }
    if ((Activity == EShopCustomerActivity::ReadyToPay || Activity == EShopCustomerActivity::Queued) && !ShopInteraction->GetHeldItem() && !HasGoodsOnCounter())
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
    if (RegularId != TEXT("Fen") || !Interactor || !Interactor->CanReach(this) || Interactor->GetHeldItem()) return false;
    if (AShopPlayerController* Player = Cast<AShopPlayerController>(Interactor->GetOwner())) return Player->OpenShopMenu(TEXT("Journal"));
    return false;
}

FText AShopCustomer::GetInteractionPrompt() const
{
    if (Activity == EShopCustomerActivity::Queued)
        return FText::Format(NSLOCTEXT("Shop", "CustomerQueued", "Waiting in line with {0}"), RequestedProductName);
    if (Activity == EShopCustomerActivity::Arriving)
    {
        return NSLOCTEXT("Shop", "CustomerArriving", "Coming in to browse");
    }
    if (Activity == EShopCustomerActivity::Leaving)
    {
        return HasCompletedSale() ? NSLOCTEXT("Shop", "CustomerLeaving", "Thank you! See you again.")
            : NSLOCTEXT("Shop", "CustomerLeavingUnserved", "Heading home");
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


void AShopCustomer::ConfigureVisitPersonality(int32 Variation)
{
    // Stable selection makes morning reloads repeat the same visit without global RNG.
    if (!bArrivalQueued && !bPurchaseReactionQueued)
        DialogueVariation = FMath::Abs(Variation % 3);
}

void AShopCustomer::QueueSpeech(const FText& Line, float Delay)
{
    SpokenLine = FText::GetEmpty();
    SpeechRemaining = 0;
    PendingSpeech = Line;
    SpeechDelay = Delay;
}

void AShopCustomer::AdvanceConversation(float DeltaSeconds)
{
    const float Step = FMath::Max(0.f, DeltaSeconds);
    SpeechRemaining = FMath::Max(0.f, SpeechRemaining - Step);
    if (SpeechRemaining <= 0) SpokenLine = FText::GetEmpty();
    if (!PendingSpeech.IsEmpty())
    {
        SpeechDelay -= Step;
        if (SpeechDelay <= 0)
        {
            SpokenLine = PendingSpeech;
            PendingSpeech = FText::GetEmpty();
            SpeechDuration = 4.3f + 0.4f * DialogueVariation;
            SpeechRemaining = FMath::Max(0.f, SpeechDuration + SpeechDelay);
            if (SpeechRemaining <= 0) SpokenLine = FText::GetEmpty();
        }
    }
}

float AShopCustomer::GetSpeechOpacity() const
{
    if (SpokenLine.IsEmpty()) return 0;
    return FMath::Clamp(FMath::Min(SpeechRemaining / 0.4f, (SpeechDuration - SpeechRemaining) / 0.15f), 0.f, 1.f);
}

void AShopCustomer::SetQueueDestination(const FVector& Destination, bool bFront)
{
    if (!HasAuthority() || HasResolvedVisit() || Destination.ContainsNaN()) return;
    if (CheckoutLocation.Equals(Destination, 0.1f) && bCheckoutAllowed == bFront) return;
    CheckoutLocation = Destination;
    bCheckoutAllowed = bFront;
    if (Activity == EShopCustomerActivity::Arriving)
    {
        VisitDestination = Destination;
        MoveTo(Destination);
    }
    else if (ShopInteraction->GetHeldItem()) ReturnToCheckout();
}

FText AShopCustomer::GetCustomerName() const
{
    return RegularId.IsNone() ? FText::FromString(TEXT("Customer")) : FText::FromName(RegularId);
}

void AShopCustomer::ConfigureRegular(int32 Index, int32 Day)
{
    static const FName Names[] = {TEXT("Ivo"), TEXT("Fen"), TEXT("Nessa")};
    static const TCHAR* Materials[] = {TEXT("/Game/Shop/Art/Materials/M_SagePaint.M_SagePaint"), TEXT("/Game/Shop/Art/Materials/M_Brass.M_Brass"), TEXT("/Game/Shop/Art/Materials/M_BookCloth.M_BookCloth")};
    Index = FMath::Clamp(Index, 0, 2);
    RegularId = Names[Index]; VisitDay = Day;
    RegularBadge->SetVisibility(true);
    RegularBadge->SetMaterial(0, LoadObject<UMaterialInterface>(nullptr, Materials[Index]));
    RegularBadge->SetRelativeScale3D(Index == 0 ? FVector(0.025, 0.14, 0.08) : Index == 1 ? FVector(0.025, 0.08, 0.15) : FVector(0.025, 0.12, 0.12));
}
