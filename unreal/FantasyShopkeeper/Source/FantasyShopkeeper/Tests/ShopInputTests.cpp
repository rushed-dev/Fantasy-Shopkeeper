#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Editor.h"
#include "Framework/Application/SlateApplication.h"
#include "Kismet2/DebuggerCommands.h"
#include "Camera/CameraComponent.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "UnrealClient.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "EngineUtils.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PawnMovementComponent.h"
#include "InputKeyEventArgs.h"
#include "EnhancedPlayerInput.h"
#include "EnhancedActionKeyMapping.h"
#include "InputAction.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "UObject/UnrealType.h"
#include "../ShopInteractionComponent.h"
#include "../ShopPickupItem.h"
#include "../ShopPlayerController.h"
#include "../ShopShelfSlot.h"
#include "../ShopBrewingStation.h"
#include "../ShopCustomer.h"
#include "../ShopGameState.h"
#include "../ShopTill.h"
#include "../ShopDoorSign.h"
#include "../ShopSupplyControl.h"
#include "../ShopStorageUpgrade.h"
#include "../ShopBrewingUpgrade.h"
#include "../ShopTillControl.h"
#include "../ShopShelfSection.h"
#include "../ShopCustomerFlow.h"
#include "../ShopSupplyCrate.h"
#include "../ShopBrewingRecipe.h"

namespace
{
    class FShopInputWorkflow : public IAutomationLatentCommand
    {
    public:
        explicit FShopInputWorkflow(FAutomationTestBase* InTest) : Test(InTest) {}

        virtual ~FShopInputWorkflow() override
        {
            if (Viewport.IsValid())
            {
                Viewport->SetIgnoreInput(bOriginalIgnoreInput);
            }
        }

