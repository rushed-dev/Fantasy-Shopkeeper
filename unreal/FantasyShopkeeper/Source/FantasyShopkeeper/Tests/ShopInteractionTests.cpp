#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Camera/CameraComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Pawn.h"
#include "../ShopInteractionComponent.h"
#include "../ShopPickupItem.h"
#include "../ShopShelfSlot.h"
#include "../ShopBrewingStation.h"
#include "../ShopBrewingRecipe.h"
#include "EngineUtils.h"
#include "../ShopCustomer.h"
#include "../ShopGameState.h"
#include "../ShopTill.h"
#include "../ShopSupplyCrate.h"
#include "../ShopCustomerFlow.h"
#include "../ShopShelfSection.h"
#include "../ShopMorningSave.h"
#include "../ShopDayLighting.h"
#include "../ShopDoorSign.h"
#include "../ShopSupplyControl.h"
#include "TimerManager.h"
#include "Engine/DirectionalLight.h"
#include "Components/LightComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

namespace
{
    struct FInteractionWorld
    {
        UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
        APawn* Pawn = nullptr;
        UCameraComponent* Camera = nullptr;
        UShopInteractionComponent* Interaction = nullptr;

        FInteractionWorld()
        {
            GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
            World->InitializeActorsForPlay(FURL());
            Pawn = World->SpawnActor<APawn>();
            Camera = NewObject<UCameraComponent>(Pawn);
            Pawn->SetRootComponent(Camera);
            Camera->RegisterComponent();
            Camera->Activate(true);
            Interaction = NewObject<UShopInteractionComponent>(Pawn);
            Interaction->RegisterComponent();
            Pawn->DispatchBeginPlay();
        }

        ~FInteractionWorld()
        {
            World->DestroyWorld(false);
            GEngine->DestroyWorldContext(World);
        }

        template <typename T>
        T* Spawn(const FVector& Location)
        {
            T* Actor = World->SpawnActor<T>(Location, FRotator::ZeroRotator);
            Actor->DispatchBeginPlay();
            return Actor;
        }

        void AimAt(const AActor* Actor)
        {
            Camera->SetWorldRotation((Actor->GetActorLocation() + FVector(0, 0, 10) - Camera->GetComponentLocation()).Rotation());
        }

        AShopBrewingStation* BrewingStation(int32 Moonleaf = 6, int32 Water = 3)
        {
            const FTransform Transform(FVector(100, 0, 0));
            AShopBrewingStation* Station = World->SpawnActorDeferred<AShopBrewingStation>(AShopBrewingStation::StaticClass(), Transform);
            Station->Recipe = NewObject<UShopBrewingRecipe>(Station);
            Station->Recipe->ProductId = TEXT("DawnTonic");
            Station->Recipe->DisplayName = FText::FromString(TEXT("Dawn Tonic"));
            Station->Recipe->IngredientCosts = {{TEXT("Moonleaf"), 2}, {TEXT("Spring Water"), 1}};
            Station->Recipe->OutputItemClass = AShopPickupItem::StaticClass();
            Station->StartingIngredients = {{TEXT("Moonleaf"), Moonleaf}, {TEXT("Spring Water"), Water}};
            Station->OutputSlot = Spawn<AShopShelfSlot>(FVector(100, 80, 0));
            Station->FinishSpawning(Transform);
            Station->DispatchBeginPlay();
            AimAt(Station);
            return Station;
        }

        AShopGameState* ShopState(int64 StartingCash = 0)
        {
            const FTransform Transform = FTransform::Identity;
            AShopGameState* State = World->SpawnActorDeferred<AShopGameState>(AShopGameState::StaticClass(), Transform);
            State->StartingCash = StartingCash;
            State->FinishSpawning(Transform);
            State->DispatchBeginPlay();
            World->SetGameState(State);
            return State;
        }

        void Evening(AShopGameState* State)
        {
            if (!State->IsDayInitialized())
            {
                Spawn<AShopCustomerFlow>(FVector::ZeroVector);
                State->InitializeDay(FString());
            }
            State->OpenShop();
            State->CloseShop();
        }

        AStaticMeshActor* Surface(const FVector& Location, const FVector& Scale)
        {
            const FTransform Transform(FRotator::ZeroRotator, Location, Scale);
            AStaticMeshActor* Surface = World->SpawnActorDeferred<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Transform);
            Surface->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
            Surface->GetStaticMeshComponent()->SetCollisionProfileName(TEXT("BlockAll"));
            Surface->FinishSpawning(Transform);
            Surface->DispatchBeginPlay();
            return Surface;
        }

        AShopPickupItem* BrewAndStock(AShopBrewingStation* Station, AShopShelfSlot* Shelf)
        {
            AimAt(Station);
            if (!Interaction->Interact())
            {
                return nullptr;
            }
            AShopPickupItem* Item = Station->OutputSlot->GetOccupyingItem();
            AimAt(Station->OutputSlot);
            if (!Interaction->Interact())
            {
                return nullptr;
            }
            AimAt(Shelf);
            return Interaction->Interact() ? Item : nullptr;
        }

        bool CollectAndReturn(AShopCustomer* Customer, AShopPickupItem* Item)
        {
            if (!IsValid(Item))
            {
                return false;
            }
            const FVector Checkout = Customer->GetActorLocation();
            Customer->SetActorLocation(Item->GetActorLocation() + FVector(0, -90, 0));
            const bool Collected = Customer->TryCollectItem(Item);
            Customer->SetActorLocation(Checkout);
            Customer->Tick(0);
            return Collected;
        }