        virtual bool Update() override
        {
            const double Now = FPlatformTime::Seconds();
            if (Now - Started > 240.0)
            {
                Test->AddError(FString::Printf(TEXT("Shop E-key workflow timed out at step %d. Customer activity=%d position=%s held=%s"),
                    Step, Customer.IsValid() ? static_cast<int32>(Customer->Activity) : -1,
                    Customer.IsValid() ? *Customer->GetActorLocation().ToString() : TEXT("missing"),
                    Customer.IsValid() ? *GetNameSafe(Customer->ShopInteraction->GetHeldItem()) : TEXT("missing")));
                return true;
            }
            UWorld* World = GEditor->PlayWorld;
            if (!World)
            {
                return false;
            }
            AShopPlayerController* Controller = Cast<AShopPlayerController>(World->GetFirstPlayerController());
            APawn* Pawn = Controller ? Controller->GetPawn().Get() : nullptr;
            if (!Controller || !Pawn)
            {
                return false;
            }
            if (Viewport.IsValid())
            {
                Viewport->SetIgnoreInput(true);
            }
            UCameraComponent* Camera = Pawn->FindComponentByClass<UCameraComponent>();
            if (!Camera)
            {
                Test->AddError(TEXT("Shop player has no camera"));
                return true;
            }
            if (Flow.IsValid()) MaxCustomersObserved = FMath::Max(MaxCustomersObserved, Flow->GetCustomersInShop());
            if (Customer.IsValid() && Customer->GetVelocity().SizeSquared2D() > 400)
            {
                UAnimInstance* Animation = Customer->GetMesh()->GetAnimInstance();
                FBoolProperty* ShouldMove = Animation ? FindFProperty<FBoolProperty>(Animation->GetClass(), TEXT("ShouldMove")) : nullptr;
                bObservedWalkAnimation |= ShouldMove && ShouldMove->GetPropertyValue_InContainer(Animation);
                const FVector Foot = Customer->GetMesh()->GetSocketTransform(TEXT("foot_l"), RTS_Component).GetLocation();
                bObservedFootMotion |= bHaveFootSample && !Foot.Equals(PreviousFoot, 0.5);
                PreviousFoot = Foot;
                bHaveFootSample = true;
            }

            // The template camera follows an animated head, so settle its view
            // after teleporting before sending a keyboard event.
            if (AimTarget.IsValid())
            {
                const FRotator Desired = (AimTarget->GetActorLocation() + FVector(0, 0, AimHeight) - Camera->GetComponentLocation()).Rotation();
                const FRotator Error = (Desired - Camera->GetComponentRotation()).GetNormalized();
                // Follow the rendered view, including the template's animated
                // head offset, when steering toward small physical controls.
                Controller->SetControlRotation((Controller->GetControlRotation() + Error * 0.5f).GetNormalized());
            }
            if (Now < NextStep)
            {
                return false;
            }

            auto AimAt = [&](AActor* Target)
            {
                Controller->FlushPressedKeys();
                if (Pawn->GetMovementComponent())
                {
                    Pawn->GetMovementComponent()->StopMovementImmediately();
                }
                AimTarget = Target;
                const bool bCustomer = Target->IsA<AShopCustomer>();
                AimHeight = bCustomer ? 0.0f : 12.0f;
                const float Distance = Target->GetActorLocation().Z < 50 ? 100.f : 140.f;
                AShopTill* CheckoutTarget = Cast<AShopTill>(Target);
                if (AShopTillControl* Control = Cast<AShopTillControl>(Target))
                {
                    CheckoutTarget = Control->GetTill();
                }
                const FVector Offset = CheckoutTarget
                    ? CheckoutTarget->GetActorTransform().TransformVectorNoScale(FVector(0, -Distance, 0))
                    : FVector(0, -Distance, 0);
                const FVector Approach(Target->GetActorLocation().X + Offset.X, Target->GetActorLocation().Y + Offset.Y,
                    Pawn->GetSimpleCollisionHalfHeight() + 2.5);
                Pawn->SetActorLocation(Approach, false, nullptr, ETeleportType::TeleportPhysics);
                const FVector AimPoint = Target->GetActorLocation() + FVector(0, 0, AimHeight);
                Controller->SetControlRotation((AimPoint - Camera->GetComponentLocation()).Rotation());
            };
            auto Key = [&](FKey Input, EInputEvent Event)
            {
                Controller->InputKey(FInputKeyEventArgs(nullptr, FInputDeviceId::CreateFromInternalId(0), Input, Event, FPlatformTime::Cycles64()));
            };

            switch (Step)
            {
            case 0:
                if (!Viewport.IsValid())
                {
                    Viewport = World->GetGameViewport();
                    if (!Viewport.IsValid())
                    {
                        Test->AddError(TEXT("Shop input test needs a game viewport"));
                        return true;
                    }
                    bOriginalIgnoreInput = Viewport->IgnoreInput();
                }
                // Inject directly through the controller while isolating the
                // test from keyboard/gamepad events on the shared desktop.
                Viewport->SetIgnoreInput(true);
                {
                    const UEnhancedPlayerInput* Input = Cast<UEnhancedPlayerInput>(Controller->PlayerInput);
                    bool bMappingReady = false;
                    if (Input)
                    {
                        for (const FEnhancedActionKeyMapping& Mapping : Input->GetEnhancedActionMappingsView())
                        {
                            if (Mapping.Key == EKeys::E && Mapping.Action
                                && Mapping.Action->GetFName() == TEXT("NativeInteractAction") && !Mapping.bShouldBeIgnored)
                            {
                                bMappingReady = true;
                                break;
                            }
                        }
                    }
                    if (!bMappingReady)
                    {
                        Controller->FlushPressedKeys();
                        return false;
                    }
                }
                // Offscreen editor automation may never render the player's
                // mesh. Its head-mounted camera still needs fresh bone poses.
                {
                    TInlineComponentArray<USkeletalMeshComponent*> PlayerMeshes(Pawn);
                    for (USkeletalMeshComponent* Mesh : PlayerMeshes)
                    {
                        Mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
                    }
                }
                for (TActorIterator<AShopCustomerFlow> It(World); It; ++It)
                {
                    Flow = *It;
                    break;
                }
                for (TActorIterator<AShopSupplyCrate> It(World); It; ++It)
                {
                    Crate = *It;
                    break;
                }
                if (!Flow.IsValid() || !Crate.IsValid())
                {
                    Test->AddError(TEXT("Shop map needs a customer flow and supply crate"));
                    return true;
                }
                if (AShopGameState* DayState = World->GetGameState<AShopGameState>(); DayState && DayState->GetDayPhase() == EShopDayPhase::Preparation)
                {
                    Test->TestEqual(TEXT("Map starts in preparation"), DayState->GetDayPhase(), EShopDayPhase::Preparation);
                    Test->TestEqual(TEXT("Fresh shop starts with six Moonleaf"), Crate->Station->GetIngredientQuantity(TEXT("Moonleaf")), 6);
                    Test->TestEqual(TEXT("Fresh shop starts with six water"), Crate->Station->GetIngredientQuantity(TEXT("Spring Water")), 6);
                    AimTarget = Crate; AimHeight = 69;
                    Pawn->SetActorLocation(Crate->GetActorLocation() + FVector(0, -140, 96), false, nullptr, ETeleportType::TeleportPhysics);
                    Step = 84; NextStep = Now + 1; return false;
                }
                for (TActorIterator<AShopCustomer> It(World); It; ++It)
                {
                    Customer = *It;
                    break;
                }
                for (TActorIterator<AShopTill> It(World); It; ++It)
                {
                    Till = *It;
                    break;
                }
                if (!Customer.IsValid() || !Till.IsValid() || !World->GetGameState<AShopGameState>())
                {
                    Test->AddError(TEXT("Shop map must contain a customer and the native cash state"));
                    return true;
                }
                for (TActorIterator<AShopBrewingStation> It(World); It; ++It)
                {
                    Station = *It;
                    break;
                }
                if (!Station.IsValid() || !IsValid(Station->OutputSlot))
                {
                    Test->AddError(TEXT("Shop map must contain a brewing station with an output slot"));
                    return true;
                }
                for (TActorIterator<AShopPickupItem> It(World); It; ++It)
                {
                    Item = *It;
                    break;
                }
                if (!Item.IsValid())
                {
                    // The saved shop may have no loose stock. Use its real
                    // recipe presentation for the pickup/transfer fixture.
                    Item = World->SpawnActor<AShopPickupItem>(Station->Recipe->OutputItemClass,
                        FVector(150, 350, 100), FRotator::ZeroRotator);
                }
                for (TActorIterator<AShopShelfSlot> It(World); It; ++It)
                {
                    if (It->IsAvailableForSale() && *It != Station->OutputSlot
                        && (!It->GetSection() || It->GetSection()->GetSlot(0) == *It))
                    {
                        Slots.Add(*It);
                    }
                }
                // This test approaches from negative Y. Use the front row so
                // its slot boxes do not occlude the intended back-row targets.
                Slots.Sort([](const TWeakObjectPtr<AShopShelfSlot>& Left, const TWeakObjectPtr<AShopShelfSlot>& Right)
                {
                    const FVector LeftPosition = Left->GetActorLocation();
                    const FVector RightPosition = Right->GetActorLocation();
                    if (!FMath::IsNearlyEqual(LeftPosition.Y, RightPosition.Y))
                    {
                        return LeftPosition.Y < RightPosition.Y;
                    }
                    return LeftPosition.X < RightPosition.X;
                });
                if (!Item.IsValid() || Slots.Num() < 2)
                {
                    Test->AddError(FString::Printf(TEXT("Shop map fixture is incomplete: item=%s sale-section-first-slots=%d"), *GetNameSafe(Item.Get()), Slots.Num()));
                    return true;
                }
                Customer->RequestedProductId = TEXT("InputFixturePending");
                AimAt(Item.Get());
                break;
            case 1:
                Test->AddInfo(FString::Printf(TEXT("Pawn=%s Camera=%s Rotation=%s Control=%s Target=%s Item=%s"),
                    *Pawn->GetActorLocation().ToString(), *Camera->GetComponentLocation().ToString(),
                    *Camera->GetComponentRotation().ToString(), *Controller->GetControlRotation().ToString(),
                    *GetNameSafe(Controller->ShopInteraction->GetInteractionTarget()), *Item->GetActorLocation().ToString()));
                Test->TestEqual(TEXT("Shop trace targets the migrated item"), Controller->ShopInteraction->GetInteractionTarget(), static_cast<AActor*>(Item.Get()));
                Key(EKeys::E, IE_Pressed);
                break;
            case 2:
                Key(EKeys::E, IE_Released);
                if (!Test->TestEqual(TEXT("E picks up the item in the shop map"), Controller->ShopInteraction->GetHeldItem(), Item.Get()))
                {
                    return true;
                }
                AimAt(Slots[0].Get());
                break;
            case 3:
                Key(EKeys::E, IE_Pressed);
                break;
            case 4:
                Key(EKeys::E, IE_Released);
                Test->TestNull(TEXT("E placement frees the hand"), Controller->ShopInteraction->GetHeldItem());
                Test->TestEqual(TEXT("First slot owns the item after E placement"), Slots[0]->GetOccupyingItem(), Item.Get());
                AimAt(Slots[0].Get());
                break;
            case 5:
                Key(EKeys::E, IE_Pressed);
                break;
            case 6:
                Key(EKeys::E, IE_Released);
                Test->TestEqual(TEXT("E retrieves the item"), Controller->ShopInteraction->GetHeldItem(), Item.Get());
                Test->TestNull(TEXT("Retrieval frees the first slot"), Slots[0]->GetOccupyingItem());
                AimTarget.Reset();
                Controller->FlushPressedKeys();
                Pawn->GetMovementComponent()->StopMovementImmediately();
                Pawn->SetActorLocation(FVector(350, -200, Pawn->GetSimpleCollisionHalfHeight() + 2.5), false, nullptr, ETeleportType::TeleportPhysics);
                Controller->SetControlRotation(FRotator(-60, 90, 0));
                Step = 36;
                NextStep = Now + 0.2;
                return false;
            case 7:
                Key(EKeys::E, IE_Pressed);
                break;
            case 8:
                Key(EKeys::E, IE_Released);
                Test->TestEqual(TEXT("E transfers the item to the second slot"), Slots[1]->GetOccupyingItem(), Item.Get());
                Test->TestNull(TEXT("Transfer frees the hand"), Controller->ShopInteraction->GetHeldItem());
                if (FObjectProperty* LegacyHeldItem = FindFProperty<FObjectProperty>(Pawn->GetClass(), TEXT("HeldItem")))
                {
                    Test->TestNull(TEXT("Native input consumes E before the legacy Blueprint interaction"), LegacyHeldItem->GetObjectPropertyValue_InContainer(Pawn));
                }
                MovementStart = Pawn->GetActorLocation();
                Key(EKeys::W, IE_Pressed);
                break;
            case 9:
                Key(EKeys::W, IE_Released);
                Test->TestTrue(TEXT("Template movement still responds to W"), FVector::Dist2D(MovementStart, Pawn->GetActorLocation()) > 1.0);
                AimTarget.Reset();
                LookStart = Controller->GetControlRotation();
                CameraStart = Camera->GetComponentRotation();
                Controller->InputKey(FInputKeyEventArgs(nullptr, FInputDeviceId::CreateFromInternalId(0), EKeys::MouseX, 20.0f, 1.0f / 60.0f, 1, FPlatformTime::Cycles64()));
                break;
            case 10:
                Test->TestTrue(TEXT("Mouse X changes controller yaw"), FMath::Abs(FMath::FindDeltaAngleDegrees(LookStart.Yaw, Controller->GetControlRotation().Yaw)) > 0.1f);
                Test->TestTrue(TEXT("Camera follows horizontal mouse look"), FMath::Abs(FMath::FindDeltaAngleDegrees(CameraStart.Yaw, Camera->GetComponentRotation().Yaw)) > 0.1f);
                LookStart = Controller->GetControlRotation();
                CameraStart = Camera->GetComponentRotation();
                Controller->InputKey(FInputKeyEventArgs(nullptr, FInputDeviceId::CreateFromInternalId(0), EKeys::MouseY, 20.0f, 1.0f / 60.0f, 1, FPlatformTime::Cycles64()));
                break;
            case 11:
                Item->Destroy();
                Customer->RequestedProductId = TEXT("DawnTonic");
                Test->TestTrue(TEXT("Mouse Y changes controller pitch"), FMath::Abs(FMath::FindDeltaAngleDegrees(LookStart.Pitch, Controller->GetControlRotation().Pitch)) > 0.1f);
                Test->TestTrue(TEXT("Camera follows vertical mouse look"), FMath::Abs(FMath::FindDeltaAngleDegrees(CameraStart.Pitch, Camera->GetComponentRotation().Pitch)) > 0.1f);
                AimAt(Station.Get());
                LeavesBeforeBrew = Station->GetIngredientQuantity(TEXT("Moonleaf"));
                WaterBeforeBrew = Station->GetIngredientQuantity(TEXT("Spring Water"));
                break;
            case 12:
                LeavesPerBrew = Station->Recipe->IngredientCosts.FindRef(TEXT("Moonleaf"));
                WaterPerBrew = Station->Recipe->IngredientCosts.FindRef(TEXT("Spring Water"));
                Test->AddInfo(FString::Printf(TEXT("Brew input visit=%d recipe=%s target=%s output=%s leaves=%d water=%d pawn=%s camera=%s"),
                    Visits + 1, *Station->Recipe->ProductId.ToString(), *GetNameSafe(Controller->ShopInteraction->GetInteractionTarget()),
                    *GetNameSafe(Station->OutputSlot->GetOccupyingItem()), Station->GetIngredientQuantity(TEXT("Moonleaf")),
                    Station->GetIngredientQuantity(TEXT("Spring Water")), *Pawn->GetActorLocation().ToString(), *Camera->GetComponentLocation().ToString()));
                Key(EKeys::E, IE_Pressed);
                break;
            case 13:
                Key(EKeys::E, IE_Released);
                Item = Station->OutputSlot->GetOccupyingItem();
                if (!Test->TestNotNull(TEXT("E brews a potion in the actual shop"), Item.Get()))
                {
                    Test->AddInfo(FString::Printf(TEXT("Rejected brew prompt=%s target=%s class=%s flags=%x held=%s"),
                        *Station->GetInteractionPrompt(Controller->ShopInteraction).ToString(),
                        *GetNameSafe(Controller->ShopInteraction->GetInteractionTarget()), *GetNameSafe(Station->Recipe->OutputItemClass.Get()),
                        static_cast<uint32>(Station->Recipe->OutputItemClass->ClassFlags), *GetNameSafe(Controller->ShopInteraction->GetHeldItem())));
                    return true;
                }
                Test->TestEqual(TEXT("Shop recipe consumes its Moonleaf cost"), Station->GetIngredientQuantity(TEXT("Moonleaf")), LeavesBeforeBrew - LeavesPerBrew);
                Test->TestEqual(TEXT("Shop recipe consumes its water cost"), Station->GetIngredientQuantity(TEXT("Spring Water")), WaterBeforeBrew - WaterPerBrew);
                break;
            case 14:
                Key(EKeys::E, IE_Pressed);
                break;
            case 15:
                Key(EKeys::E, IE_Released);
                Test->TestEqual(TEXT("Repeated E with occupied output preserves stock"), Station->GetIngredientQuantity(TEXT("Moonleaf")), LeavesBeforeBrew - LeavesPerBrew);
                Test->TestEqual(TEXT("Repeated E retains the same output item"), Station->OutputSlot->GetOccupyingItem(), Item.Get());
                AimAt(Item.Get());
                break;
            case 16:
                Key(EKeys::E, IE_Pressed);
                break;
            case 17:
                Key(EKeys::E, IE_Released);
                Test->TestEqual(TEXT("E collects the brewed potion"), Controller->ShopInteraction->GetHeldItem(), Item.Get());
                Test->TestNull(TEXT("Collection frees brewing output"), Station->OutputSlot->GetOccupyingItem());
                AimAt(Slots[0].Get());
                break;
            case 18:
                Key(EKeys::E, IE_Pressed);
                break;
            case 19:
                Key(EKeys::E, IE_Released);
                Test->TestEqual(TEXT("E shelves the brewed potion"), Slots[0]->GetOccupyingItem(), Item.Get());
                Test->TestEqual(TEXT("Shelved potion keeps its product identity"), Item->ProductId, Station->Recipe->ProductId);
                CashBeforeSale = World->GetGameState<AShopGameState>()->GetCash();
                SalePrice = Customer->PriceBits;
                AimTarget.Reset();
                Pawn->SetActorLocation(FVector(50, 150, 96), false, nullptr, ETeleportType::TeleportPhysics);
                break;
            case 20:
                if (Customer->Activity != EShopCustomerActivity::ReadyToPay)
                {
                    if (Now > NextCustomerDiagnostic)
                    {
                        const UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
                        TArray<FBox> Bounds;
                        if (Navigation && Navigation->GetDefaultNavDataInstance())
                        {
                            Navigation->GetNavigationBoundsForNavData(*Navigation->GetDefaultNavDataInstance(), Bounds);
                        }
                        UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(World,
                            Customer->GetActorLocation(), Slots[0]->GetActorLocation() + FVector(0, -100, -100), Customer.Get());
                        UNavigationPath* DefaultPath = UNavigationSystemV1::FindPathToLocationSynchronously(World,
                            Customer->GetActorLocation(), Slots[0]->GetActorLocation() + FVector(0, -100, -100));
                        const FNavAgentProperties& Agent = Customer->GetNavAgentPropertiesRef();
                        Test->AddInfo(FString::Printf(TEXT("Waiting for customer: activity=%d position=%s controller=%s navigation=%s path=%d points=%d bounds=%d registered=%d building=%d"),
                            static_cast<int32>(Customer->Activity), *Customer->GetActorLocation().ToString(), *GetNameSafe(Customer->GetController()),
                            Navigation ? *GetNameSafe(Navigation->GetDefaultNavDataInstance()) : TEXT("missing"),
                            Path && Path->IsValid(), Path ? Path->PathPoints.Num() : 0, Bounds.Num(),
                            Navigation ? Navigation->GetNavigationBounds().Num() : -1,
                            UNavigationSystemV1::IsNavigationBeingBuilt(World)));
                        Test->AddInfo(FString::Printf(TEXT("Customer navigation agent radius=%f height=%f default-path=%d bounds=%s"),
                            Agent.AgentRadius, Agent.AgentHeight, DefaultPath && DefaultPath->IsValid(),
                            Bounds.Num() ? *Bounds[0].ToString() : TEXT("missing")));
                        NextCustomerDiagnostic = Now + 5;
                    }
                    return false;
                }
                Test->TestTrue(TEXT("Customer carries the brewed potion or has just placed it on the counter"), Customer->ShopInteraction->GetHeldItem() == Item.Get() || Customer->CounterItem.Get() == Item.Get());
                Test->TestTrue(TEXT("Moving customer enters the walking animation"), bObservedWalkAnimation);
                Test->TestTrue(TEXT("Moving customer's foot pose advances"), bObservedFootMotion);
                Test->TestNull(TEXT("Customer pickup frees the shelf before payment"), Slots[0]->GetOccupyingItem());
                Test->TestEqual(TEXT("Customer collection credits no money"), World->GetGameState<AShopGameState>()->GetCash(), CashBeforeSale);
                AimAt(Till.Get());
                break;
            case 21:
                Test->AddInfo(FString::Printf(TEXT("Till start target=%s till=%s root=%s camera=%s customer=%s phase=%d feedback=%s"),
                    *GetNameSafe(Controller->ShopInteraction->GetInteractionTarget()), *GetNameSafe(Till.Get()),
                    *Till->GetActorLocation().ToString(), *Camera->GetComponentLocation().ToString(),
                    *GetNameSafe(Till->Customer), static_cast<int32>(Till->GetPhase()), *Till->GetFeedback().ToString()));
                Key(EKeys::E, IE_Pressed);
                break;
            case 22:
                Key(EKeys::E, IE_Released);
                Test->TestEqual(TEXT("Customer automatically presents payment"), Till->GetPhase(), EShopTillPhase::MakingChange);
                Test->TestEqual(TEXT("Real goods rest on this counter"), Item->GetCurrentSlot() ? Item->GetCurrentSlot()->GetCheckoutTill() : nullptr, Till.Get());
                Test->TestNull(TEXT("Customer has put goods down"), Customer->ShopInteraction->GetHeldItem());
                Test->TestFalse(TEXT("Physical checkout preserves mouse look"), Controller->IsLookInputIgnored());
                Test->TestFalse(TEXT("Physical checkout preserves movement"), Controller->IsMoveInputIgnored());
                AimAt(Till->FindControl(EShopTillControl::AddCoin, 1));
                AimHeight = 0;
                break;
            case 23:
                Key(EKeys::E, IE_Pressed);
                break;
            case 24:
                Key(EKeys::E, IE_Released);
                Test->TestEqual(TEXT("E on the coin stack adds a visible bit"), Till->GetCoinCount(1), 1);
                for (int32 Bit = 1; Bit < Customer->TenderBits - SalePrice; ++Bit)
                {
                    Test->TestTrue(TEXT("Physical stack supplies each change coin"), Controller->ShopInteraction->Interact());
                }
                Test->TestEqual(TEXT("Change tray shows the actual counted coins"), Till->GetVisibleChangeCoins(1), Customer->TenderBits - SalePrice);
                Test->TestEqual(TEXT("Counting change earns no money before confirmation"), World->GetGameState<AShopGameState>()->GetCash(), CashBeforeSale);
                {
                    FString Screenshot;
                    if (FParse::Value(FCommandLine::Get(), TEXT("ShopTillScreenshot="), Screenshot))
                    {
                        AimAt(Till.Get());
                        AimHeight = 115;
                        Step = 42;
                        NextStep = Now + 1;
                        return false;
                    }
                }
                LookStart = Controller->GetControlRotation();
                AimTarget.Reset();
                Controller->InputKey(FInputKeyEventArgs(nullptr, FInputDeviceId::CreateFromInternalId(0), EKeys::MouseX, 20.0f, 1.0f / 60.0f, 1, FPlatformTime::Cycles64()));
                break;
            case 25:
                Test->TestTrue(TEXT("Mouse look works throughout checkout"), FMath::Abs(FMath::FindDeltaAngleDegrees(LookStart.Yaw, Controller->GetControlRotation().Yaw)) > 0.1f);
                AimAt(Till->FindControl(EShopTillControl::Confirm));
                AimHeight = 0;
                break;
            case 26:
                Key(EKeys::E, IE_Pressed);
                break;
            case 72:
                {
                    FString Screenshot;
                    if (FParse::Value(FCommandLine::Get(), TEXT("ShopPaidScreenshot="), Screenshot))
                        FScreenshotRequest::RequestScreenshot(Screenshot, false, false);
                }
                Step = 73; NextStep = Now + 0.2;
                return false;
            case 73:
                AimAt(Till->FindControl(EShopTillControl::Confirm)); AimHeight = 0;
                Step = 27; NextStep = Now + 0.2;
                return false;
            case 27:
                Key(EKeys::E, IE_Released);
                {
                    FString Screenshot;
                    if (!bCapturedSale && FParse::Value(FCommandLine::Get(), TEXT("ShopPaidScreenshot="), Screenshot))
                    {
                        bCapturedSale = true;
                        AimAt(Till.Get()); AimHeight = 115;
                        Step = 72; NextStep = Now + 0.1;
                        return false;
                    }
                }
                Test->TestTrue(TEXT("Physical SALE key completes the customer order"), Customer->HasCompletedSale());
                Test->TestEqual(TEXT("E sale credits the quoted price"), World->GetGameState<AShopGameState>()->GetCash(), CashBeforeSale + SalePrice);
                Test->TestFalse(TEXT("Sold potion leaves the world"), Item.IsValid());
                Test->TestTrue(TEXT("Paid customer carries their purchased bottle out"), Customer->PurchasedItemVisual->IsVisible() && Customer->PurchasedItemVisual->GetStaticMesh());
                Test->TestNull(TEXT("Sale frees the stocked shelf"), Slots[0]->GetOccupyingItem());
                Test->TestFalse(TEXT("Repeated physical confirmation cannot pay twice"), Controller->ShopInteraction->Interact());
                Test->TestEqual(TEXT("Repeated confirmation earns no extra cash"), World->GetGameState<AShopGameState>()->GetCash(), CashBeforeSale + SalePrice);
                bCapturedSale = false;
                ++Visits;
                if (Visits >= 3)
                {
                    TActorIterator<AShopDoorSign> Sign(World);
                    AimTarget = *Sign; AimHeight = 0;
                    Pawn->SetActorLocation(Sign->GetActorLocation() + FVector(0, 110, -59), false, nullptr, ETeleportType::TeleportPhysics);
                    Step = 53; NextStep = Now + 1;
                    return false;
                }
                Step = 31;
                NextStep = Now + 0.2;
                return false;
            case 28:
                AimTarget = Crate;
                AimHeight = 35;
                Pawn->SetActorLocation(Crate->GetActorLocation() + FVector(0, -140, 96), false, nullptr, ETeleportType::TeleportPhysics);
                LeavesBeforeBrew = Station->GetIngredientQuantity(TEXT("Moonleaf"));
                WaterBeforeBrew = Station->GetIngredientQuantity(TEXT("Spring Water"));
                CashBeforeSupplies = World->GetGameState<AShopGameState>()->GetCash();
                SupplyChoiceCycles = 0;
                Step = 40;
                NextStep = Now + 0.2;
                return false;
            case 29:
                Key(EKeys::E, IE_Pressed);
                Step = 81; NextStep = Now + 0.5;
                return false;
            case 81:
                Key(EKeys::E, IE_Released);
                Test->TestTrue(TEXT("E opens the supplier ordering page"), Controller->IsOrderingPageOpen());
                Test->TestEqual(TEXT("Reading the book cannot spend cash"), World->GetGameState<AShopGameState>()->GetCash(), CashBeforeSupplies);
                Test->TestTrue(TEXT("Reading the page blocks movement"), Controller->IsMoveInputIgnored());
                // Route through the focused Slate page, not just the controller.
                FSlateApplication::Get().ProcessKeyDownEvent(FKeyEvent(EKeys::T, FModifierKeysState(), 0, false, 0, 0));
                Test->TestEqual(TEXT("Ordering page receives quantity keys through Slate"), Crate->GetPackCount(), 2);
                FSlateApplication::Get().ProcessKeyDownEvent(FKeyEvent(EKeys::F, FModifierKeysState(), 0, false, 0, 0));
                Test->TestEqual(TEXT("Page can reduce quantity before paying"), Crate->GetPackCount(), 1);
                {
                    FString Screenshot;
                    if (FParse::Value(FCommandLine::Get(), TEXT("ShopOrderingScreenshot="), Screenshot))
                        FScreenshotRequest::RequestScreenshot(Screenshot, true, false);
                }
                Step = 82; NextStep = Now + 0.5;
                return false;
            case 82:
                Key(EKeys::Enter, IE_Pressed); Key(EKeys::Enter, IE_Released);
                Key(EKeys::Enter, IE_Pressed); Key(EKeys::Enter, IE_Released);
                Test->TestEqual(TEXT("Page confirmation charges once"), World->GetGameState<AShopGameState>()->GetCash(), CashBeforeSupplies - Crate->PriceBits);
                Key(EKeys::F10, IE_Pressed); Key(EKeys::F10, IE_Released);
                Test->TestFalse(TEXT("F10 closes the ordering page"), Controller->IsOrderingPageOpen());
                Test->TestFalse(TEXT("Closing book restores movement"), Controller->IsMoveInputIgnored());
                Step = 30; NextStep = Now + 0.2;
                return false;
            case 84:
                Key(EKeys::E, IE_Pressed);
                Step = 85; NextStep = Now + 0.5;
                return false;
            case 85:
                Key(EKeys::E, IE_Released);
                Test->TestTrue(TEXT("Preparation allows reading the supplier page"), Controller->IsOrderingPageOpen());
                Key(EKeys::Enter, IE_Pressed); Key(EKeys::Enter, IE_Released);
                Test->TestFalse(TEXT("Preparation cannot submit supply orders"), World->GetGameState<AShopGameState>()->HasConfirmedSupplies());
                Key(EKeys::F10, IE_Pressed); Key(EKeys::F10, IE_Released);
                {
                    TActorIterator<AShopDoorSign> Sign(World);
                    if (!Test->TestTrue(TEXT("Shop has an open/closed sign"), bool(Sign))) return true;
                    AimTarget = *Sign; AimHeight = 0;
                    Pawn->SetActorLocation(Sign->GetActorLocation() + FVector(0, 110, -59), false, nullptr, ETeleportType::TeleportPhysics);
                }
                Step = 51; NextStep = Now + 1;
                return false;
            case 30:
                Key(EKeys::E, IE_Released);
                Test->TestEqual(TEXT("Evening order does not deliver Moonleaf immediately"), Station->GetIngredientQuantity(TEXT("Moonleaf")), LeavesBeforeBrew);
                Test->TestEqual(TEXT("Evening order does not deliver water immediately"), Station->GetIngredientQuantity(TEXT("Spring Water")), WaterBeforeBrew);
                Test->TestEqual(TEXT("Supply delivery charges the advertised price"), World->GetGameState<AShopGameState>()->GetCash(), CashBeforeSupplies - Crate->PriceBits);
                for (TActorIterator<AShopSupplyControl> It(World); It; ++It)
                    if (It->GetAction() == EShopSupplyControl::Rest && It->GetDesk() == Crate)
                    { AimTarget = *It; AimHeight = 0; break; }
                Step = 45;
                NextStep = Now + 0.4;
                return false;
            case 31:
                if (Flow->GetActiveCustomer() == Customer.Get() || !IsValid(Flow->GetActiveCustomer())
                    || Flow->GetActiveCustomer()->Activity != EShopCustomerActivity::Browsing)
                {
                    return false;
                }
                Test->TestFalse(TEXT("Paid customer walks to the exit and leaves the world"), Customer.IsValid());
                Test->TestEqual(TEXT("Service counts each paid visit once"), Flow->GetCompletedVisits(), Visits);
                Customer = Flow->GetActiveCustomer();
                if (Flow->Orders.IsEmpty())
                {
                    Test->AddError(TEXT("Shop customer flow needs its authored order sequence"));
                    return true;
                }
                {
                    const FShopCustomerOrder& Order = Flow->Orders[Visits % Flow->Orders.Num()];
                    Test->TestEqual(TEXT("Next visitor follows the authored product sequence"), Customer->RequestedProductId, Order.ProductId);
                    Test->TestEqual(TEXT("Next visitor has the authored presentation class"), Customer->GetClass(), Order.CustomerClass.Get());
                    Test->TestEqual(TEXT("Next visitor has the authored tender"), Customer->TenderBits, Order.TenderBits);
                }
                Test->TestEqual(TEXT("New arrival is bound to the till"), Till->Customer.Get(), Customer.Get());
                Test->TestEqual(TEXT("New arrival clears the old receipt"), Till->GetPhase(), EShopTillPhase::Idle);
                AimAt(Station.Get());
                LeavesBeforeBrew = Station->GetIngredientQuantity(TEXT("Moonleaf"));
                WaterBeforeBrew = Station->GetIngredientQuantity(TEXT("Spring Water"));
                Step = Station->Recipe->ProductId == Customer->RequestedProductId ? 12 : 34;
                NextStep = Now + 0.2;
                return false;
            case 32:
                {
                    FString Screenshot;
                    FParse::Value(FCommandLine::Get(), TEXT("ShopCornerScreenshot="), Screenshot);
                    FScreenshotRequest::RequestScreenshot(Screenshot, false, false);
                }
                break;
            case 33:
                return true;
            case 34:
                Controller->FlushPressedKeys();
                Pawn->GetMovementComponent()->StopMovementImmediately();
                RecipeSelectionPosition = Pawn->GetActorLocation();
                Key(EKeys::R, IE_Pressed);
                break;
            case 35:
                Key(EKeys::R, IE_Released);
                Test->TestEqual(TEXT("R selects the next customer's recipe"), Station->Recipe->ProductId, Customer->RequestedProductId);
                Test->TestTrue(TEXT("Recipe selection preserves the player's horizontal position"), FVector::Dist2D(Pawn->GetActorLocation(), RecipeSelectionPosition) < 1);
                Step = 12;
                NextStep = Now + 0.2;
                return false;
            case 36:
                Key(EKeys::G, IE_Pressed);
                break;
            case 37:
                Key(EKeys::G, IE_Released);
                if (!Test->TestNull(TEXT("G sets the held bottle onto a clear floor"), Controller->ShopInteraction->GetHeldItem()))
                {
                    return true;
                }
                Test->TestNull(TEXT("Set-down leaves the bottle outside sale stock"), Item->GetCurrentSlot());
                AimAt(Item.Get());
                break;
            case 38:
                Key(EKeys::E, IE_Pressed);
                break;
            case 39:
                Key(EKeys::E, IE_Released);
                Test->TestEqual(TEXT("E retrieves a bottle from the floor"), Controller->ShopInteraction->GetHeldItem(), Item.Get());
                AimAt(Slots[1].Get());
                Step = 7;
                NextStep = Now + 0.2;
                return false;
            case 42:
                {
                    FString Screenshot;
                    FParse::Value(FCommandLine::Get(), TEXT("ShopTillScreenshot="), Screenshot);
                    FScreenshotRequest::RequestScreenshot(Screenshot, false, false);
                    Step = 43;
                    NextStep = Now + 0.5;
                    return false;
                }
            case 43:
                LookStart = Controller->GetControlRotation();
                AimTarget.Reset();
                Controller->InputKey(FInputKeyEventArgs(nullptr, FInputDeviceId::CreateFromInternalId(0), EKeys::MouseX, 20.0f, 1.0f / 60.0f, 1, FPlatformTime::Cycles64()));
                Step = 25;
                NextStep = Now + 0.2;
                return false;
            case 44:
                if (World->GetGameState<AShopGameState>()->GetDayPhase() != EShopDayPhase::Evening)
                {
                    NextStep = Now + 0.2;
                    return false;
                }
                Test->TestNull(TEXT("Final departure leaves no fourth visitor"), Flow->GetActiveCustomer());
                Test->TestEqual(TEXT("Three shoppers overlap in the rendered shop"), MaxCustomersObserved, 3);
                Test->TestEqual(TEXT("Day includes both products sold"), World->GetGameState<AShopGameState>()->GetTakings(), int64(42));
                AimTarget = Crate;
                AimHeight = 69;
                Pawn->SetActorLocation(Crate->GetActorLocation() + FVector(0, -100, 96), false, nullptr, ETeleportType::TeleportPhysics);
                Step = 48;
                NextStep = Now + 5;
                return false;
            case 48:
                {
                    FString Screenshot;
                    if (FParse::Value(FCommandLine::Get(), TEXT("ShopEveningScreenshot="), Screenshot))
                        FScreenshotRequest::RequestScreenshot(Screenshot, false, false);
                }
                World->GetGameState<AShopGameState>()->ConfirmNoSupplies();
                Key(EKeys::N, IE_Pressed); Key(EKeys::N, IE_Released);
                Test->TestTrue(TEXT("Rest warns about unplanned shortages"), Controller->IsRestPromptOpen());
                Step = 86;
                NextStep = Now + 0.5;
                return false;
            case 86:
                {
                    FString Screenshot;
                    if (FParse::Value(FCommandLine::Get(), TEXT("ShopShortageScreenshot="), Screenshot))
                        FScreenshotRequest::RequestScreenshot(Screenshot, false, false);
                }
                Step = 87; NextStep = Now + 0.5;
                return false;
            case 87:
                Key(EKeys::F10, IE_Pressed); Key(EKeys::F10, IE_Released);
                Test->TestFalse(TEXT("Return to planning cancels rest"), Controller->IsRestPromptOpen());
                Step = 74; NextStep = Now + 0.5;
                return false;
            case 74:
                for (TActorIterator<AShopStorageUpgrade> It(World); It; ++It)
                {
                    AimTarget = *It;
                    AimHeight = 45;
                    Pawn->SetActorLocation(It->GetActorLocation() + FVector(0, -140, 96), false, nullptr, ETeleportType::TeleportPhysics);
                    break;
                }
                Step = 75;
                NextStep = Now + 1;
                return false;
            case 75:
                {
                    FString Screenshot;
                    if (FParse::Value(FCommandLine::Get(), TEXT("ShopStorageScreenshot="), Screenshot))
                        FScreenshotRequest::RequestScreenshot(Screenshot, false, false);
                }
                Step = 76;
                NextStep = Now + 0.5;
                return false;
            case 76:
                for (TActorIterator<AShopBrewingUpgrade> It(World); It; ++It)
                {
                    AimTarget = *It;
                    AimHeight = -12;
                    Pawn->SetActorLocation(It->GetActorLocation() + FVector(0, -140, 0), false, nullptr, ETeleportType::TeleportPhysics);
                    break;
                }
                Step = 77;
                NextStep = Now + 1;
                return false;
            case 77:
                {
                    FString Screenshot;
                    if (FParse::Value(FCommandLine::Get(), TEXT("ShopBatchScreenshot="), Screenshot))
                        FScreenshotRequest::RequestScreenshot(Screenshot, false, false);
                }
                Step = 78;
                NextStep = Now + 0.5;
                return false;
            case 78:
                // Controller injection alone misses editor shortcuts such as Esc/Stop PIE.
                Test->TestFalse(TEXT("Editor play commands do not consume recovery key during play"),
                    FPlayWorldCommands::GlobalPlayWorldActions->ProcessCommandBindings(EKeys::F10, FModifierKeysState(), false));
                Key(EKeys::F10, IE_Pressed);
                Key(EKeys::F10, IE_Released);
                Test->TestTrue(TEXT("F10 offers restart confirmation"), Controller->IsRestartPromptOpen());
                Key(EKeys::F, IE_Pressed);
                Key(EKeys::F, IE_Released);
                Test->TestTrue(TEXT("New game has a separate confirmation"), Controller->IsFreshGameConfirmation());
                Step = 79;
                NextStep = Now + 0.5;
                return false;
            case 79:
                {
                    FString Screenshot;
                    if (FParse::Value(FCommandLine::Get(), TEXT("ShopRestartScreenshot="), Screenshot))
                        FScreenshotRequest::RequestScreenshot(Screenshot, false, false);
                }
                Step = 80;
                NextStep = Now + 0.5;
                return false;
            case 80:
                Key(EKeys::F10, IE_Pressed);
                Key(EKeys::F10, IE_Released);
                Test->TestFalse(TEXT("F10 cancels restart"), Controller->IsRestartPromptOpen());
                Test->TestFalse(TEXT("Cancel restores movement input"), Controller->IsMoveInputIgnored());
                Step = 28;
                NextStep = Now + 0.5;
                return false;
            case 45:
                Key(EKeys::E, IE_Pressed);
                Step = 64;
                NextStep = Now + 0.2;
                return false;
            case 64:
                Key(EKeys::E, IE_Released);
                Test->TestTrue(TEXT("Rest tab begins overnight"), World->GetGameState<AShopGameState>()->IsOvernight());
                Step = 69;
                NextStep = Now + 1.7;
                return false;
            case 69:
                {
                    FString Screenshot;
                    if (FParse::Value(FCommandLine::Get(), TEXT("ShopNightScreenshot="), Screenshot))
                        FScreenshotRequest::RequestScreenshot(Screenshot, false, false);
                }
                Step = 65; NextStep = Now + 4.3;
                return false;
            case 65:
                Test->TestEqual(TEXT("Rest advances to morning two"), World->GetGameState<AShopGameState>()->GetDay(), 2);
                Test->TestEqual(TEXT("Next morning requires explicit reopening"), World->GetGameState<AShopGameState>()->GetDayPhase(), EShopDayPhase::Preparation);
                Test->TestNull(TEXT("Preparation has no customer"), Flow->GetActiveCustomer());
                Test->TestEqual(TEXT("Morning storage waits for unpacking"), Station->GetIngredientQuantity(TEXT("Moonleaf")), LeavesBeforeBrew);
                for (TActorIterator<AShopSupplyControl> It(World); It; ++It)
                    if (It->GetAction() == EShopSupplyControl::Delivery)
                    { AimTarget = *It; AimHeight = 28; Pawn->SetActorLocation(It->GetActorLocation() + FVector(0, -110, 96), false, nullptr, ETeleportType::TeleportPhysics); break; }
                Step = 66; NextStep = Now + 2;
                return false;
            case 66:
                {
                    FString Screenshot;
                    if (FParse::Value(FCommandLine::Get(), TEXT("ShopMorningScreenshot="), Screenshot))
                        FScreenshotRequest::RequestScreenshot(Screenshot, false, false);
                }
                Step = 67; NextStep = Now + 0.2;
                return false;
            case 67:
                Key(EKeys::E, IE_Pressed);
                Step = 68; NextStep = Now + 0.2;
                return false;
            case 68:
                Key(EKeys::E, IE_Released);
                Test->TestEqual(TEXT("Unpack six paid Moonleaf"), Station->GetIngredientQuantity(TEXT("Moonleaf")), LeavesBeforeBrew + 6);
                Test->TestEqual(TEXT("Unpack three paid water"), Station->GetIngredientQuantity(TEXT("Spring Water")), WaterBeforeBrew + 3);
                Step = 46; NextStep = Now + 0.2;
                return false;
            case 49:
                AimTarget.Reset();
                Pawn->SetActorLocation(FVector(400, 70, 160), false, nullptr, ETeleportType::TeleportPhysics);
                Controller->SetControlRotation((FVector(665, 370, 120) - Camera->GetComponentLocation()).Rotation());
                Step = 50;
                NextStep = Now + 3;
                return false;
            case 50:
                {
                    FString Screenshot;
                    if (FParse::Value(FCommandLine::Get(), TEXT("ShopMorningScreenshot="), Screenshot))
                        FScreenshotRequest::RequestScreenshot(Screenshot, false, false);
                }
                Step = 46;
                NextStep = Now + 0.2;
                return false;
            case 46:
                // A separate stocked-morning fixture verifies physical queue navigation.
                // Reuse ownership APIs; customers still walk, collect and return unaided.
                {
                    APawn* Stocker = World->SpawnActor<APawn>();
                    UCameraComponent* Eyes = NewObject<UCameraComponent>(Stocker);
                    Stocker->SetRootComponent(Eyes); Eyes->RegisterComponent(); Eyes->Activate(true);
                    UShopInteractionComponent* Hands = NewObject<UShopInteractionComponent>(Stocker); Hands->RegisterComponent();
                    for (int32 Index = 0; Index < 3; ++Index)
                    {
                        AShopShelfSlot* Slot = Slots[Index].Get();
                        Stocker->SetActorLocation(Slot->GetActorLocation() + FVector(0, -100, 10));
                        UShopBrewingRecipe* Recipe = Station->AvailableRecipes[Index == 1 ? 1 : 0];
                        AShopPickupItem* Bottle = World->SpawnActor<AShopPickupItem>(Recipe->OutputItemClass,
                            Stocker->GetActorLocation() + FVector(0, 35, 0), FRotator::ZeroRotator);
                        Eyes->SetWorldRotation((Bottle->GetActorLocation() + FVector(0, 0, 10) - Eyes->GetComponentLocation()).Rotation());
                        if (!Test->TestTrue(TEXT("Queue fixture picks up real recipe bottle"), Bottle->PickUp(Hands))) { Stocker->Destroy(); return true; }
                        Eyes->SetWorldRotation((Slot->GetActorLocation() + FVector(0, 0, 10) - Eyes->GetComponentLocation()).Rotation());
                        if (!Test->TestTrue(TEXT("Queue fixture stocks through shared placement"), Bottle->PlaceInSlot(Hands, Slot))) { Stocker->Destroy(); return true; }
                    }
                    Stocker->Destroy();
                    AimTarget.Reset();
                    Pawn->SetActorLocation(FVector(-100, -80, 96), false, nullptr, ETeleportType::TeleportPhysics);
                    Controller->SetControlRotation((Flow->GetQueuePosition(1) - Camera->GetComponentLocation()).Rotation());
                    World->GetGameState<AShopGameState>()->OpenShop();
                    Step = 88; NextStep = Now + 1;
                    return false;
                }
            case 88:
                if (Flow->GetCustomersInShop() != 3 || !Flow->GetCustomerAt(1) || !Flow->GetCustomerAt(2)
                    || Flow->GetCustomerAt(1)->Activity != EShopCustomerActivity::Queued
                    || Flow->GetCustomerAt(2)->Activity != EShopCustomerActivity::Queued
                    || Till->GetPhase() != EShopTillPhase::MakingChange) return false;
                Test->TestEqual(TEXT("Only queue front owns the counter"), Till->Customer.Get(), Flow->GetCustomerAt(0));
                {
                    FString Screenshot;
                    if (FParse::Value(FCommandLine::Get(), TEXT("ShopQueueScreenshot="), Screenshot))
                        FScreenshotRequest::RequestScreenshot(Screenshot, false, false);
                }
                Step = 89; NextStep = Now + 1;
                return false;
            case 89:
                {
                    FString Screenshot;
                    if (!FParse::Value(FCommandLine::Get(), TEXT("ShopCornerScreenshot="), Screenshot)) return true;
                    Pawn->SetActorLocation(FVector(400, 70, 160), false, nullptr, ETeleportType::TeleportPhysics);
                    Controller->SetControlRotation((FVector(665, 370, 120) - Camera->GetComponentLocation()).Rotation());
                    Step = 32;
                    NextStep = Now + 3;
                    return false;
                }
            case 51:
                Key(EKeys::E, IE_Pressed);
                break;
            case 52:
                Key(EKeys::E, IE_Released);
                Test->TestEqual(TEXT("E turns the sign to open"), World->GetGameState<AShopGameState>()->GetDayPhase(), EShopDayPhase::Trading);
                {
                    FString Screenshot;
                    if (FParse::Value(FCommandLine::Get(), TEXT("ShopSignScreenshot="), Screenshot))
                        FScreenshotRequest::RequestScreenshot(Screenshot, false, false);
                }
                if (!Flow->GetActiveCustomer()) Flow->StartNextVisit();
                Step = 0; NextStep = Now + 0.2; return false;
            case 53:
                Key(EKeys::E, IE_Pressed);
                break;
            case 54:
                Key(EKeys::E, IE_Released);
                AimTarget.Reset();
                Test->TestTrue(TEXT("E turns the sign to closed"), World->GetGameState<AShopGameState>()->GetDayPhase() != EShopDayPhase::Trading);
                Step = 44; NextStep = Now + 0.2; return false;
            case 40:
                Key(EKeys::R, IE_Pressed);
                break;
            case 41:
                Key(EKeys::R, IE_Released);
                ++SupplyChoiceCycles;
                if (SupplyChoiceCycles < 3)
                {
                    Step = 40;
                }
                else
                {
                    Test->TestEqual(TEXT("R cycles through individual packs back to the bundle"), Crate->PriceBits, 12);
                    Test->TestEqual(TEXT("Selected bundle has six Moonleaf"), Crate->Delivery.FindRef(TEXT("Moonleaf")), 6);
                    Test->TestEqual(TEXT("Selected bundle has three water"), Crate->Delivery.FindRef(TEXT("Spring Water")), 3);
                    Step = 29;
                }
                NextStep = Now + 0.2;
                return false;
            }

            ++Step;
            NextStep = Now + 0.2;
            return false;
        }