        AShopTill* PreparedTill(AShopCustomer* Customer, AShopTill* ExistingTill = nullptr)
        {
            AShopTill* Till = ExistingTill ? ExistingTill : Spawn<AShopTill>(Customer->GetActorLocation() * 0.6);
            Till->AssignCustomer(Customer);
            AimAt(Till);
            if (!Till->BeginCheckout(Interaction))
            {
                return nullptr;
            }
            for (int32 Bit = 0; Bit < Customer->TenderBits - Customer->PriceBits; ++Bit)
            {
                Till->AddChangeCoin(1);
            }
            return Till;
        }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShopOwnershipTest, "FantasyShopkeeper.Interaction.OwnershipAndRetries",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShopOwnershipTest::RunTest(const FString& Parameters)
{
    FInteractionWorld Test;
    AShopPickupItem* Item = Test.Spawn<AShopPickupItem>(FVector(100, 0, 0));
    AShopPickupItem* OtherItem = Test.Spawn<AShopPickupItem>(FVector(100, -70, 0));
    AShopShelfSlot* Slot = Test.Spawn<AShopShelfSlot>(FVector(120, 60, 0));
    AShopShelfSlot* OtherSlot = Test.Spawn<AShopShelfSlot>(FVector(120, 110, 0));

    Test.AimAt(Item);
    TestTrue(TEXT("Camera trace picks up the first item"), Test.Interaction->Interact());
    TestEqual(TEXT("Held reference identifies the item"), Test.Interaction->GetHeldItem(), Item);
    TestEqual(TEXT("Item records its carrier"), Item->GetCarrier(), Test.Interaction);
    TestFalse(TEXT("Held item collision is disabled"), Item->GetActorEnableCollision());
    TestFalse(TEXT("Pickup retry is rejected"), Item->PickUp(Test.Interaction));

    Test.AimAt(OtherItem);
    TestFalse(TEXT("Cannot pick up a second item"), Test.Interaction->Interact());
    TestNull(TEXT("Second item remains unowned"), OtherItem->GetCarrier());

    Test.AimAt(Slot);
    TestTrue(TEXT("Trace places the item into a shelf slot"), Test.Interaction->Interact());
    TestNull(TEXT("Placement clears held reference"), Test.Interaction->GetHeldItem());
    TestEqual(TEXT("Slot owns the placed item"), Slot->GetOccupyingItem(), Item);
    TestEqual(TEXT("Item records its slot"), Item->GetCurrentSlot(), Slot);
    TestTrue(TEXT("Placed item collision is restored"), Item->GetActorEnableCollision());
    TestTrue(TEXT("Placement matches the slot anchor"), Item->GetActorLocation().Equals(Slot->PlacementPoint->GetComponentLocation(), 0.01));
    TestFalse(TEXT("Placement retry cannot duplicate stock"), Item->PlaceInSlot(Test.Interaction, OtherSlot));

    Test.AimAt(OtherItem);
    TestTrue(TEXT("Pick up the other item"), Test.Interaction->Interact());
    Test.AimAt(Slot);
    TestFalse(TEXT("An occupied slot rejects placement"), Test.Interaction->Interact());
    TestEqual(TEXT("Rejected item stays in hand"), Test.Interaction->GetHeldItem(), OtherItem);
    TestEqual(TEXT("Existing shelf ownership stays intact"), Slot->GetOccupyingItem(), Item);

    Test.AimAt(OtherSlot);
    TestTrue(TEXT("Place the other item into a free slot"), Test.Interaction->Interact());
    Test.AimAt(Slot);
    TestTrue(TEXT("Retrieve the original item through normal interaction"), Test.Interaction->Interact());
    TestNull(TEXT("Retrieval frees the old slot"), Slot->GetOccupyingItem());
    TestNull(TEXT("Retrieved item clears its slot reference"), Item->GetCurrentSlot());

    Item->Destroy();
    TestNull(TEXT("Destroying a held item frees the hand"), Test.Interaction->GetHeldItem());
    OtherItem->Destroy();
    TestNull(TEXT("Destroying a shelved item frees the slot"), OtherSlot->GetOccupyingItem());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShopReachTest, "FantasyShopkeeper.Interaction.ReachOcclusionAndCleanup",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShopReachTest::RunTest(const FString& Parameters)
{
    FInteractionWorld Test;
    AShopPickupItem* Item = Test.Spawn<AShopPickupItem>(FVector(300, 0, 0));
    Test.AimAt(Item);
    TestFalse(TEXT("Distant item cannot be picked up directly"), Item->PickUp(Test.Interaction));
    TestFalse(TEXT("Distant trace does not interact"), Test.Interaction->Interact());
    TestFalse(TEXT("Missing interactor is rejected"), Item->PickUp(nullptr));

    Item->SetActorLocation(FVector(100, 0, 0));
    AActor* Wall = Test.Spawn<AActor>(FVector(50, 0, 0));
    UBoxComponent* WallCollision = NewObject<UBoxComponent>(Wall);
    Wall->SetRootComponent(WallCollision);
    WallCollision->SetBoxExtent(FVector(5, 50, 50));
    WallCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    WallCollision->SetCollisionResponseToAllChannels(ECR_Block);
    WallCollision->RegisterComponent();
    Wall->SetActorLocation(FVector(50, 0, 0));
    Test.AimAt(Item);
    TestFalse(TEXT("Wall prevents direct pickup"), Item->PickUp(Test.Interaction));
    TestFalse(TEXT("Wall stops the interaction trace"), Test.Interaction->Interact());
    Wall->Destroy();

    const FVector OriginalScale(1.2, 0.8, 1.5);
    Item->SetActorScale3D(OriginalScale);
    TestTrue(TEXT("Item can be picked up when the wall is removed"), Test.Interaction->Interact());
    AShopShelfSlot* Slot = Test.Spawn<AShopShelfSlot>(FVector(120, 80, 0));
    Slot->SetActorScale3D(FVector(2.0));
    Test.AimAt(Slot);
    TestTrue(TEXT("Place onto a scaled slot"), Test.Interaction->Interact());
    TestTrue(TEXT("World scale is preserved"), Item->GetActorScale3D().Equals(OriginalScale, 0.01));
    Slot->Destroy();
    TestNull(TEXT("Destroying the slot releases the item"), Item->GetCurrentSlot());
    TestNull(TEXT("Released item detaches"), Item->GetAttachParentActor());
    TestTrue(TEXT("Released item remains interactable"), Item->GetActorEnableCollision());

    Test.AimAt(Item);
    TestTrue(TEXT("Pick up item from deleted slot"), Test.Interaction->Interact());
    Test.Interaction->DestroyComponent();
    TestNull(TEXT("Destroying interactor releases its item"), Item->GetCarrier());
    TestNull(TEXT("Released held item detaches"), Item->GetAttachParentActor());
    TestTrue(TEXT("Released held item restores collision"), Item->GetActorEnableCollision());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShopBrewingStockTest, "FantasyShopkeeper.Brewing.IngredientsAndPhysicalOutput",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShopBrewingStockTest::RunTest(const FString& Parameters)
{
    FInteractionWorld Test;
    AShopBrewingStation* Station = Test.BrewingStation();
    TestTrue(TEXT("Interact brews a batch"), Test.Interaction->Interact());
    AShopPickupItem* Product = Station->OutputSlot->GetOccupyingItem();
    if (!TestNotNull(TEXT("Batch produces a physical item"), Product))
    {
        return false;
    }
    TestEqual(TEXT("Exact leaf cost deducted"), Station->GetIngredientQuantity(TEXT("Moonleaf")), 4);
    TestEqual(TEXT("Exact water cost deducted"), Station->GetIngredientQuantity(TEXT("Spring Water")), 2);
    TestEqual(TEXT("Output identifies the recipe product"), Product->ProductId, FName(TEXT("DawnTonic")));
    TestEqual(TEXT("Output belongs to its slot"), Product->GetCurrentSlot(), Station->OutputSlot.Get());
    TestFalse(TEXT("Repeated input cannot brew into occupied output"), Test.Interaction->Interact());
    TestEqual(TEXT("Blocked brewing consumes nothing"), Station->GetIngredientQuantity(TEXT("Moonleaf")), 4);

    Test.AimAt(Station->OutputSlot);
    TestTrue(TEXT("Brewed output can be picked up normally"), Test.Interaction->Interact());
    Test.AimAt(Station);
    TestFalse(TEXT("Full hands reject brewing"), Station->TryBrew(Test.Interaction));
    AShopShelfSlot* Shelf = Test.Spawn<AShopShelfSlot>(FVector(120, -80, 0));
    Test.AimAt(Shelf);
    TestTrue(TEXT("Brewed potion can be shelved normally"), Test.Interaction->Interact());
    if (!TestNotNull(TEXT("Shelf receives the potion"), Shelf->GetOccupyingItem()))
    {
        return false;
    }
    TestEqual(TEXT("Shelf retains the potion's identity"), Shelf->GetOccupyingItem()->ProductId, FName(TEXT("DawnTonic")));

    for (int32 Batch = 0; Batch < 2; ++Batch)
    {
        Test.AimAt(Station);
        TestTrue(TEXT("Remaining supplies produce another batch"), Test.Interaction->Interact());
        if (AShopPickupItem* NextProduct = Station->OutputSlot->GetOccupyingItem())
        {
            NextProduct->Destroy();
        }
    }
    TestEqual(TEXT("Three batches use all leaves"), Station->GetIngredientQuantity(TEXT("Moonleaf")), 0);
    TestEqual(TEXT("Three batches use all water"), Station->GetIngredientQuantity(TEXT("Spring Water")), 0);
    TestFalse(TEXT("Exhausted supply rejects another batch"), Test.Interaction->Interact());
    TestNull(TEXT("No output appears without ingredients"), Station->OutputSlot->GetOccupyingItem());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShopBrewingValidationTest, "FantasyShopkeeper.Brewing.RejectedBatchesPreserveStock",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShopBrewingValidationTest::RunTest(const FString& Parameters)
{
    FInteractionWorld Test;
    AShopBrewingStation* Station = Test.BrewingStation(6, 0);
    TestFalse(TEXT("One missing ingredient rejects entire recipe"), Test.Interaction->Interact());
    TestEqual(TEXT("Available ingredient is not partially consumed"), Station->GetIngredientQuantity(TEXT("Moonleaf")), 6);
    TestNull(TEXT("Missing water produces no item"), Station->OutputSlot->GetOccupyingItem());

    Station->Recipe->IngredientCosts[TEXT("Moonleaf")] = -2;
    TestFalse(TEXT("Negative recipe cost is invalid"), Station->Recipe->IsRecipeValid());
    TestFalse(TEXT("Invalid cost cannot create ingredients"), Station->TryBrew(Test.Interaction));
    TestEqual(TEXT("Rejected negative cost preserves stock"), Station->GetIngredientQuantity(TEXT("Moonleaf")), 6);
    Station->Recipe->IngredientCosts[TEXT("Moonleaf")] = 0;
    TestFalse(TEXT("Zero ingredient cost is invalid"), Station->Recipe->IsRecipeValid());
    Station->Recipe->IngredientCosts.Empty();
    TestFalse(TEXT("An empty recipe is invalid"), Station->Recipe->IsRecipeValid());
    Station->Recipe->IngredientCosts.Add(NAME_None, 1);
    TestFalse(TEXT("An unidentified ingredient is invalid"), Station->Recipe->IsRecipeValid());
    Station->Recipe->IngredientCosts = {{TEXT("Moonleaf"), 2}};
    Station->Recipe->OutputItemClass = nullptr;
    TestFalse(TEXT("Missing output class is invalid"), Station->TryBrew(Test.Interaction));
    Station->Recipe->OutputItemClass = AShopPickupItem::StaticClass();
    Station->Recipe->ProductId = NAME_None;
    TestFalse(TEXT("Missing product ID is invalid"), Station->TryBrew(Test.Interaction));
    Station->Recipe->ProductId = TEXT("DawnTonic");
    Station->SetActorLocation(FVector(300, 0, 0));
    Test.AimAt(Station);
    TestFalse(TEXT("Distant station rejects direct brewing"), Station->TryBrew(Test.Interaction));
    Station->SetActorLocation(FVector(100, 0, 0));
    Test.AimAt(Station);
    Station->OutputSlot->Destroy();
    TestFalse(TEXT("Missing output slot rejects brewing"), Station->TryBrew(Test.Interaction));
    TestEqual(TEXT("All rejected batches preserve stock"), Station->GetIngredientQuantity(TEXT("Moonleaf")), 6);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShopBrewingRetryTest, "FantasyShopkeeper.Brewing.ReentrancyAndSpawnRollback",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShopBrewingRetryTest::RunTest(const FString& Parameters)
{
    FInteractionWorld Test;
    AShopBrewingStation* Station = Test.BrewingStation();
    bool Reentered = false;
    bool NestedSuccess = false;
    FDelegateHandle Handler = Test.World->AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateLambda([&](AActor* Actor)
    {
        if (Actor->IsA<AShopPickupItem>())
        {
            Reentered = true;
            NestedSuccess = Station->TryBrew(Test.Interaction);
        }
    }));
    TestTrue(TEXT("Outer batch succeeds"), Station->TryBrew(Test.Interaction));
    Test.World->RemoveOnActorSpawnedHandler(Handler);
    TestTrue(TEXT("Spawn callback attempted reentrant brewing"), Reentered);
    TestFalse(TEXT("Nested batch is rejected"), NestedSuccess);
    TestEqual(TEXT("Ingredients are charged once"), Station->GetIngredientQuantity(TEXT("Moonleaf")), 4);
    if (!TestNotNull(TEXT("Outer batch creates its output"), Station->OutputSlot->GetOccupyingItem()))
    {
        return false;
    }
    Station->OutputSlot->GetOccupyingItem()->Destroy();

    Handler = Test.World->AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateLambda([&](AActor* Actor)
    {
        if (Actor->IsA<AShopPickupItem>())
        {
            Station->OutputSlot->Destroy();
        }
    }));
    TestFalse(TEXT("Output destroyed during spawn rejects the batch"), Station->TryBrew(Test.Interaction));
    Test.World->RemoveOnActorSpawnedHandler(Handler);
    TestEqual(TEXT("Failed output commits no ingredients"), Station->GetIngredientQuantity(TEXT("Moonleaf")), 4);
    int32 Products = 0;
    for (TActorIterator<AShopPickupItem> It(Test.World); It; ++It)
    {
        ++Products;
    }
    TestEqual(TEXT("Failed batch leaves no loose product"), Products, 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShopSaleTest, "FantasyShopkeeper.Sale.PaymentAndRetries",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShopSaleTest::RunTest(const FString& Parameters)
{
    FInteractionWorld Test;
    AShopGameState* State = Test.ShopState();
    AShopBrewingStation* Station = Test.BrewingStation();
    AShopShelfSlot* FirstShelf = Test.Spawn<AShopShelfSlot>(FVector(120, -80, 0));
    AShopShelfSlot* SecondShelf = Test.Spawn<AShopShelfSlot>(FVector(100, -120, 0));
    AShopPickupItem* FirstItem = Test.BrewAndStock(Station, FirstShelf);
    AShopPickupItem* SecondItem = Test.BrewAndStock(Station, SecondShelf);
    if (!TestNotNull(TEXT("First tonic stocked"), FirstItem) || !TestNotNull(TEXT("Second tonic stocked"), SecondItem))
    {
        return false;
    }
    AShopCustomer* Customer = Test.Spawn<AShopCustomer>(FVector(0, 140, 0));
    AShopPickupItem* SoldItem = Customer->FindSaleItem();
    if (!TestNotNull(TEXT("Customer finds eligible stock"), SoldItem))
    {
        return false;
    }
    AShopPickupItem* RemainingItem = SoldItem == FirstItem ? SecondItem : FirstItem;
    AShopShelfSlot* SoldSlot = SoldItem->GetCurrentSlot();
    UStaticMesh* PurchasedMesh = SoldItem->ItemMesh->GetStaticMesh();
    TestTrue(TEXT("Customer collects through shared pickup mechanics"), Test.CollectAndReturn(Customer, SoldItem));
    TestNull(TEXT("Collection frees the stock position before payment"), SoldSlot->GetOccupyingItem());
    TestEqual(TEXT("Customer carries the selected physical item"), SoldItem->GetCarrier(), Customer->ShopInteraction.Get());
    AShopTill* Till = Test.PreparedTill(Customer);
    if (!TestNotNull(TEXT("Till is ready for payment confirmation"), Till))
    {
        return false;
    }
    TestTrue(TEXT("Till completes the customer's purchase"), Till->ConfirmSale(Test.Interaction));
    TestEqual(TEXT("Fixed payment credited exactly"), State->GetCash(), int64(12));
    TestTrue(TEXT("Customer records completed order"), Customer->HasCompletedSale());
    TestTrue(TEXT("Departing customer retains the purchased bottle presentation"), Customer->PurchasedItemVisual->GetStaticMesh() == PurchasedMesh);
    TestTrue(TEXT("Purchased bottle presentation is visible after payment"), Customer->PurchasedItemVisual->IsVisible());
    TestEqual(TEXT("Purchased presentation has no interaction collision"), Customer->PurchasedItemVisual->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
    TestFalse(TEXT("Purchased physical item leaves the world"), IsValid(SoldItem));
    TestNull(TEXT("Sale frees the item's slot"), SoldSlot->GetOccupyingItem());
    TestTrue(TEXT("Other matching stock stays available"), IsValid(RemainingItem));
    TestNotNull(TEXT("Remaining item keeps its shelf ownership"), RemainingItem->GetCurrentSlot());
    TestFalse(TEXT("Repeated confirmation cannot pay again"), Till->ConfirmSale(Test.Interaction));
    TestEqual(TEXT("Retry preserves cash"), State->GetCash(), int64(12));
    TestTrue(TEXT("Retry does not consume another matching item"), IsValid(RemainingItem));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShopSaleEligibilityTest, "FantasyShopkeeper.Sale.StockEligibilityAndRejection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShopSaleEligibilityTest::RunTest(const FString& Parameters)
{
    FInteractionWorld Test;
    AShopGameState* State = Test.ShopState();
    AShopBrewingStation* Station = Test.BrewingStation();
    AShopCustomer* Customer = Test.Spawn<AShopCustomer>(FVector(0, 140, 0));
    Test.AimAt(Customer);
    TestFalse(TEXT("No shelf stock means no sale"), Test.Interaction->Interact());
    Test.AimAt(Station);
    TestTrue(TEXT("Brew the requested product"), Test.Interaction->Interact());
    AShopPickupItem* Item = Station->OutputSlot->GetOccupyingItem();
    if (!TestNotNull(TEXT("Brewed item exists for eligibility checks"), Item))
    {
        return false;
    }
    TestFalse(TEXT("Brewing output is not a sale shelf"), Station->OutputSlot->bAvailableForSale);
    Test.AimAt(Customer);
    TestFalse(TEXT("Uncollected brewing output cannot be sold"), Test.Interaction->Interact());
    Test.AimAt(Station->OutputSlot);
    TestTrue(TEXT("Collect output"), Test.Interaction->Interact());
    Test.AimAt(Customer);
    TestFalse(TEXT("Held stock cannot be sold as shelf stock"), Test.Interaction->Interact());
    AShopShelfSlot* Shelf = Test.Spawn<AShopShelfSlot>(FVector(120, -80, 0));
    Shelf->bAvailableForSale = false;
    Test.AimAt(Shelf);
    TestTrue(TEXT("Place into a storage-only slot"), Test.Interaction->Interact());
    Test.AimAt(Customer);
    TestFalse(TEXT("Storage-only slot does not offer stock for sale"), Test.Interaction->Interact());
    Shelf->bAvailableForSale = true;
    Item->ProductId = TEXT("OtherPotion");
    TestFalse(TEXT("Wrong product is rejected"), Test.Interaction->Interact());
    Item->ProductId = TEXT("DawnTonic");
    TestTrue(TEXT("Customer collects eligible stock"), Test.CollectAndReturn(Customer, Item));
    AShopTill* Till = Test.PreparedTill(Customer);
    if (!TestNotNull(TEXT("Eligibility test has a prepared till"), Till))
    {
        return false;
    }
    Customer->PriceBits = 0;
    TestFalse(TEXT("Zero payment is rejected"), Till->ConfirmSale(Test.Interaction));
    Customer->PriceBits = -12;
    TestFalse(TEXT("Negative payment is rejected"), Till->ConfirmSale(Test.Interaction));
    Customer->PriceBits = 12;
    Customer->RequestedProductId = NAME_None;
    TestFalse(TEXT("Unidentified product request is rejected"), Till->ConfirmSale(Test.Interaction));
    Customer->RequestedProductId = TEXT("DawnTonic");
    Customer->SetActorLocation(FVector(0, 300, 0));
    TestFalse(TEXT("Distant customer rejects checkout"), Till->ConfirmSale(Test.Interaction));
    Customer->SetActorLocation(FVector(0, 140, 0));
    TestFalse(TEXT("Missing interactor cannot pay"), Till->ConfirmSale(nullptr));
    Test.World->SetGameState(nullptr);
    TestFalse(TEXT("Missing shop account rejects sale"), Till->ConfirmSale(Test.Interaction));
    Test.World->SetGameState(State);
    TestEqual(TEXT("Rejected sales preserve cash"), State->GetCash(), int64(0));
    TestEqual(TEXT("Rejected sales preserve unpaid counter stock"), Customer->CounterItem.Get(), Item);
    TestEqual(TEXT("Rejected stock remains reserved at this till"), Item->GetCurrentSlot()->GetCheckoutTill(), Till);
    TestFalse(TEXT("Rejected sales leave the order pending"), Customer->HasCompletedSale());
    Item->Destroy();
    TestFalse(TEXT("Removed stock cannot be sold"), Till->ConfirmSale(Test.Interaction));
    TestEqual(TEXT("Missing stock earns no money"), State->GetCash(), int64(0));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShopSaleAtomicityTest, "FantasyShopkeeper.Sale.ReentrantPaymentAndOverflow",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShopSaleAtomicityTest::RunTest(const FString& Parameters)
{
    FInteractionWorld Test;
    AShopGameState* State = Test.ShopState(12);
    AShopBrewingStation* Station = Test.BrewingStation();
    AShopSupplyCrate* Crate = Test.Spawn<AShopSupplyCrate>(FVector(-100, 80, 0));
    Crate->Station = Station;
    AShopPickupItem* FirstItem = Test.BrewAndStock(Station, Test.Spawn<AShopShelfSlot>(FVector(120, -80, 0)));
    AShopPickupItem* SecondItem = Test.BrewAndStock(Station, Test.Spawn<AShopShelfSlot>(FVector(100, -120, 0)));
    if (!TestNotNull(TEXT("First item ready for sale"), FirstItem) || !TestNotNull(TEXT("Second item ready for sale"), SecondItem))
    {
        return false;
    }
    AShopCustomer* Customer = Test.Spawn<AShopCustomer>(FVector(0, 140, 0));
    AShopCustomer* OtherCustomer = Test.Spawn<AShopCustomer>(FVector(-140, 0, 0));
    AShopPickupItem* SoldItem = Customer->FindSaleItem();
    TestTrue(TEXT("Outer customer carries a selected potion"), Test.CollectAndReturn(Customer, SoldItem));
    TestTrue(TEXT("Other customer carries the remaining potion"), Test.CollectAndReturn(OtherCustomer, OtherCustomer->FindSaleItem()));
    AShopTill* Till = Test.PreparedTill(Customer);
    AShopTill* OtherTill = Test.PreparedTill(OtherCustomer);
    if (!TestNotNull(TEXT("Outer till prepared"), Till) || !TestNotNull(TEXT("Nested till prepared"), OtherTill))
    {
        return false;
    }
    bool CallbackRan = false;
    bool OtherCustomerReachable = false;
    bool NestedSale = false;
    bool SupplyReachable = false;
    bool NestedDelivery = false;
    const FDelegateHandle Handler = Test.World->AddOnActorDestroyedHandler(FOnActorDestroyed::FDelegate::CreateLambda([&](AActor* Actor)
    {
        if (Actor == SoldItem)
        {
            CallbackRan = true;
            Test.AimAt(OtherTill);
            OtherCustomerReachable = Test.Interaction->CanReach(OtherTill);
            NestedSale = OtherTill->ConfirmSale(Test.Interaction);
            Test.AimAt(Crate);
            SupplyReachable = Test.Interaction->CanReach(Crate);
            NestedDelivery = Test.Interaction->Interact();
            Customer->PriceBits = 99;
        }
    }));
    Test.AimAt(Till);
    TestTrue(TEXT("Outer sale completes"), Till->ConfirmSale(Test.Interaction));
    Test.World->RemoveOnActorDestroyedHandler(Handler);
    TestTrue(TEXT("Destruction callback attempted another purchase"), CallbackRan);
    TestTrue(TEXT("Nested buyer was in reach"), OtherCustomerReachable);
    TestFalse(TEXT("Reentrant payment is blocked"), NestedSale);
    TestTrue(TEXT("Supply crate was reachable during the payment callback"), SupplyReachable);
    TestFalse(TEXT("A payment callback cannot make a nested supply purchase"), NestedDelivery);
    TestEqual(TEXT("Nested delivery cannot change ingredients"), Station->GetIngredientQuantity(TEXT("Moonleaf")), 2);
    TestFalse(TEXT("Nested customer's order stays pending"), OtherCustomer->HasCompletedSale());
    TestEqual(TEXT("Original quoted payment credited once"), State->GetCash(), int64(24));

    FInteractionWorld OverflowTest;
    AShopGameState* FullState = OverflowTest.ShopState(MAX_int64 - 5);
    AShopBrewingStation* OtherStation = OverflowTest.BrewingStation();
    AShopShelfSlot* Shelf = OverflowTest.Spawn<AShopShelfSlot>(FVector(120, -80, 0));
    AShopPickupItem* Item = OverflowTest.BrewAndStock(OtherStation, Shelf);
    AShopCustomer* Buyer = OverflowTest.Spawn<AShopCustomer>(FVector(0, 140, 0));
    TestTrue(TEXT("Overflow buyer collects stock"), OverflowTest.CollectAndReturn(Buyer, Item));
    AShopTill* OverflowTill = OverflowTest.PreparedTill(Buyer);
    if (!TestNotNull(TEXT("Overflow till prepared"), OverflowTill))
    {
        return false;
    }
    TestFalse(TEXT("Overflowing payment is rejected"), OverflowTill->ConfirmSale(OverflowTest.Interaction));
    TestEqual(TEXT("Overflow rejection preserves money"), FullState->GetCash(), MAX_int64 - 5);
    TestEqual(TEXT("Overflow rejection preserves unpaid counter stock"), Buyer->CounterItem.Get(), Item);
    TestEqual(TEXT("Overflow leaves goods on their counter"), Item->GetCurrentSlot()->GetCheckoutTill(), OverflowTill);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShopCustomerCollectionTest, "FantasyShopkeeper.Customer.CollectionOwnershipAndCleanup",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShopCustomerCollectionTest::RunTest(const FString& Parameters)
{
    FInteractionWorld Test;
    AShopBrewingStation* Station = Test.BrewingStation();
    AShopShelfSlot* Shelf = Test.Spawn<AShopShelfSlot>(FVector(120, -80, 0));
    AShopPickupItem* Item = Test.BrewAndStock(Station, Shelf);
    AShopCustomer* Buyer = Test.Spawn<AShopCustomer>(FVector(120, -170, 0));
    AShopCustomer* OtherBuyer = Test.Spawn<AShopCustomer>(FVector(20, -170, 0));
    if (!TestNotNull(TEXT("Stock for customer collection"), Item))
    {
        return false;
    }
    Shelf->bAvailableForSale = false;
    TestFalse(TEXT("Customer respects storage-only slots"), Buyer->TryCollectItem(Item));
    Shelf->bAvailableForSale = true;
    Item->ProductId = TEXT("OtherPotion");
    TestFalse(TEXT("Customer respects requested product identity"), Buyer->TryCollectItem(Item));
    Item->ProductId = TEXT("DawnTonic");
    AActor* Wall = Test.Spawn<AActor>(FVector(120, -125, 30));
    UBoxComponent* Collision = NewObject<UBoxComponent>(Wall);
    Wall->SetRootComponent(Collision);
    Collision->SetBoxExtent(FVector(30, 5, 50));
    Collision->SetCollisionProfileName(TEXT("BlockAll"));
    Collision->RegisterComponent();
    Wall->SetActorLocation(FVector(120, -125, 30));
    TestFalse(TEXT("Customer cannot collect through a wall"), Buyer->TryCollectItem(Item));
    TestEqual(TEXT("Blocked collection preserves shelf ownership"), Shelf->GetOccupyingItem(), Item);
    Wall->Destroy();
    TestTrue(TEXT("Customer uses ordinary pickup once the path is clear"), Buyer->TryCollectItem(Item));
    TestEqual(TEXT("Item records the customer's interaction component"), Item->GetCarrier(), Buyer->ShopInteraction.Get());
    TestNull(TEXT("Collection frees the original shelf"), Shelf->GetOccupyingItem());
    TestFalse(TEXT("Another customer cannot steal a carried item"), OtherBuyer->TryCollectItem(Item));
    Test.AimAt(Item);
    TestFalse(TEXT("Player cannot steal a carried item"), Item->PickUp(Test.Interaction));
    TestFalse(TEXT("Customer pickup retry is rejected"), Buyer->TryCollectItem(Item));
    Buyer->Destroy();
    TestNull(TEXT("Customer destruction releases its carried item"), Item->GetCarrier());
    TestNull(TEXT("Released item detaches"), Item->GetAttachParentActor());
    TestTrue(TEXT("Released item restores interaction collision"), Item->GetActorEnableCollision());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShopTillChangeTest, "FantasyShopkeeper.Till.CounterGoodsAndManualChange",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShopTillChangeTest::RunTest(const FString& Parameters)
{
    FInteractionWorld Test;
    AShopGameState* State = Test.ShopState();
    AShopBrewingStation* Station = Test.BrewingStation();
    AShopPickupItem* Item = Test.BrewAndStock(Station, Test.Spawn<AShopShelfSlot>(FVector(120, -80, 0)));
    AShopCustomer* Buyer = Test.Spawn<AShopCustomer>(FVector(0, 140, 0));
    Buyer->TenderBits = 123;
    if (!TestTrue(TEXT("Buyer collects the till order"), Test.CollectAndReturn(Buyer, Item)))
    {
        return false;
    }
    AShopTill* Till = Test.Spawn<AShopTill>(FVector(0, 84, 0));
    Till->Customer = Buyer;
    Test.AimAt(Till);
    TestTrue(TEXT("Till opens for the waiting buyer"), Till->BeginCheckout(Test.Interaction));
    TestEqual(TEXT("Goods and tender start change counting immediately"), Till->GetPhase(), EShopTillPhase::MakingChange);
    TestEqual(TEXT("The actual bottle is on this counter"), Item->GetCurrentSlot()->GetCheckoutTill(), Till);
    TestNull(TEXT("The customer releases their hand"), Buyer->ShopInteraction->GetHeldItem());
    TestFalse(TEXT("Player cannot steal unpaid counter goods"), Item->PickUp(Test.Interaction));
    TestFalse(TEXT("Other customers cannot collect counter goods"), Buyer->TryCollectItem(Item));
    TestEqual(TEXT("Presenting payment credits no money"), State->GetCash(), int64(0));
    TestFalse(TEXT("Unsupported coin denominations are rejected"), Till->AddChangeCoin(2));
    TestTrue(TEXT("Add one crown"), Till->AddChangeCoin(100));
    TestTrue(TEXT("Add one mark"), Till->AddChangeCoin(10));
    TestTrue(TEXT("Add one bit"), Till->AddChangeCoin(1));
    TestEqual(TEXT("Mixed denominations total correctly"), Till->GetChangeTotal(), int64(111));
    Till->AddChangeCoin(1);
    TestFalse(TEXT("Excess change blocks confirmation"), Till->ConfirmSale(Test.Interaction));
    TestTrue(TEXT("Change tray can be corrected"), Till->RemoveChangeCoin(1));
    TestTrue(TEXT("Reopening preserves the pending transaction"), Till->BeginCheckout(Test.Interaction));
    TestEqual(TEXT("Reopening preserves the counted coins"), Till->GetChangeTotal(), int64(111));
    TestEqual(TEXT("Reopening preserves offered tender"), Till->GetTender(), 123);
    Till->SetActorLocation(FVector(0, 400, 0));
    TestFalse(TEXT("Remote confirmation is rejected"), Till->ConfirmSale(Test.Interaction));
    Till->SetActorLocation(FVector(0, 84, 0));
    Test.AimAt(Till);
    TestTrue(TEXT("Correct change commits the sale"), Till->ConfirmSale(Test.Interaction));
    TestEqual(TEXT("Net receipt is the item price"), State->GetCash(), int64(12));
    TestFalse(TEXT("Repeated confirmation cannot pay twice"), Till->ConfirmSale(Test.Interaction));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShopTillInvalidationTest, "FantasyShopkeeper.Till.ChangedOrdersAndTenderValidation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShopTillInvalidationTest::RunTest(const FString& Parameters)
{
    FInteractionWorld Test;
    AShopGameState* State = Test.ShopState();
    AShopBrewingStation* Station = Test.BrewingStation();
    AShopShelfSlot* Shelf = Test.Spawn<AShopShelfSlot>(FVector(120, -80, 0));
    AShopPickupItem* Item = Test.BrewAndStock(Station, Shelf);
    AShopCustomer* Buyer = Test.Spawn<AShopCustomer>(FVector(0, 140, 0));
    if (!TestTrue(TEXT("Buyer collects order for invalidation checks"), Test.CollectAndReturn(Buyer, Item)))
    {
        return false;
    }
    AShopTill* Till = Test.Spawn<AShopTill>(FVector(0, 84, 0));
    Till->Customer = Buyer;
    Test.AimAt(Till);
    Buyer->TenderBits = 1;
    TestFalse(TEXT("Insufficient customer tender cannot start checkout"), Till->BeginCheckout(Test.Interaction));
    Buyer->TenderBits = 20;
    TestTrue(TEXT("Valid customer tender starts checkout"), Till->BeginCheckout(Test.Interaction));
    for (int32 Bit = 0; Bit < 8; ++Bit)
    {
        Till->AddChangeCoin(1);
    }
    Buyer->PriceBits = 13;
    TestFalse(TEXT("A changed quote cannot commit stale payment"), Till->ConfirmSale(Test.Interaction));
    Buyer->PriceBits = 12;
    Item->Destroy();
    TestFalse(TEXT("Destroyed carried stock cannot be paid for"), Till->ConfirmSale(Test.Interaction));
    Buyer->Tick(0);
    TestFalse(TEXT("Reopening a vanished order waits for replacement stock"), Till->BeginCheckout(Test.Interaction));
    TestEqual(TEXT("Invalidated order clears uncommitted change"), Till->GetChangeTotal(), int64(0));
    TestEqual(TEXT("Invalidated order credits no cash"), State->GetCash(), int64(0));

    Item = Test.BrewAndStock(Station, Shelf);
    TestTrue(TEXT("Buyer can collect a replacement order"), Test.CollectAndReturn(Buyer, Item));
    Test.AimAt(Till);
    TestTrue(TEXT("Till recovers with the replacement order"), Till->BeginCheckout(Test.Interaction));
    for (int32 Bit = 0; Bit < 8; ++Bit)
    {
        Till->AddChangeCoin(1);
    }
    AShopTill* OtherTill = Test.Spawn<AShopTill>(FVector(60, 84, 0));
    OtherTill->Customer = Buyer;
    Test.AimAt(OtherTill);
    TestFalse(TEXT("Another till cannot claim goods already on a counter"), OtherTill->BeginCheckout(Test.Interaction));
    for (int32 Bit = 0; Bit < 8; ++Bit)
    {
        OtherTill->AddChangeCoin(1);
    }
    Test.AimAt(Till);
    TestTrue(TEXT("First till commits once"), Till->ConfirmSale(Test.Interaction));
    Test.AimAt(OtherTill);
    TestFalse(TEXT("Second till cannot duplicate the same payment"), OtherTill->ConfirmSale(Test.Interaction));
    TestEqual(TEXT("Competing tills credit one purchase"), State->GetCash(), int64(12));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShopSupplyPurchaseTest, "FantasyShopkeeper.Supplies.AtomicPurchaseAndCapacity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShopSupplyPurchaseTest::RunTest(const FString& Parameters)
{
    FInteractionWorld Test;
    AShopGameState* State = Test.ShopState(36);
    AShopBrewingStation* Station = Test.BrewingStation(0, 0);
    Station->IngredientCapacity = 12;
    AShopSupplyCrate* Crate = Test.Spawn<AShopSupplyCrate>(FVector(-100, 0, 0));
    Crate->Station = Station;
    Test.AimAt(Crate);
    TestFalse(TEXT("Preparation cannot buy supplies"), Test.Interaction->Interact());
    Test.Evening(State);
    TestTrue(TEXT("Select two packs"), Crate->ChangeQuantity(Test.Interaction, 1));
    TestTrue(TEXT("Evening confirms the complete paid order"), Test.Interaction->Interact());
    TestEqual(TEXT("Order charges the advertised total"), State->GetCash(), int64(12));
    TestEqual(TEXT("No same-day ingredients appear"), Station->GetIngredientQuantity(TEXT("Moonleaf")), 0);
    TestFalse(TEXT("Repeated confirmation cannot pay twice"), Test.Interaction->Interact());
    TestTrue(TEXT("Morning receives the paid order"), State->AdvanceMorning());
    TestTrue(TEXT("Unpack the sealed morning parcel"), State->UnpackDelivery());
    TestEqual(TEXT("Delivery reaches capacity"), Station->GetIngredientQuantity(TEXT("Moonleaf")), 12);
    TestEqual(TEXT("Both packs delivered once"), Station->GetIngredientQuantity(TEXT("Spring Water")), 6);
    TestFalse(TEXT("Morning cannot repeat the delivery"), State->AdvanceMorning());
    Test.Evening(State);
    TestFalse(TEXT("Over-capacity order rejected"), Test.Interaction->Interact());
    TestEqual(TEXT("Rejected order costs nothing"), State->GetCash(), int64(12));
    AShopShelfSlot* Shelf = Test.Spawn<AShopShelfSlot>(FVector(120, -80, 0));
    TestNotNull(TEXT("Purchased ingredients brew a physical potion"), Test.BrewAndStock(Station, Shelf));
    TestEqual(TEXT("Brewing consumes the purchased Moonleaf"), Station->GetIngredientQuantity(TEXT("Moonleaf")), 10);
    TestEqual(TEXT("Brewing consumes the purchased water"), Station->GetIngredientQuantity(TEXT("Spring Water")), 5);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShopSupplyRejectionTest, "FantasyShopkeeper.Supplies.InvalidDeliveryAndInteraction",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShopSupplyRejectionTest::RunTest(const FString& Parameters)
{
    FInteractionWorld Test;
    AShopGameState* State = Test.ShopState(12);
    AShopBrewingStation* Station = Test.BrewingStation(0, 0);
    AShopSupplyCrate* Crate = Test.Spawn<AShopSupplyCrate>(FVector(-100, 0, 0));
    Crate->Station = Station;
    Test.Evening(State);
    Test.AimAt(Crate);
    Crate->Delivery.Add(TEXT("Spring Water"), -1);
    TestFalse(TEXT("One malformed quantity rejects the entire delivery"), Test.Interaction->Interact());
    TestEqual(TEXT("Malformed delivery leaves money intact"), State->GetCash(), int64(12));
    TestEqual(TEXT("Malformed delivery cannot partially deliver Moonleaf"), Station->GetIngredientQuantity(TEXT("Moonleaf")), 0);
    Crate->Delivery.Add(TEXT("Spring Water"), 3);
    Crate->PriceBits = 0;
    TestFalse(TEXT("Zero price is rejected"), Test.Interaction->Interact());
    Crate->PriceBits = -1;
    TestFalse(TEXT("Negative price cannot credit money"), Test.Interaction->Interact());
    Crate->PriceBits = 13;
    TestFalse(TEXT("Insufficient cash rejects the delivery"), Test.Interaction->Interact());
    Crate->PriceBits = 12;
    Crate->SetActorLocation(FVector(-400, 0, 0));
    TestFalse(TEXT("Direct interaction cannot buy distant supplies"), IShopInteractable::Execute_Interact(Crate, Test.Interaction));
    Crate->SetActorLocation(FVector(-100, 0, 0));
    AShopPickupItem* HeldItem = Test.Spawn<AShopPickupItem>(FVector(100, -70, 0));
    Test.AimAt(HeldItem);
    TestTrue(TEXT("Pickup for full-hands check succeeds"), Test.Interaction->Interact());
    Test.AimAt(Crate);
    TestFalse(TEXT("Supplies require empty hands"), Test.Interaction->Interact());
    HeldItem->Destroy();
    Crate->Delivery.Add(TEXT("Moonleaf"), MAX_int32);
    Station->IngredientCapacity = MAX_int32;
    TestTrue(TEXT("Maximum quantity can be stored without integer overflow"), Test.Interaction->Interact());
    TestEqual(TEXT("Exact payment leaves zero cash"), State->GetCash(), int64(0));
    TestFalse(TEXT("Repeated request cannot spend below zero"), Test.Interaction->Interact());
    Station->Destroy();
    TestFalse(TEXT("Destroyed station rejects direct delivery"), IShopInteractable::Execute_Interact(Crate, Test.Interaction));

    FInteractionWorld Overflow;
    AShopGameState* OverflowState = Overflow.ShopState(24);
    AShopBrewingStation* FullStation = Overflow.BrewingStation(MAX_int32 - 1, 0);
    FullStation->IngredientCapacity = MAX_int32;
    AShopSupplyCrate* OtherCrate = Overflow.Spawn<AShopSupplyCrate>(FVector(-100, 0, 0));
    OtherCrate->Station = FullStation;
    Overflow.Evening(OverflowState);
    Overflow.AimAt(OtherCrate);
    TestFalse(TEXT("Delivery addition cannot overflow a stored quantity"), Overflow.Interaction->Interact());
    TestEqual(TEXT("Overflow rejection preserves cash"), OverflowState->GetCash(), int64(24));
    TestEqual(TEXT("Overflow rejection preserves the full ingredient"), FullStation->GetIngredientQuantity(TEXT("Moonleaf")), MAX_int32 - 1);
    TestEqual(TEXT("Overflow rejection cannot partially deliver water"), FullStation->GetIngredientQuantity(TEXT("Spring Water")), 0);
    TestTrue(TEXT("Supplier can select a Moonleaf-only pack"), OtherCrate->CycleOffer(Overflow.Interaction));
    TestTrue(TEXT("Supplier can select a water-only pack"), OtherCrate->CycleOffer(Overflow.Interaction));
    TestEqual(TEXT("Individual water pack has its own price"), OtherCrate->PriceBits, 6);
    TestTrue(TEXT("Water can be replenished while Moonleaf storage is full"), Overflow.Interaction->Interact());
    TestEqual(TEXT("Individual pack preserves the full ingredient"), FullStation->GetIngredientQuantity(TEXT("Moonleaf")), MAX_int32 - 1);
    TestTrue(TEXT("Individual order delivered next morning"), OverflowState->AdvanceMorning());
    TestTrue(TEXT("Unpack the sealed morning parcel"), OverflowState->UnpackDelivery());
    TestEqual(TEXT("Individual water pack delivers its complete quantity"), FullStation->GetIngredientQuantity(TEXT("Spring Water")), 3);
    TestEqual(TEXT("Individual pack charges its advertised price"), OverflowState->GetCash(), int64(18));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShopCustomerFlowTest, "FantasyShopkeeper.Customer.RepeatedVisitsAndTillReset",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShopCustomerFlowTest::RunTest(const FString& Parameters)
{
    FInteractionWorld Test;
    AShopGameState* State = Test.ShopState();
    AShopBrewingStation* Station = Test.BrewingStation();
    AShopShelfSlot* Shelf = Test.Spawn<AShopShelfSlot>(FVector(120, -80, 0));
    AShopCustomer* Buyer = Test.Spawn<AShopCustomer>(FVector(0, 140, 0));
    AShopTill* Till = Test.Spawn<AShopTill>(FVector(0, 84, 0));
    Till->Customer = Buyer;
    const FTransform Transform = FTransform::Identity;
    AShopCustomerFlow* Flow = Test.World->SpawnActorDeferred<AShopCustomerFlow>(AShopCustomerFlow::StaticClass(), Transform);
    Flow->Till = Till;
    Flow->CustomerClass = AShopCustomer::StaticClass();
    Flow->bCheckoutFollowsTill = false;
    Flow->Entrance = FVector(-140, 140, 0);
    Flow->Checkout = Buyer->GetActorLocation();
    Flow->VisitInterval = 0;
    Flow->DepartureDelay = 0;
    Flow->FinishSpawning(Transform);
    Flow->DispatchBeginPlay();
    TestEqual(TEXT("Service adopts the placed first customer"), Flow->GetActiveCustomer(), Buyer);
    TestFalse(TEXT("A pending visit prevents a duplicate customer"), Flow->StartNextVisit());
    for (int32 Visit = 0; Visit < 3; ++Visit)
    {
        AShopPickupItem* Item = Test.BrewAndStock(Station, Shelf);
        if (!TestNotNull(TEXT("Each visit receives a physical brew"), Item)
            || !TestTrue(TEXT("Visitor collects the brew"), Test.CollectAndReturn(Buyer, Item)))
        {
            return false;
        }
        Test.AimAt(Till);
        TestTrue(TEXT("Each new visitor starts checkout"), Till->BeginCheckout(Test.Interaction));
        TestEqual(TEXT("New checkout starts with an empty tray"), Till->GetChangeTotal(), int64(0));
        for (int32 Bit = 0; Bit < Buyer->TenderBits - Buyer->PriceBits; ++Bit)
        {
            Till->AddChangeCoin(1);
        }
        TestTrue(TEXT("Visitor completes payment"), Till->ConfirmSale(Test.Interaction));
        Flow->Tick(0);
        Flow->Tick(0);
        TestEqual(TEXT("Visit is counted exactly once"), Flow->GetCompletedVisits(), Visit + 1);
        TestEqual(TEXT("Paid visitor begins walking out"), Buyer->Activity, EShopCustomerActivity::Leaving);
        Buyer->SetActorLocation(Flow->Entrance);
        Buyer->Tick(0);
        Flow->Tick(0);
        TestFalse(TEXT("Departed visitor is removed"), IsValid(Buyer));
        TestNull(TEXT("Departure clears the till customer"), Till->Customer.Get());
        Flow->Tick(0);
        Buyer = Flow->GetActiveCustomer();
        if (!TestNotNull(TEXT("Next customer is spawned"), Buyer))
        {
            return false;
        }
        TestEqual(TEXT("Till binds to the new customer"), Till->Customer.Get(), Buyer);
        TestEqual(TEXT("Till has cleared the previous receipt"), Till->GetPhase(), EShopTillPhase::Idle);
        TestEqual(TEXT("New customer begins by entering the shop"), Buyer->Activity, EShopCustomerActivity::Arriving);
        Buyer->SetActorLocation(Flow->Checkout);
        Buyer->Tick(0);
        TestEqual(TEXT("Arrival transitions to browsing"), Buyer->Activity, EShopCustomerActivity::Browsing);
    }
    TestEqual(TEXT("Three visits credit three sales"), State->GetCash(), int64(36));
    Flow->Destroy();
    TestFalse(TEXT("Removing the service cleans up its spawned customer"), IsValid(Buyer));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShopRecipeSelectionTest, "FantasyShopkeeper.Brewing.RecipeSelectionAndDistinctProducts",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShopRecipeSelectionTest::RunTest(const FString& Parameters)
{
    FInteractionWorld Test;
    AShopBrewingStation* Station = Test.BrewingStation();
    UShopBrewingRecipe* Tonic = Station->Recipe;
    UShopBrewingRecipe* Ink = NewObject<UShopBrewingRecipe>(Station);
    Ink->ProductId = TEXT("MoonlitInk");
    Ink->DisplayName = FText::FromString(TEXT("Moonlit Ink"));
    Ink->IngredientCosts = {{TEXT("Moonleaf"), 1}, {TEXT("Spring Water"), 2}};
    Ink->OutputItemClass = AShopPickupItem::StaticClass();
    UShopBrewingRecipe* Invalid = NewObject<UShopBrewingRecipe>(Station);
    Station->AvailableRecipes = {Tonic, nullptr, Invalid, Ink, Tonic};
    TestTrue(TEXT("First recipe brews through interaction"), Test.Interaction->Interact());
    AShopPickupItem* First = Station->OutputSlot->GetOccupyingItem();
    if (!TestNotNull(TEXT("First recipe has a physical output"), First))
    {
        return false;
    }
    TestTrue(TEXT("Recipe selection skips missing and invalid entries"), Station->CycleRecipe(Test.Interaction));
    TestEqual(TEXT("Second valid recipe is selected"), Station->Recipe.Get(), Ink);
    TestEqual(TEXT("Switching cannot transform an existing output"), First->ProductId, FName(TEXT("DawnTonic")));
    TestFalse(TEXT("Selected recipe cannot overwrite an occupied output"), Station->TryBrew(Test.Interaction));
    Test.AimAt(First);
    TestTrue(TEXT("First product can be collected"), Test.Interaction->Interact());
    Test.AimAt(Station);
    TestFalse(TEXT("Recipe switch requires empty hands"), Station->CycleRecipe(Test.Interaction));
    AShopShelfSlot* Shelf = Test.Spawn<AShopShelfSlot>(FVector(120, -80, 0));
    Test.AimAt(Shelf);
    TestTrue(TEXT("First product can be stocked"), Test.Interaction->Interact());
    Test.AimAt(Station);
    TestTrue(TEXT("Second recipe brews through interaction"), Test.Interaction->Interact());
    AShopPickupItem* Second = Station->OutputSlot->GetOccupyingItem();
    if (!TestNotNull(TEXT("Second recipe has a physical output"), Second))
    {
        return false;
    }
    TestEqual(TEXT("Second output has its own stable product ID"), Second->ProductId, FName(TEXT("MoonlitInk")));
    TestEqual(TEXT("Second output has the recipe display name"), Second->DisplayName.ToString(), FString(TEXT("Moonlit Ink")));
    TestEqual(TEXT("Both recipes charge their own Moonleaf cost"), Station->GetIngredientQuantity(TEXT("Moonleaf")), 3);
    TestEqual(TEXT("Both recipes charge their own water cost"), Station->GetIngredientQuantity(TEXT("Spring Water")), 0);
    TestTrue(TEXT("Recipe selection wraps to the tonic"), Station->CycleRecipe(Test.Interaction));
    TestEqual(TEXT("Wrapped recipe is the first valid entry"), Station->Recipe.Get(), Tonic);
    Station->AvailableRecipes = {nullptr, Invalid};
    TestFalse(TEXT("An invalid-only list preserves the selected recipe"), Station->CycleRecipe(Test.Interaction));
    Station->AvailableRecipes.Empty();
    TestFalse(TEXT("An empty recipe list is safe"), Station->CycleRecipe(Test.Interaction));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShopRecipeMutationTest, "FantasyShopkeeper.Brewing.ChangedRecipeRollsBackBatch",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShopRecipeMutationTest::RunTest(const FString& Parameters)
{
    FInteractionWorld Test;
    AShopBrewingStation* Station = Test.BrewingStation();
    bool ChangedRecipe = false;
    const FDelegateHandle Handler = Test.World->AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateLambda([&](AActor* Actor)
    {
        if (Actor->IsA<AShopPickupItem>())
        {
            ChangedRecipe = true;
            Station->Recipe->IngredientCosts.Add(TEXT("Moonleaf"), 1);
        }
    }));
    TestFalse(TEXT("A recipe edited during spawning rejects the batch"), Station->TryBrew(Test.Interaction));
    Test.World->RemoveOnActorSpawnedHandler(Handler);
    TestTrue(TEXT("Recipe mutation callback ran"), ChangedRecipe);
    TestEqual(TEXT("Rejected changed recipe preserves Moonleaf"), Station->GetIngredientQuantity(TEXT("Moonleaf")), 6);
    TestEqual(TEXT("Rejected changed recipe preserves water"), Station->GetIngredientQuantity(TEXT("Spring Water")), 3);
    TestNull(TEXT("Rejected changed recipe leaves the output empty"), Station->OutputSlot->GetOccupyingItem());
    for (TActorIterator<AShopPickupItem> It(Test.World); It; ++It)
    {
        AddError(TEXT("Changed recipe left a loose product behind"));
    }
    TestTrue(TEXT("The next request can brew the edited recipe"), Station->TryBrew(Test.Interaction));
    TestEqual(TEXT("Retry charges the edited Moonleaf cost once"), Station->GetIngredientQuantity(TEXT("Moonleaf")), 5);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShopCustomerOrdersTest, "FantasyShopkeeper.Customer.OrderConfigurationAndSpawnRetries",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShopCustomerOrdersTest::RunTest(const FString& Parameters)
{
    FInteractionWorld Test;
    AShopTill* Till = Test.Spawn<AShopTill>(FVector(0, 84, 0));
    AShopCustomerFlow* Flow = Test.Spawn<AShopCustomerFlow>(FVector::ZeroVector);
    Flow->Till = Till;
    Flow->bCheckoutFollowsTill = false;
    Flow->Entrance = FVector(-140, 140, 0);
    Flow->Checkout = FVector(0, 140, 0);
    Flow->VisitInterval = 0;
    FShopCustomerOrder Order;
    Order.ProductId = TEXT("MoonlitInk");
    Order.ProductName = FText::FromString(TEXT("Moonlit Ink"));
    Order.PriceBits = 18;
    Order.TenderBits = 50;
    Order.CustomerClass = AShopCustomer::StaticClass();
    Flow->Orders.Add(Order);
    Flow->Orders[0].TenderBits = 1;
    TestFalse(TEXT("Invalid tender prevents a visitor from spawning"), Flow->StartNextVisit());
    TestNull(TEXT("Invalid order leaves checkout empty"), Till->Customer.Get());
    Flow->Orders[0].TenderBits = 50;
    bool SpawnCallbackRan = false;
    bool NestedVisit = false;
    const FDelegateHandle Handler = Test.World->AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateLambda([&](AActor* Actor)
    {
        if (Actor->IsA<AShopCustomer>())
        {
            SpawnCallbackRan = true;
            NestedVisit = Flow->StartNextVisit();
        }
    }));
    TestTrue(TEXT("A valid per-order presentation class starts a visit"), Flow->StartNextVisit());
    Test.World->RemoveOnActorSpawnedHandler(Handler);
    TestTrue(TEXT("Visitor spawn callback ran"), SpawnCallbackRan);
    TestFalse(TEXT("Spawn callback cannot start a duplicate visit"), NestedVisit);
    AShopCustomer* Buyer = Flow->GetActiveCustomer();
    if (!TestNotNull(TEXT("Configured visitor exists"), Buyer))
    {
        return false;
    }
    TestEqual(TEXT("Visitor requests the configured product"), Buyer->RequestedProductId, FName(TEXT("MoonlitInk")));
    TestEqual(TEXT("Visitor receives the configured price"), Buyer->PriceBits, 18);
    TestEqual(TEXT("Visitor receives the configured tender"), Buyer->TenderBits, 50);
    Buyer->Destroy();
    TestNull(TEXT("Destroyed visitor is no longer exposed as active"), Flow->GetActiveCustomer());
    Flow->Tick(0);
    Buyer = Flow->GetActiveCustomer();
    TestNotNull(TEXT("A replacement visitor can retry an interrupted visit"), Buyer);
    TestEqual(TEXT("An interrupted unpaid visit is not counted"), Flow->GetCompletedVisits(), 0);
    TestFalse(TEXT("An active replacement prevents another spawn"), Flow->StartNextVisit());
    Buyer->Destroy();
    AShopCustomer* AssignedElsewhere = Test.Spawn<AShopCustomer>(FVector(0, 140, 0));
    Till->AssignCustomer(AssignedElsewhere);
    Flow->Tick(0);
    TestNull(TEXT("Service cannot overwrite another live checkout customer"), Flow->GetActiveCustomer());
    TestEqual(TEXT("Existing checkout assignment is preserved"), Till->Customer.Get(), AssignedElsewhere);
    AssignedElsewhere->Destroy();
    Flow->Tick(0);
    Buyer = Flow->GetActiveCustomer();
    TestNotNull(TEXT("Service recovers when the checkout becomes available"), Buyer);
    Flow->Destroy();
    TestFalse(TEXT("Service shutdown removes the spawned replacement"), IsValid(Buyer));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShopSetDownTest, "FantasyShopkeeper.Interaction.SetDownAndBlockedSurfaces",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShopSetDownTest::RunTest(const FString& Parameters)
{
    FInteractionWorld Test;
    Test.Camera->SetWorldLocation(FVector(0, 0, 100));
    Test.Surface(FVector(100, 0, -5), FVector(6, 6, 0.1));
    AShopPickupItem* Item = Test.Spawn<AShopPickupItem>(FVector(100, 0, 100));
    const FVector ItemScale(1.2, 0.8, 1.5);
    Item->SetActorScale3D(ItemScale);
    Test.AimAt(Item);
    TestTrue(TEXT("Pickup prepares an item for set-down"), Test.Interaction->Interact());
    TestFalse(TEXT("Missing interactor cannot set an item down"), Item->TrySetDown(nullptr));
    Test.Camera->SetWorldRotation(FRotator::ZeroRotator);
    TestFalse(TEXT("A view into empty air preserves the held item"), Test.Interaction->SetDownHeldItem());
    Test.Camera->SetWorldLocation(FVector(0, 0, 400));
    Test.Camera->SetWorldRotation(FRotator(-45, 0, 0));
    TestFalse(TEXT("A surface outside interaction range is rejected"), Test.Interaction->SetDownHeldItem());
    Test.Camera->SetWorldLocation(FVector(0, 0, 100));
    AStaticMeshActor* Wall = Test.Surface(FVector(112, 0, 15), FVector(0.12, 0.2, 0.3));
    TestFalse(TEXT("A nearby wall blocks the bottle footprint"), Test.Interaction->SetDownHeldItem());
    TestEqual(TEXT("Rejected set-down preserves the carried item"), Test.Interaction->GetHeldItem(), Item);
    TestFalse(TEXT("Rejected set-down keeps carried collision disabled"), Item->GetActorEnableCollision());
    Wall->Destroy();
    TestTrue(TEXT("A clear flat surface accepts the bottle"), Test.Interaction->SetDownHeldItem());
    TestNull(TEXT("Set-down frees the hand"), Test.Interaction->GetHeldItem());
    TestNull(TEXT("Set-down clears the carrier"), Item->GetCarrier());
    TestNull(TEXT("A loose bottle has no stock slot"), Item->GetCurrentSlot());
    TestNull(TEXT("Set-down detaches the actor"), Item->GetAttachParentActor());
    TestTrue(TEXT("Set-down restores interaction collision"), Item->GetActorEnableCollision());
    TestTrue(TEXT("Set-down preserves world scale"), Item->GetActorScale3D().Equals(ItemScale, 0.01));
    Item->ItemMesh->UpdateBounds();
    TestTrue(TEXT("The bottle base rests just above its support"),
        FMath::Abs(Item->ItemMesh->Bounds.Origin.Z - Item->ItemMesh->Bounds.BoxExtent.Z - 0.2) < 0.01);
    TestFalse(TEXT("Repeating set-down cannot act on an empty hand"), Test.Interaction->SetDownHeldItem());
    Test.AimAt(Item);
    TestTrue(TEXT("Loose bottle can be retrieved through normal interaction"), Test.Interaction->Interact());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShopSectionStockTest, "FantasyShopkeeper.ShelfSection.SequentialStockAndProductLock",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShopSectionStockTest::RunTest(const FString& Parameters)
{
    FInteractionWorld Test;
    AShopShelfSection* Section = Test.Spawn<AShopShelfSection>(FVector(100, 60, 0));
    for (int32 Index = 0; Index < 8; ++Index)
    {
        AShopPickupItem* Item = Test.Spawn<AShopPickupItem>(FVector(100, -70, 0));
        Item->ProductId = TEXT("DawnTonic");
        Test.AimAt(Item);
        if (!TestTrue(TEXT("Pick up stock for the section"), Test.Interaction->Interact()))
        {
            return false;
        }
        Test.AimAt(Section);
        if (Index == 1)
        {
            TestFalse(TEXT("Direct placement cannot skip earlier positions"), Item->PlaceInSlot(Test.Interaction, Section->GetSlot(7)));
        }
        TestTrue(TEXT("Whole-section interaction stocks the next position"), Test.Interaction->Interact());
        TestEqual(TEXT("The correct sequential slot owns the bottle"), Section->GetSlot(Index)->GetOccupyingItem(), Item);
        TestEqual(TEXT("Section count increases once per bottle"), Section->GetStockCount(), Index + 1);
        TestEqual(TEXT("Section locks to the first product"), Section->GetLockedProductId(), FName(TEXT("DawnTonic")));
        if (Index == 0)
        {
            AShopPickupItem* Wrong = Test.Spawn<AShopPickupItem>(FVector(100, -70, 0));
            Wrong->ProductId = TEXT("MoonlitInk");
            Test.AimAt(Wrong);
            TestTrue(TEXT("Pick up a different product"), Test.Interaction->Interact());
            Test.AimAt(Section);
            TestFalse(TEXT("Locked section rejects another product"), Test.Interaction->Interact());
            TestFalse(TEXT("Direct slot API also enforces the product lock"), Wrong->PlaceInSlot(Test.Interaction, Section->GetSlot(1)));
            TestEqual(TEXT("Rejected bottle stays carried"), Test.Interaction->GetHeldItem(), Wrong);
            Wrong->Destroy();
        }
    }
    AShopPickupItem* Extra = Test.Spawn<AShopPickupItem>(FVector(100, -70, 0));
    Extra->ProductId = TEXT("DawnTonic");
    Test.AimAt(Extra);
    Test.Interaction->Interact();
    Test.AimAt(Section);
    TestFalse(TEXT("A ninth bottle is rejected"), Test.Interaction->Interact());
    TestEqual(TEXT("Full section retains exactly eight bottles"), Section->GetStockCount(), 8);
    Extra->Destroy();
    Section->GetSlot(0)->GetOccupyingItem()->Destroy();
    TestEqual(TEXT("Destruction frees the first position"), Section->GetNextFreeSlot(), Section->GetSlot(0));
    TestEqual(TEXT("Remaining stock keeps the section locked"), Section->GetLockedProductId(), FName(TEXT("DawnTonic")));
    for (int32 Index = 1; Index < 8; ++Index)
    {
        Test.AimAt(Section);
        AShopPickupItem* Expected = Section->GetSlot(Index)->GetOccupyingItem();
        TestTrue(TEXT("Whole-section retrieval takes the first stocked position"), Test.Interaction->Interact());
        TestEqual(TEXT("Retrieved bottle follows stock order"), Test.Interaction->GetHeldItem(), Expected);
        Expected->Destroy();
    }
    TestTrue(TEXT("An empty section releases its product assignment"), Section->GetLockedProductId().IsNone());
    AShopPickupItem* Ink = Test.Spawn<AShopPickupItem>(FVector(100, -70, 0));
    Ink->ProductId = TEXT("MoonlitInk");
    Test.AimAt(Ink);
    Test.Interaction->Interact();
    Test.AimAt(Section);
    TestTrue(TEXT("Emptied section can be reassigned to another product"), Test.Interaction->Interact());
    TestEqual(TEXT("New product fills the first position"), Section->GetSlot(0)->GetOccupyingItem(), Ink);
    Section->Destroy();
    TestNull(TEXT("Removing the section releases its stock"), Ink->GetCurrentSlot());
    TestTrue(TEXT("Released stock survives as a loose item"), IsValid(Ink));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShopSectionCustomerTest, "FantasyShopkeeper.ShelfSection.SharedCustomerPickupAndReach",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShopSectionCustomerTest::RunTest(const FString& Parameters)
{
    FInteractionWorld Test;
    AShopShelfSection* Section = Test.Spawn<AShopShelfSection>(FVector(100, 60, 0));
    for (int32 Index = 0; Index < 2; ++Index)
    {
        AShopPickupItem* Item = Test.Spawn<AShopPickupItem>(FVector(100, -70, 0));
        Item->ProductId = TEXT("DawnTonic");
        Test.AimAt(Item);
        Test.Interaction->Interact();
        Test.AimAt(Section);
        TestTrue(TEXT("Prepare matching section stock"), Test.Interaction->Interact());
    }
    AShopCustomer* Customer = Test.Spawn<AShopCustomer>(FVector(0, 140, 0));
    TestFalse(TEXT("Customer cannot skip the first stocked position"), Customer->TryCollectItem(Section->GetSlot(1)->GetOccupyingItem()));
    TestEqual(TEXT("Customer selects the first stocked bottle"), Customer->FindSaleItem(), Section->GetSlot(0)->GetOccupyingItem());
    TestTrue(TEXT("Customer uses the shared pickup operation through the section"), Customer->TryCollectItem(Customer->FindSaleItem()));
    TestEqual(TEXT("Customer collection frees a section position"), Section->GetStockCount(), 1);
    Section->bAvailableForSale = false;
    TestNull(TEXT("Storage sections are excluded from sale selection"), Customer->FindSaleItem());
    Section->SetActorLocation(FVector(500, 60, 0));
    Test.AimAt(Section);
    TestFalse(TEXT("Whole-section retrieval still enforces interaction distance"), Test.Interaction->Interact());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShopPhysicalTillTest, "FantasyShopkeeper.Till.PhysicalControlsAndCoinPresentation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShopPhysicalTillTest::RunTest(const FString& Parameters)
{
    FInteractionWorld Test;
    AShopGameState* State = Test.ShopState();
    AShopBrewingStation* Station = Test.BrewingStation();
    AShopShelfSlot* Shelf = Test.Spawn<AShopShelfSlot>(FVector(120, 60, 0));
    AShopPickupItem* Item = Test.BrewAndStock(Station, Shelf);
    AShopCustomer* Buyer = Test.Spawn<AShopCustomer>(FVector(0, 140, 0));
    if (!TestTrue(TEXT("Customer returns with a product for physical checkout"), Test.CollectAndReturn(Buyer, Item)))
    {
        return false;
    }
    const FTransform Transform(FVector(0, 84, 0));
    AShopTill* Till = Test.World->SpawnActorDeferred<AShopTill>(AShopTill::StaticClass(), Transform);
    Till->bPhysicalCounter = true;
    Till->Customer = Buyer;
    Till->FinishSpawning(Transform);
    Till->DispatchBeginPlay();
    Test.Camera->SetWorldLocation(FVector(0, -60, 155));
    auto Press = [&](EShopTillControl Action, int32 Value = 0)
    {
        AShopTillControl* Control = Till->FindControl(Action, Value);
        if (!Control)
        {
            return false;
        }
        Test.Camera->SetWorldRotation((Control->GetActorLocation() - Test.Camera->GetComponentLocation()).Rotation());
        TestEqual(TEXT("The aimed physical control is the actual trace target"), Test.Interaction->GetInteractionTarget(), static_cast<AActor*>(Control));
        return Test.Interaction->Interact();
    };
    TestTrue(TEXT("Customer puts their goods and payment on the desk"), Till->PresentCustomerGoods());
    TestEqual(TEXT("No price-entry phase"), Till->GetPhase(), EShopTillPhase::MakingChange);
    TestTrue(TEXT("Goods have a visible travel presentation"), Till->IsPresentingGoods());
    TestFalse(TEXT("Reserved item is not drawn twice"), Item->ItemMesh->IsVisible());
    TestEqual(TEXT("Animation does not change reserved ownership"), Item->GetCurrentSlot()->GetCheckoutTill(), Till);
    Till->AdvancePresentation(0.2f);
    TestTrue(TEXT("Placement takes time"), Till->IsPresentingGoods());
    Till->AdvancePresentation(0.3f);
    TestFalse(TEXT("Bottle settles on the mat"), Till->IsPresentingGoods());
    TestTrue(TEXT("Real item becomes visible after settling"), Item->ItemMesh->IsVisible());
    Till->AdvancePresentation(0.7f);
    TestEqual(TEXT("Two tender coins each clink once"), Till->GetPaymentClinkCount(), 2);
    Till->PresentCustomerGoods(); Till->AdvancePresentation(3.f);
    TestEqual(TEXT("Repeated presentation cannot replay payment clinks"), Till->GetPaymentClinkCount(), 2);
    TestFalse(TEXT("Wrong change is still rejected"), Press(EShopTillControl::Confirm));
    TestTrue(TEXT("Mismatch gives feedback without supplying the answer"), Till->GetFeedback().ToString().Contains(TEXT("does not balance")));

    TestTrue(TEXT("A physical bit stack supplies a coin"), Press(EShopTillControl::AddCoin, 1));
    TestEqual(TEXT("One tray coin is rendered"), Till->GetVisibleChangeCoins(1), 1);
    TestTrue(TEXT("A tray lane returns its coin"), Press(EShopTillControl::RemoveCoin, 1));
    TestEqual(TEXT("Returned coin leaves the tray presentation"), Till->GetVisibleChangeCoins(1), 0);
    TestFalse(TEXT("An empty tray lane cannot remove another coin"), Press(EShopTillControl::RemoveCoin, 1));
    for (int32 Coin = 0; Coin < Buyer->TenderBits - Buyer->PriceBits; ++Coin)
    {
        TestTrue(TEXT("Count exact physical change"), Press(EShopTillControl::AddCoin, 1));
    }
    TestEqual(TEXT("Physical counting does not credit cash"), State->GetCash(), int64(0));
    TestTrue(TEXT("Physical confirmation completes the guarded sale"), Press(EShopTillControl::Confirm));
    TestEqual(TEXT("Sale credits the price once"), State->GetCash(), int64(12));
    TestFalse(TEXT("Physical confirmation cannot repeat payment"), Press(EShopTillControl::Confirm));
    TestTrue(TEXT("Repeated stamp preserves the successful receipt"), Till->GetFeedback().ToString().Contains(TEXT("Sale complete")));
    Till->AdvancePresentation(0.15f);
    TestTrue(TEXT("PAID stamp moves after success"), Till->FindControl(EShopTillControl::Confirm)->Visual->GetRelativeLocation().Z > -3.f);
    Till->AdvancePresentation(0.5f);
    TestEqual(TEXT("Stamp returns to rest"), Till->FindControl(EShopTillControl::Confirm)->Visual->GetRelativeLocation().Z, -3.0);

    Test.Camera->SetWorldLocation(FVector(0, -600, 155));
    AShopTillControl* RemoteKey = Till->FindControl(EShopTillControl::AddCoin, 1);
    Test.Camera->SetWorldRotation((RemoteKey->GetActorLocation() - Test.Camera->GetComponentLocation()).Rotation());
    TestNull(TEXT("Distant physical controls are outside the trace"), Test.Interaction->GetInteractionTarget());
    TestFalse(TEXT("Physical controls reject remote input"), IShopInteractable::Execute_Interact(RemoteKey, Test.Interaction));
    AShopTillControl* Control = Till->FindControl(EShopTillControl::Confirm);
    Till->Destroy();
    TestFalse(TEXT("Removing the counter removes its interaction controls"), IsValid(Control));
    return true;
}

namespace
{
    AShopCustomerFlow* DayFlow(FInteractionWorld& Test)
    {
        AShopCustomerFlow* Flow = Test.Spawn<AShopCustomerFlow>(FVector::ZeroVector);
        Flow->Till = Test.Spawn<AShopTill>(FVector(0, 84, 0));
        Flow->CustomerClass = AShopCustomer::StaticClass();
        Flow->bCheckoutFollowsTill = false;
    Flow->Entrance = FVector(-140, 140, 0);
        Flow->Checkout = FVector(0, 140, 0);
        Flow->VisitInterval = Flow->DepartureDelay = 0;
        return Flow;
    }

    struct FCheckpointFile
    {
        FString Path = FPaths::ProjectSavedDir() / TEXT("Automation") / (FGuid::NewGuid().ToString() + TEXT(".sav"));
        ~FCheckpointFile() { IFileManager::Get().Delete(*Path); IFileManager::Get().Delete(*(Path + TEXT(".tmp"))); }
        UShopMorningSave* Read()
        {
            TArray<uint8> Bytes;
            FFileHelper::LoadFileToArray(Bytes, *Path);
            return UShopMorningSave::Decode(Bytes);
        }
        void Write(UShopMorningSave* Save)
        {
            TArray<uint8> Bytes;
            UShopMorningSave::Encode(Save, Bytes);
            FFileHelper::SaveArrayToFile(Bytes, *Path);
        }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShopDayTransitionsTest, "FantasyShopkeeper.Day.TransitionsAndFiniteVisits",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShopDayTransitionsTest::RunTest(const FString& Parameters)
{
    FInteractionWorld Test;
    AShopGameState* State = Test.ShopState();
    AShopBrewingStation* Station = Test.BrewingStation();
    AShopCustomerFlow* Flow = DayFlow(Test);
    Flow->VisitsPerDay = 1;
    State->InitializeDay(FString());
    TestFalse(TEXT("Preparation blocks arrivals"), Flow->StartNextVisit());
    TestFalse(TEXT("Cannot skip preparation straight to tomorrow"), State->AdvanceMorning());
    TestTrue(TEXT("Explicit opening"), State->OpenShop());
    TestFalse(TEXT("Opening twice rejected"), State->OpenShop());
    TestTrue(TEXT("Finite allocation starts"), Flow->StartNextVisit());
    AShopCustomer* Buyer = Flow->GetActiveCustomer();
    if (!TestNotNull(TEXT("Active buyer"), Buyer)) return false;
    Buyer->SetActorLocation(Flow->Checkout);
    Buyer->Tick(0);
    TestTrue(TEXT("Close with an active customer"), State->CloseShop());
    TestFalse(TEXT("Repeated close rejected"), State->CloseShop());
    TestEqual(TEXT("Closing waits for buyer"), State->GetDayPhase(), EShopDayPhase::Closing);
    TestFalse(TEXT("Closing blocks new arrivals"), Flow->StartNextVisit());
    TestFalse(TEXT("Cannot sleep through an obligation"), State->AdvanceMorning());
    AShopShelfSlot* Shelf = Test.Spawn<AShopShelfSlot>(FVector(120, -80, 0));
    AShopPickupItem* Item = Test.BrewAndStock(Station, Shelf);
    TestTrue(TEXT("Missing stock can still be brewed during closing"), IsValid(Item));
    TestTrue(TEXT("Closing buyer collects"), Test.CollectAndReturn(Buyer, Item));
    AShopTill* Till = Flow->Till;
    Test.AimAt(Till);
    TestTrue(TEXT("Closing checkout starts"), Till->BeginCheckout(Test.Interaction));
    for (int32 Coin = 0; Coin < 8; ++Coin) Till->AddChangeCoin(1);
    TestTrue(TEXT("Closing sale commits"), Till->ConfirmSale(Test.Interaction));
    TestEqual(TEXT("Takings count net price only"), State->GetTakings(), int64(12));
    Flow->Tick(0);
    Buyer->SetActorLocation(Flow->Entrance);
    Buyer->Tick(0);
    Flow->Tick(0);
    Flow->Tick(0);
    TestEqual(TEXT("Departure completes closing"), State->GetDayPhase(), EShopDayPhase::Evening);
    TestNull(TEXT("Finite day does not wrap forever"), Flow->GetActiveCustomer());
    State->ConfirmNoSupplies();
    TestTrue(TEXT("Next morning commits"), State->AdvanceMorning());
    TestFalse(TEXT("Repeated next morning rejected"), State->AdvanceMorning());
    TestEqual(TEXT("Day advances once"), State->GetDay(), 2);
    TestEqual(TEXT("Closing cash preserved"), State->GetCash(), int64(12));
    TestEqual(TEXT("New daily totals reset"), State->GetTakings(), int64(0));
    TestTrue(TEXT("Can open another day"), State->OpenShop());
    TestTrue(TEXT("New day replenishes visitor allocation"), Flow->StartNextVisit());
    Flow->GetActiveCustomer()->Destroy();
    Flow->Tick(0);
    TestEqual(TEXT("Exhausted finite allocation ends automatically"), State->GetDayPhase(), EShopDayPhase::Evening);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShopMorningRestoreTest, "FantasyShopkeeper.Day.MorningRestoresWholeEconomy",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShopMorningRestoreTest::RunTest(const FString& Parameters)
{
    FCheckpointFile File;
    FInteractionWorld Test;
    AShopGameState* State = Test.ShopState(50);
    AShopBrewingStation* Station = Test.BrewingStation();
    AShopCustomerFlow* Flow = DayFlow(Test);
    AShopShelfSection* Section = Test.Spawn<AShopShelfSection>(FVector(120, -80, 0));
    TestNotNull(TEXT("Prepare shelf stock"), Test.BrewAndStock(Station, Section->GetSlot(0)));
    TestNotNull(TEXT("Prepare second shelf position"), Test.BrewAndStock(Station, Section->GetSlot(1)));
    Test.AimAt(Section);
    TestTrue(TEXT("Leave a gap in the shelf"), Test.Interaction->Interact());
    AShopPickupItem* Loose = Test.Interaction->GetHeldItem();
    Test.Interaction->ReleaseHeldItem();
    if (Loose) Loose->SetActorLocation(FVector(400, 400, 0));
    Test.AimAt(Station);
    TestTrue(TEXT("Prepare output stock"), Station->TryBrew(Test.Interaction));
    State->InitializeDay(File.Path);
    if (!TestNotNull(TEXT("Real disk checkpoint written"), File.Read())) return false;
    const int32 Leaves = Station->GetIngredientQuantity(TEXT("Moonleaf"));
    AShopSupplyCrate* Crate = Test.Spawn<AShopSupplyCrate>(FVector(-100, 0, 0));
    Crate->Station = Station;
    Test.AimAt(Crate);
    TestFalse(TEXT("Preparation no longer replenishes supplies"), Test.Interaction->Interact());
    State->OpenShop();
    Flow->StartNextVisit();
    AShopCustomer* Buyer = Flow->GetActiveCustomer();
    if (!TestNotNull(TEXT("Buyer spawned"), Buyer)) return false;
    Buyer->SetActorLocation(Flow->Checkout);
    Buyer->Tick(0);
    TestTrue(TEXT("Buyer collects saved stock"), Test.CollectAndReturn(Buyer, Section->GetSlot(1)->GetOccupyingItem()));
    AShopTill* Till = Flow->Till;
    Test.AimAt(Till);
    Till->BeginCheckout(Test.Interaction);
    for (int32 Coin = 0; Coin < 8; ++Coin) Till->AddChangeCoin(1);
    TestTrue(TEXT("Sale before restore"), Till->ConfirmSale(Test.Interaction));
    TestEqual(TEXT("Cash reflects sale before restore"), State->GetCash(), int64(62));
    for (int32 Retry = 0; Retry < 3; ++Retry)
    {
        TestTrue(TEXT("Restore can repeat without adding anything"), State->RestoreMorning());
        TestEqual(TEXT("Morning cash replaced"), State->GetCash(), int64(50));
        TestEqual(TEXT("Purchase delivery rolled back"), Station->GetIngredientQuantity(TEXT("Moonleaf")), Leaves);
        TestEqual(TEXT("Exactly one saved shelf bottle"), Section->GetStockCount(), 1);
        TestNotNull(TEXT("Output bottle restored"), Station->OutputSlot->GetOccupyingItem());
        int32 Count = 0;
        for (TActorIterator<AShopPickupItem> It(Test.World); It; ++It) ++Count;
        TestEqual(TEXT("No stock duplication"), Count, 3);
        TestNull(TEXT("Empty first position preserved"), Section->GetSlot(0)->GetOccupyingItem());
        TestNotNull(TEXT("Exact second position preserved"), Section->GetSlot(1)->GetOccupyingItem());
        TestEqual(TEXT("Section lock rebuilt"), Section->GetLockedProductId(), FName(TEXT("DawnTonic")));
        TestNull(TEXT("Old visit discarded"), Flow->GetActiveCustomer());
        TestEqual(TEXT("Pending receipt cleared"), Till->GetPhase(), EShopTillPhase::Idle);
        TestFalse(TEXT("Old sale cannot be replayed"), Till->ConfirmSale(Test.Interaction));
        TestEqual(TEXT("Daily spending reset"), State->GetSupplySpending(), int64(0));
    }
    Test.AimAt(Crate);
    State->OpenShop();
    State->CloseShop();
    TestTrue(TEXT("Evening order works after load"), Test.Interaction->Interact());
    TestEqual(TEXT("Pending supplies not delivered during evening"), Station->GetIngredientQuantity(TEXT("Moonleaf")), Leaves);
    TestTrue(TEXT("Morning saves paid supplies and unsold stock"), State->AdvanceMorning());
    TestTrue(TEXT("Reload next morning"), State->RestoreMorning());
    TestEqual(TEXT("Day two restored"), State->GetDay(), 2);
    TestEqual(TEXT("Paid cost not replayed"), State->GetCash(), int64(38));
    TestTrue(TEXT("Restored parcel unpacks once"), State->UnpackDelivery());
    TestFalse(TEXT("Repeated unpack is rejected"), State->UnpackDelivery());
    TestEqual(TEXT("Delivery not replayed"), Station->GetIngredientQuantity(TEXT("Moonleaf")), Leaves + 6);
    // A replacement GameState simulates startup against the same stable map actors.
    State->Destroy();
    State = Test.ShopState(999);
    State->InitializeDay(File.Path);
    TestEqual(TEXT("New session restores disk cash"), State->GetCash(), int64(38));
    TestEqual(TEXT("New session restores day"), State->GetDay(), 2);
    TestEqual(TEXT("New session preserves shelf stock"), Section->GetStockCount(), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShopInvalidMorningTest, "FantasyShopkeeper.Day.InvalidSavesArePreserved",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShopInvalidMorningTest::RunTest(const FString& Parameters)
{
    FCheckpointFile File;
    FInteractionWorld Test;
    AShopGameState* State = Test.ShopState(25);
    AShopBrewingStation* Station = Test.BrewingStation();
    AShopCustomerFlow* Flow = DayFlow(Test);
    AShopShelfSlot* Shelf = Test.Spawn<AShopShelfSlot>(FVector(120, -80, 0));
    Test.BrewAndStock(Station, Shelf);
    State->InitializeDay(File.Path);
    UShopMorningSave* Valid = File.Read();
    if (!TestNotNull(TEXT("Fixture checkpoint"), Valid)) return false;
    auto Reject = [&](UShopMorningSave* Bad)
    {
        File.Write(Bad);
        TArray<uint8> Before, After;
        FFileHelper::LoadFileToArray(Before, *File.Path);
        TestFalse(TEXT("Invalid save rejected"), State->RestoreMorning());
        TestEqual(TEXT("Rejected restore keeps money"), State->GetCash(), int64(25));
        TestNotNull(TEXT("Rejected restore keeps stock"), Shelf->GetOccupyingItem());
        State->OpenShop();
        State->CloseShop();
        State->ConfirmNoSupplies();
        TestFalse(TEXT("Invalid file cannot be overwritten by morning advance"), State->AdvanceMorning());
        FFileHelper::LoadFileToArray(After, *File.Path);
        TestTrue(TEXT("Invalid file preserved byte for byte"), Before == After);
        File.Write(Valid);
        TestTrue(TEXT("Corrected checkpoint can restore"), State->RestoreMorning());
    };
    UShopMorningSave* Bad = DuplicateObject<UShopMorningSave>(Valid, GetTransientPackage());
    Bad->Version = 999; Reject(Bad);
    Bad = DuplicateObject<UShopMorningSave>(Valid, GetTransientPackage());
    Bad->Cash = -1; Reject(Bad);
    Bad = DuplicateObject<UShopMorningSave>(Valid, GetTransientPackage());
    Bad->Ingredients[0].Quantities[TEXT("Moonleaf")] = 31; Reject(Bad);
    Bad = DuplicateObject<UShopMorningSave>(Valid, GetTransientPackage());
    const FShopSavedStock DuplicateStock = Bad->Stock[0];
    Bad->Stock.Add(DuplicateStock); Reject(Bad);
    Bad = DuplicateObject<UShopMorningSave>(Valid, GetTransientPackage());
    Bad->Stock[0].Slot = TEXT("MissingShelf"); Reject(Bad);
    Bad = DuplicateObject<UShopMorningSave>(Valid, GetTransientPackage());
    Bad->Stock[0].ItemClass = TEXT("/Script/Engine.Actor"); Reject(Bad);
    Bad = DuplicateObject<UShopMorningSave>(Valid, GetTransientPackage());
    Bad->Stock[0].Product = TEXT("UnknownProduct"); Reject(Bad);
    AShopShelfSlot* ExtraShelf = Test.Spawn<AShopShelfSlot>(FVector(120, -130, 0));
    AShopPickupItem* CounterGoods = Test.BrewAndStock(Station, ExtraShelf);
    AShopCustomer* Buyer = Test.Spawn<AShopCustomer>(FVector(0, 140, 0));
    TestTrue(TEXT("Prepare live counter goods for invalid destination check"), Test.CollectAndReturn(Buyer, CounterGoods));
    AShopTill* Counter = Flow->Till;
    Counter->AssignCustomer(Buyer);
    if (!TestTrue(TEXT("Pending counter"), Counter->PresentCustomerGoods())) return false;
    Bad = DuplicateObject<UShopMorningSave>(Valid, GetTransientPackage());
    Bad->Stock[0].Slot = CounterGoods->GetCurrentSlot()->GetName();
    Reject(Bad); // Transient checkout positions are never valid morning destinations.
    TArray<uint8> Bytes;
    UShopMorningSave::Encode(Valid, Bytes);
    Bytes.Last() ^= 1;
    FFileHelper::SaveArrayToFile(Bytes, *File.Path);
    TestFalse(TEXT("Corrupt payload rejected before Unreal deserialization"), State->RestoreMorning());
    Bytes.SetNum(12);
    FFileHelper::SaveArrayToFile(Bytes, *File.Path);
    TestFalse(TEXT("Truncated file rejected"), State->RestoreMorning());
    State->Destroy();
    State = Test.ShopState(25);
    State->InitializeDay(File.Path);
    TestTrue(TEXT("Invalid startup reports the issue"), State->GetCheckpointMessage().Contains(TEXT("Invalid")));
    TestEqual(TEXT("Invalid startup retains default cash"), State->GetCash(), int64(25));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShopMorningFailureTest, "FantasyShopkeeper.Day.HeldStockAndWriteFailure",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShopMorningFailureTest::RunTest(const FString& Parameters)
{
    FCheckpointFile File;
    FInteractionWorld Test;
    AShopGameState* State = Test.ShopState();
    AShopBrewingStation* Station = Test.BrewingStation();
    DayFlow(Test);
    State->InitializeDay(File.Path);
    State->OpenShop();
    State->CloseShop();
    Test.AimAt(Station);
    bool NestedRestore = true;
    const FDelegateHandle Handle = Test.World->AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateLambda([&](AActor* Actor)
    {
        if (Actor->IsA<AShopPickupItem>()) NestedRestore = State->RestoreMorning();
    }));
    TestTrue(TEXT("Brew commits while a callback attempts restoration"), Station->TryBrew(Test.Interaction));
    Test.World->RemoveOnActorSpawnedHandler(Handle);
    TestFalse(TEXT("Cannot restore halfway through consuming ingredients"), NestedRestore);
    Test.AimAt(Station->OutputSlot);
    TestTrue(TEXT("Hold a finished brew"), Test.Interaction->Interact());
    State->ConfirmNoSupplies();
    TestFalse(TEXT("Held stock blocks sleep"), State->AdvanceMorning());
    TestEqual(TEXT("Blocked sleep keeps day"), State->GetDay(), 1);
    TestTrue(TEXT("Put brew back"), Test.Interaction->Interact());
    // A real file lock tests failure without logging an expected filesystem warning.
    TUniquePtr<FArchive> Lock(IFileManager::Get().CreateFileReader(*File.Path));
    TestFalse(TEXT("Locked destination blocks advancement"), State->AdvanceMorning());
    TestEqual(TEXT("Write failure retains evening"), State->GetDayPhase(), EShopDayPhase::Evening);
    Lock.Reset();
    TestTrue(TEXT("Retry after failure succeeds"), State->AdvanceMorning());
    TestEqual(TEXT("Retry advances exactly once"), State->GetDay(), 2);
    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShopDeferredSuppliesTest, "FantasyShopkeeper.Supplies.MorningDeliveryRetriesAndRestore",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShopDeferredSuppliesTest::RunTest(const FString& Parameters)
{
    FCheckpointFile File;
    FInteractionWorld Test;
    AShopGameState* State = Test.ShopState(50);
    AShopBrewingStation* Station = Test.BrewingStation(0, 0);
    AShopCustomerFlow* Flow = DayFlow(Test);
    AShopSupplyCrate* Crate = Test.Spawn<AShopSupplyCrate>(FVector(-100, 0, 0));
    Crate->Station = Station;
    State->InitializeDay(File.Path);
    Test.AimAt(Crate);
    TestFalse(TEXT("No preparation ordering"), Test.Interaction->Interact());
    State->OpenShop();
    TestFalse(TEXT("No trading ordering"), Test.Interaction->Interact());
    State->CloseShop();
    TestFalse(TEXT("Morning needs an explicit order or zero order"), State->AdvanceMorning());
    TestTrue(TEXT("Confirm paid evening order"), Test.Interaction->Interact());
    TestEqual(TEXT("Debit once"), State->GetCash(), int64(38));
    TestFalse(TEXT("Repeated payment rejected"), Test.Interaction->Interact());
    TestEqual(TEXT("Evening ingredients unchanged"), Station->GetIngredientQuantity(TEXT("Moonleaf")), 0);
    TestTrue(TEXT("Restart rolls back the entire pending order"), State->RestoreMorning());
    TestEqual(TEXT("Paid debit rolled back with undelivered order"), State->GetCash(), int64(50));
    TestFalse(TEXT("Order status resets with morning"), State->HasConfirmedSupplies());
    State->OpenShop(); State->CloseShop();
    TestTrue(TEXT("Can order after restoration"), Test.Interaction->Interact());
    // Pending order owns quantities, not a live pointer to the supplier offer.
    Crate->Delivery[TEXT("Moonleaf")] = 29;
    Station->IngredientCapacity = 5;
    TestFalse(TEXT("Changed storage capacity blocks morning coherently"), State->AdvanceMorning());
    Station->IngredientCapacity = 30;
    TUniquePtr<FArchive> Lock(IFileManager::Get().CreateFileReader(*File.Path));
    TestFalse(TEXT("Locked save cannot deliver paid stock"), State->AdvanceMorning());
    TestTrue(TEXT("Paid order retained for retry"), State->HasConfirmedSupplies());
    TestEqual(TEXT("Failed write creates no ingredients"), Station->GetIngredientQuantity(TEXT("Moonleaf")), 0);
    TestEqual(TEXT("Failed write charges no second payment"), State->GetCash(), int64(38));
    Lock.Reset();
    TestTrue(TEXT("Retry delivers and saves"), State->AdvanceMorning());
    TestTrue(TEXT("Unpack the sealed morning parcel"), State->UnpackDelivery());
    TestEqual(TEXT("Original ordered quantity used"), Station->GetIngredientQuantity(TEXT("Moonleaf")), 6);
    TestEqual(TEXT("Original water quantity delivered"), Station->GetIngredientQuantity(TEXT("Spring Water")), 3);
    for (int32 Retry = 0; Retry < 2; ++Retry)
    {
        TestTrue(TEXT("Delivered morning restores"), State->RestoreMorning());
        TestEqual(TEXT("Restoration returns ingredients to unopened state"), Station->GetIngredientQuantity(TEXT("Moonleaf")), 0);
        TestTrue(TEXT("Restored paid parcel unpacks"), State->UnpackDelivery());
        TestFalse(TEXT("Repeated unpack cannot duplicate ingredients"), State->UnpackDelivery());
        TestEqual(TEXT("Delivery is never replayed"), Station->GetIngredientQuantity(TEXT("Moonleaf")), 6);
        TestEqual(TEXT("Payment is never replayed"), State->GetCash(), int64(38));
    }
    State->OpenShop(); State->CloseShop();
    TestTrue(TEXT("Choose zero packs"), Crate->ChangeQuantity(Test.Interaction, -1));
    TestTrue(TEXT("Confirm zero order through ordinary interaction"), Test.Interaction->Interact());
    TestEqual(TEXT("Zero order costs nothing"), State->GetCash(), int64(38));
    TestTrue(TEXT("Zero order permits next morning"), State->AdvanceMorning());
    TestEqual(TEXT("Zero order adds no ingredients"), Station->GetIngredientQuantity(TEXT("Moonleaf")), 6);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShopLightingTest, "FantasyShopkeeper.Day.PhaseLightingTransitions",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShopLightingTest::RunTest(const FString& Parameters)
{
    FInteractionWorld Test;
    AShopGameState* State = Test.ShopState();
    DayFlow(Test);
    State->InitializeDay(FString());
    ADirectionalLight* Sun = Test.Spawn<ADirectionalLight>(FVector::ZeroVector);
    Sun->GetLightComponent()->SetMobility(EComponentMobility::Movable);
    Sun->GetLightComponent()->SetIntensity(10000);
    AShopDayLighting* Lighting = Test.Spawn<AShopDayLighting>(FVector::ZeroVector);
    Lighting->Tick(1);
    TestTrue(TEXT("Morning has daylight"), Sun->GetLightComponent()->Intensity > 7000);
    State->OpenShop();
    Lighting->Tick(5);
    TestEqual(TEXT("Trading reaches full daylight"), Sun->GetLightComponent()->Intensity, 10000.f);
    State->CloseShop();
    Lighting->Tick(1);
    TestTrue(TEXT("Closing interpolates rather than snapping"), Lighting->GetNightBlend() > 0 && Lighting->GetNightBlend() < 1);
    Lighting->Tick(4);
    TestEqual(TEXT("Evening reaches night"), Lighting->GetNightBlend(), 1.f);
    TestTrue(TEXT("Sun is dim at night"), Sun->GetLightComponent()->Intensity < 1);
    Lighting->Tick(100);
    TestEqual(TEXT("Waiting never advances the day"), State->GetDay(), 1);
    State->ConfirmNoSupplies(); State->AdvanceMorning();
    Lighting->Tick(1);
    TestEqual(TEXT("Explicit morning restores day lighting"), Lighting->GetNightBlend(), 0.f);
    TestTrue(TEXT("Morning sun returns"), Sun->GetLightComponent()->Intensity > 7000);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShopUnservedVisitTest, "FantasyShopkeeper.Customer.EndUnservedVisitWithoutMoneyOrStock",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShopUnservedVisitTest::RunTest(const FString& Parameters)
{
    FInteractionWorld Test;
    AShopGameState* State = Test.ShopState();
    AShopBrewingStation* Station = Test.BrewingStation();
    AShopCustomerFlow* Flow = DayFlow(Test);
    Flow->VisitsPerDay = 1;
    State->InitializeDay(FString());
    State->OpenShop(); Flow->StartNextVisit();
    AShopCustomer* Buyer = Flow->GetActiveCustomer();
    if (!TestNotNull(TEXT("Unserved visitor"), Buyer)) return false;
    TestTrue(TEXT("Player can resolve a stock shortage"), Flow->EndUnservedVisit());
    TestFalse(TEXT("Repeat dismissal rejected"), Flow->EndUnservedVisit());
    TestFalse(TEXT("Unserved customer has not paid"), Buyer->HasCompletedSale());
    Buyer->SetActorLocation(Flow->Entrance); Buyer->Tick(0); Flow->Tick(0); Flow->Tick(0);
    TestEqual(TEXT("Resolved opportunity reaches evening"), State->GetDayPhase(), EShopDayPhase::Evening);
    TestEqual(TEXT("No invented payment"), State->GetCash(), int64(0));
    State->ConfirmNoSupplies(); State->AdvanceMorning(); State->OpenShop(); Flow->StartNextVisit();
    Buyer = Flow->GetActiveCustomer();
    if (!TestNotNull(TEXT("Following morning visitor"), Buyer)) return false;
    Buyer->SetActorLocation(Flow->Checkout); Buyer->Tick(0);
    AShopShelfSlot* Shelf = Test.Spawn<AShopShelfSlot>(FVector(120, -80, 0));
    AShopPickupItem* Item = Test.BrewAndStock(Station, Shelf);
    TestTrue(TEXT("Visitor collects physical stock"), Test.CollectAndReturn(Buyer, Item));
    TestFalse(TEXT("Cannot dismiss a customer carrying unpaid stock"), Flow->EndUnservedVisit());
    TestEqual(TEXT("Checkout obligation retains ownership"), Buyer->ShopInteraction->GetHeldItem(), Item);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShopDoorSignTest, "FantasyShopkeeper.Day.DoorSignInteraction",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShopDoorSignTest::RunTest(const FString& Parameters)
{
    FInteractionWorld Test;
    AShopGameState* State = Test.ShopState();
    DayFlow(Test);
    State->InitializeDay(FString());
    AShopDoorSign* Sign = Test.Spawn<AShopDoorSign>(FVector(-100, 0, 0));
    Test.AimAt(Sign);
    TestTrue(TEXT("E on sign opens shop"), Test.Interaction->Interact());
    TestEqual(TEXT("Sign starts trading"), State->GetDayPhase(), EShopDayPhase::Trading);
    Sign->SetActorLocation(FVector(-500, 0, 0));
    TestFalse(TEXT("Cannot close remotely"), Sign->Interact_Implementation(Test.Interaction));
    Sign->SetActorLocation(FVector(-100, 0, 0));
    TestTrue(TEXT("E on sign closes shop"), Test.Interaction->Interact());
    TestEqual(TEXT("Empty shop reaches evening"), State->GetDayPhase(), EShopDayPhase::Evening);
    TestFalse(TEXT("Repeated sign interaction cannot reopen evening"), Test.Interaction->Interact());
    State->ConfirmNoSupplies(); State->AdvanceMorning();
    AShopPickupItem* Item = Test.Spawn<AShopPickupItem>(FVector(100, -70, 0));
    Test.AimAt(Item); Test.Interaction->Interact(); Test.AimAt(Sign);
    TestFalse(TEXT("Full hands cannot turn the sign"), Test.Interaction->Interact());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShopMultipleOrdersTest, "FantasyShopkeeper.Supplies.MultipleOrdersThroughDayFour",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShopMultipleOrdersTest::RunTest(const FString& Parameters)
{
    FCheckpointFile File;
    FInteractionWorld Test;
    AShopGameState* State = Test.ShopState(50);
    AShopBrewingStation* Station = Test.BrewingStation(0, 0);
    AShopCustomerFlow* Flow = DayFlow(Test);
    AShopShelfSlot* Shelf = Test.Spawn<AShopShelfSlot>(FVector(120, -80, 0));
    AShopSupplyCrate* Crate = Test.Spawn<AShopSupplyCrate>(FVector(-100, 0, 0));
    Crate->Station = Station;
    State->InitializeDay(File.Path);
    State->OpenShop(); State->CloseShop();
    for (int32 Morning = 2; Morning <= 4; ++Morning)
    {
        Test.AimAt(Crate);
        // Select the mixed bundle, then a separate water order in the same evening.
        for (int32 Choice = 0; Crate->Delivery.Num() != 2 && Choice < 3; ++Choice) Crate->CycleOffer(Test.Interaction);
        const int64 CashBefore = State->GetCash();
        const int32 Leaves = Station->GetIngredientQuantity(TEXT("Moonleaf"));
        const int32 Water = Station->GetIngredientQuantity(TEXT("Spring Water"));
        TestTrue(TEXT("Pay for bundle"), Test.Interaction->Interact());
        TestFalse(TEXT("Repeated E cannot duplicate this order"), Test.Interaction->Interact());
        TestTrue(TEXT("Can select another offer after paying"), Crate->CycleOffer(Test.Interaction));
        TestTrue(TEXT("Select water"), Crate->CycleOffer(Test.Interaction));
        TestTrue(TEXT("Pay for a second order in this evening"), Test.Interaction->Interact());
        TestFalse(TEXT("Second order also rejects repeated E"), Test.Interaction->Interact());
        TestEqual(TEXT("Both orders charged exactly once"), State->GetCash(), CashBefore - 18);
        TestEqual(TEXT("Orders do not arrive tonight"), Station->GetIngredientQuantity(TEXT("Spring Water")), Water);
        Station->IngredientCapacity = Water + 5;
        TestFalse(TEXT("Combined pending quantities must fit"), State->AdvanceMorning());
        Station->IngredientCapacity = 30;
        TestTrue(TEXT("Deliver both orders"), State->AdvanceMorning());
        TestEqual(TEXT("Advance through day four"), State->GetDay(), Morning);
        TestTrue(TEXT("Restore delivered morning"), State->RestoreMorning());
        TestTrue(TEXT("Repeated restore cannot redeliver"), State->RestoreMorning());
        TestTrue(TEXT("Unpack both orders together"), State->UnpackDelivery());
        TestEqual(TEXT("Both water orders arrive once"), Station->GetIngredientQuantity(TEXT("Spring Water")), Water + 6);
        TestEqual(TEXT("Bundle leaves arrive once"), Station->GetIngredientQuantity(TEXT("Moonleaf")), Leaves + 6);
        TestEqual(TEXT("Reload does not recharge orders"), State->GetCash(), CashBefore - 18);
        State->OpenShop();
        for (int32 Visit = 0; Visit < 3; ++Visit)
        {
            const bool bInk = Visit == 1;
            Station->Recipe->ProductId = bInk ? TEXT("MoonlitInk") : TEXT("DawnTonic");
            Station->Recipe->IngredientCosts = {{TEXT("Moonleaf"), bInk ? 1 : 2}, {TEXT("Spring Water"), bInk ? 2 : 1}};
            AShopPickupItem* Item = Test.BrewAndStock(Station, Shelf);
            if (!TestNotNull(TEXT("Enough supplies for every visit"), Item)) return false;
            AShopCustomer* Buyer = Test.Spawn<AShopCustomer>(FVector(0, 140, 0));
            Buyer->RequestedProductId = Station->Recipe->ProductId;
            Buyer->PriceBits = bInk ? 18 : 12;
            Buyer->TenderBits = bInk ? 50 : 20;
            TestTrue(TEXT("Customer collects brewed goods"), Test.CollectAndReturn(Buyer, Item));
            AShopTill* Till = Test.PreparedTill(Buyer, Flow->Till);
            if (!TestNotNull(TEXT("Goods and payment reach checkout"), Till))
            {
                AddError(FString::Printf(TEXT("Morning %d visit %d: buyer %s activity %d target %s held %s item slot %s"), Morning, Visit,
                    *Buyer->GetActorLocation().ToString(), int32(Buyer->Activity), *GetNameSafe(Test.Interaction->GetInteractionTarget()),
                    *GetNameSafe(Buyer->ShopInteraction->GetHeldItem()), *GetNameSafe(Item->GetCurrentSlot())));
                return false;
            }
            TestTrue(TEXT("Sell without a typed total"), Till->ConfirmSale(Test.Interaction));
            Till->AssignCustomer(nullptr); Buyer->Destroy();
        }
        TestEqual(TEXT("All three visits fund the following evening"), State->GetCash(), CashBefore - 18 + 42);
        State->CloseShop();
    }
    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShopPhysicalDeliveryTest, "FantasyShopkeeper.Supplies.PhysicalParcelAndInvalidSave",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShopPhysicalDeliveryTest::RunTest(const FString& Parameters)
{
    FCheckpointFile File;
    FInteractionWorld Test;
    AShopGameState* State = Test.ShopState(50);
    AShopBrewingStation* Station = Test.BrewingStation(0, 0);
    DayFlow(Test);
    AShopSupplyCrate* Desk = Test.Spawn<AShopSupplyCrate>(FVector(-100, 0, 0));
    Desk->Station = Station;
    State->InitializeDay(File.Path);
    State->OpenShop(); State->CloseShop();
    Test.AimAt(Desk);
    TestTrue(TEXT("Evening order succeeds"), Test.Interaction->Interact());
    TestTrue(TEXT("Physical book includes tomorrow's visitors"), Desk->GetPlanningText().Contains(TEXT("3 visitors")));
    TestTrue(TEXT("Morning checkpoints the unopened parcel"), State->AdvanceMorning());
    TestEqual(TEXT("Delivery is not usable before unpacking"), Station->GetIngredientQuantity(TEXT("Moonleaf")), 0);
    TestFalse(TEXT("Opening directs player to unpack"), State->OpenShop());
    AShopSupplyControl* Parcel = Test.Spawn<AShopSupplyControl>(FVector(0, -100, 0));
    Parcel->Configure(Desk, EShopSupplyControl::Delivery);
    Test.AimAt(Parcel);
    TestTrue(TEXT("Reachable E unpacks the parcel"), Test.Interaction->Interact());
    TestEqual(TEXT("Unpack transfers all ingredients once"), Station->GetIngredientQuantity(TEXT("Moonleaf")), 6);
    TestFalse(TEXT("Second E has no delivery to duplicate"), Parcel->Interact_Implementation(Test.Interaction));
    TestTrue(TEXT("Restore rolls back unpacking"), State->RestoreMorning());
    Parcel->Tick(0);
    TestEqual(TEXT("Restored storage excludes parcel"), Station->GetIngredientQuantity(TEXT("Moonleaf")), 0);
    TestEqual(TEXT("Restored parcel contains original paid quantity"), State->GetDeliveredQuantity(TEXT("Moonleaf")), 6);
    Parcel->SetActorLocation(FVector(0, -500, 0));
    TestFalse(TEXT("Cannot unpack remotely"), Parcel->Interact_Implementation(Test.Interaction));
    TArray<uint8> Bytes;
    FFileHelper::LoadFileToArray(Bytes, *File.Path);
    UShopMorningSave* Save = UShopMorningSave::Decode(Bytes);
    if (!TestNotNull(TEXT("Saved unopened parcel"), Save)) return false;
    Save->Delivery.Add(TEXT("Moonleaf"), MAX_int32);
    UShopMorningSave::Encode(Save, Bytes);
    FFileHelper::SaveArrayToFile(Bytes, *File.Path);
    TestFalse(TEXT("Overflowing parcel save rejected before changes"), State->RestoreMorning());
    TestEqual(TEXT("Rejected save preserves paid parcel"), State->GetDeliveredQuantity(TEXT("Moonleaf")), 6);
    TestEqual(TEXT("Rejected save preserves cash"), State->GetCash(), int64(38));
    // Old checkpoints restore their already-delivered ingredients without inventing a parcel.
    Save->Version = 1;
    Save->Delivery.Empty();
    Save->DeliveryStation.Empty();
    UShopMorningSave::Encode(Save, Bytes);
    FFileHelper::SaveArrayToFile(Bytes, *File.Path);
    TestTrue(TEXT("Version one checkpoint remains readable"), State->RestoreMorning());
    TestFalse(TEXT("Old checkpoint has no invented delivery"), State->HasDelivery());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShopOvernightTest, "FantasyShopkeeper.Day.PhysicalRestAndOvernightGuard",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShopOvernightTest::RunTest(const FString& Parameters)
{
    FInteractionWorld Test;
    AShopGameState* State = Test.ShopState();
    DayFlow(Test);
    State->InitializeDay(FString());
    AShopSupplyCrate* Desk = Test.Spawn<AShopSupplyCrate>(FVector(100, 100, 0));
    AShopSupplyControl* Rest = Test.Spawn<AShopSupplyControl>(FVector(-100, 0, 0));
    Rest->Configure(Desk, EShopSupplyControl::Rest);
    Test.Camera->SetWorldRotation((Rest->GetActorLocation() - Test.Camera->GetComponentLocation()).Rotation());
    TestFalse(TEXT("Cannot sleep in preparation"), Test.Interaction->Interact());
    State->OpenShop(); State->CloseShop();
    TestFalse(TEXT("Rest needs explicit evening planning"), Test.Interaction->Interact());
    Rest->Configure(Desk, EShopSupplyControl::Pack);
    TestTrue(TEXT("E on pack tab changes offer"), Test.Interaction->Interact());
    Rest->Configure(Desk, EShopSupplyControl::More);
    TestTrue(TEXT("E on more tab increments quantity"), Test.Interaction->Interact());
    TestEqual(TEXT("Quantity is now two"), Desk->GetPackCount(), 2);
    Rest->Configure(Desk, EShopSupplyControl::Less);
    TestTrue(TEXT("E on less tab decrements quantity"), Test.Interaction->Interact());
    Rest->Configure(Desk, EShopSupplyControl::Rest);
    State->ConfirmNoSupplies();
    TestTrue(TEXT("E on rest tab starts the night"), Test.Interaction->Interact());
    TestTrue(TEXT("Transition has an explicit guard"), State->IsOvernight());
    TestEqual(TEXT("Night does not immediately commit morning"), State->GetDay(), 1);
    TestFalse(TEXT("No repeated sleep"), State->BeginOvernight());
    TestFalse(TEXT("No orders during fade"), State->ConfirmNoSupplies());
    TestFalse(TEXT("No early morning commit during fade"), State->AdvanceMorning());
    {
        TGuardValue<uint64> FrameGuard(GFrameCounter, GFrameCounter);
        Test.World->GetTimerManager().Tick(0.f);
        ++GFrameCounter;
        Test.World->GetTimerManager().Tick(4.f);
    }
    TestFalse(TEXT("Night completes"), State->IsOvernight());
    TestEqual(TEXT("One morning committed"), State->GetDay(), 2);
    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShopCheckoutPresentationCancelTest, "FantasyShopkeeper.Till.PresentationCancellationPreservesGoods",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShopCheckoutPresentationCancelTest::RunTest(const FString& Parameters)
{
    FInteractionWorld Test;
    AShopGameState* State = Test.ShopState();
    AShopBrewingStation* Station = Test.BrewingStation();
    AShopShelfSlot* Shelf = Test.Spawn<AShopShelfSlot>(FVector(120, 60, 0));
    AShopPickupItem* Item = Test.BrewAndStock(Station, Shelf);
    AShopCustomer* Buyer = Test.Spawn<AShopCustomer>(FVector(0, 140, 0));
    if (!TestTrue(TEXT("Customer collects goods"), Test.CollectAndReturn(Buyer, Item))) return false;
    AShopTill* Till = Test.Spawn<AShopTill>(FVector(0, 84, 0));
    Till->bPhysicalCounter = true;
    Till->AssignCustomer(Buyer);
    TestTrue(TEXT("Goods presentation starts"), Till->PresentCustomerGoods());
    Till->AdvancePresentation(0.1f);
    Till->AssignCustomer(nullptr);
    TestTrue(TEXT("Cancelling restores the item's visibility"), Item->ItemMesh->IsVisible());
    TestNull(TEXT("Cancelled item becomes loose stock"), Item->GetCurrentSlot());
    TestFalse(TEXT("Cancelled cosmetic copy disappears"), Till->IsPresentingGoods());
    Till->AdvancePresentation(5.f);
    TestEqual(TEXT("Cancelled payment never clinks"), Till->GetPaymentClinkCount(), 0);
    TestEqual(TEXT("Animation never credits money"), State->GetCash(), int64(0));
    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShopCustomerDialogueTest, "FantasyShopkeeper.Customer.RequestAndPurchaseReactionOnce",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShopCustomerDialogueTest::RunTest(const FString& Parameters)
{
    FInteractionWorld Test;
    AShopGameState* State = Test.ShopState();
    AShopBrewingStation* Station = Test.BrewingStation();
    AShopShelfSlot* Shelf = Test.Spawn<AShopShelfSlot>(FVector(120, 60, 0));
    AShopPickupItem* Item = Test.BrewAndStock(Station, Shelf);
    AShopCustomer* Buyer = Test.Spawn<AShopCustomer>(FVector(0, 140, 0));
    Buyer->ConfigureVisitPersonality(0);
    TestTrue(TEXT("Begin the visit"), Buyer->BeginVisit(Buyer->GetActorLocation()));
    TestTrue(TEXT("Request waits for a short natural pause"), Buyer->GetSpokenLine().IsEmpty());
    Buyer->AdvanceConversation(0.55f);
    TestTrue(TEXT("Arrival names the requested product"), Buyer->GetSpokenLine().ToString().Contains(TEXT("Dawn Tonic")));
    TestTrue(TEXT("Subtitle fades in"), Buyer->GetSpeechOpacity() > 0);
    Buyer->AdvanceConversation(10.f);
    TestTrue(TEXT("Arrival expires"), Buyer->GetSpokenLine().IsEmpty());
    Buyer->BeginVisit(Buyer->GetActorLocation()); Buyer->AdvanceConversation(1.f);
    TestTrue(TEXT("Route retries do not repeat arrival dialogue"), Buyer->GetSpokenLine().IsEmpty());
    if (!TestTrue(TEXT("Collect goods"), Test.CollectAndReturn(Buyer, Item))) return false;
    AShopTill* Till = Test.PreparedTill(Buyer);
    if (!TestNotNull(TEXT("Checkout ready"), Till)) return false;
    Till->RemoveChangeCoin(1);
    TestFalse(TEXT("Wrong change rejects sale"), Till->ConfirmSale(Test.Interaction));
    Buyer->AdvanceConversation(1.f);
    TestTrue(TEXT("Rejected sale does not thank the player for a purchase"), Buyer->GetSpokenLine().IsEmpty());
    Till->AddChangeCoin(1);
    TestTrue(TEXT("Exact manual change completes sale"), Till->ConfirmSale(Test.Interaction));
    TestTrue(TEXT("Reaction has a short pause"), Buyer->GetSpokenLine().IsEmpty());
    Buyer->AdvanceConversation(0.3f);
    TestTrue(TEXT("Committed sale earns a reaction"), Buyer->GetSpokenLine().ToString().Contains(TEXT("Much obliged")));
    TestEqual(TEXT("Dialogue cannot change takings"), State->GetCash(), int64(12));
    Buyer->AdvanceConversation(10.f);
    TestFalse(TEXT("Repeat confirmation cannot pay twice"), Till->ConfirmSale(Test.Interaction));
    Buyer->AdvanceConversation(1.f);
    TestTrue(TEXT("Repeat confirmation cannot repeat thanks"), Buyer->GetSpokenLine().IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShopCustomerDialogueVariationsTest, "FantasyShopkeeper.Customer.DialogueVariationsAndUnservedExit",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShopCustomerDialogueVariationsTest::RunTest(const FString& Parameters)
{
    FInteractionWorld Test;
    Test.ShopState();
    TSet<FString> Lines;
    for (int32 Variation = 0; Variation < 3; ++Variation)
    {
        AShopCustomer* Buyer = Test.Spawn<AShopCustomer>(FVector(400 + Variation * 100, 0, 0));
        Buyer->RequestedProductName = FText::FromString(TEXT("Moonlit Ink"));
        Buyer->ConfigureVisitPersonality(Variation);
        Buyer->BeginVisit(Buyer->GetActorLocation());
        Buyer->AdvanceConversation(0.5f);
        TestEqual(TEXT("Greeting delays vary predictably"), Buyer->GetSpokenLine().IsEmpty(), Variation != 0);
        Buyer->AdvanceConversation(0.6f);
        Lines.Add(Buyer->GetSpokenLine().ToString());
        TestTrue(TEXT("Every variation names the actual product"), Buyer->GetSpokenLine().ToString().Contains(TEXT("Moonlit Ink")));
        TestTrue(TEXT("Departure pace remains bounded"), Buyer->GetDeparturePace() >= 0.85f && Buyer->GetDeparturePace() <= 1.16f);
        TestTrue(TEXT("Unserved visit can still end"), Buyer->LeaveWithoutPurchase(FVector(1000, 0, 0)));
        Buyer->AdvanceConversation(10.f);
        TestTrue(TEXT("Unserved exit clears purchase/request speech"), Buyer->GetSpokenLine().IsEmpty());
        TestFalse(TEXT("Unserved exit does not imply a purchase"), Buyer->GetInteractionPrompt().ToString().Contains(TEXT("Thank")));
        Buyer->Destroy();
    }
    TestEqual(TEXT("Three distinct requests"), Lines.Num(), 3);
    AShopCustomer* First = Test.Spawn<AShopCustomer>(FVector(400, 0, 0));
    AShopCustomer* Reloaded = Test.Spawn<AShopCustomer>(FVector(500, 0, 0));
    First->ConfigureVisitPersonality(4); Reloaded->ConfigureVisitPersonality(4);
    First->BeginVisit(First->GetActorLocation()); Reloaded->BeginVisit(Reloaded->GetActorLocation());
    First->AdvanceConversation(1.f); Reloaded->AdvanceConversation(1.f);
    TestEqual(TEXT("Same visit selection restores the same wording"), First->GetSpokenLine().ToString(), Reloaded->GetSpokenLine().ToString());
    return true;
}

#endif