    private:
        FAutomationTestBase* Test;
        double Started = FPlatformTime::Seconds();
        double NextStep = 0.0;
        double NextCustomerDiagnostic = 0.0;
        int32 Step = 0;
        TWeakObjectPtr<AShopPickupItem> Item;
        TWeakObjectPtr<AShopBrewingStation> Station;
        TWeakObjectPtr<AShopCustomer> Customer;
        TWeakObjectPtr<AShopTill> Till;
        TWeakObjectPtr<AShopCustomerFlow> Flow;
        TWeakObjectPtr<AShopSupplyCrate> Crate;
        TWeakObjectPtr<UGameViewportClient> Viewport;
        TWeakObjectPtr<AActor> AimTarget;
        TArray<TWeakObjectPtr<AShopShelfSlot>> Slots;
        FVector MovementStart = FVector::ZeroVector;
        FVector RecipeSelectionPosition = FVector::ZeroVector;
        FRotator LookStart = FRotator::ZeroRotator;
        FRotator CameraStart = FRotator::ZeroRotator;
        float AimHeight = 12.0f;
        int64 CashBeforeSale = 0;
        int32 SalePrice = 0;
        int32 Visits = 0;
        int32 MaxCustomersObserved = 0;
        int32 SupplyChoiceCycles = 0;
        int32 LeavesBeforeBrew = 0;
        int32 WaterBeforeBrew = 0;
        int32 LeavesPerBrew = 0;
        int32 WaterPerBrew = 0;
        int64 CashBeforeSupplies = 0;
        bool bObservedWalkAnimation = false;
        bool bObservedFootMotion = false;
        bool bHaveFootSample = false;
        bool bOriginalIgnoreInput = false;
        bool bCapturedSale = false;
        FVector PreviousFoot = FVector::ZeroVector;
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShopMapInputTest, "FantasyShopkeeper.Interaction.ShopMapKeyboardWorkflow",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShopMapInputTest::RunTest(const FString& Parameters)
{
    ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(TEXT("/Game/Levels/L_Shop")));
    ADD_LATENT_AUTOMATION_COMMAND(FWaitForShadersToFinishCompiling());
    ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
    ADD_LATENT_AUTOMATION_COMMAND(FWaitForShadersToFinishCompiling());
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<FShopInputWorkflow>(this));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}

#endif
