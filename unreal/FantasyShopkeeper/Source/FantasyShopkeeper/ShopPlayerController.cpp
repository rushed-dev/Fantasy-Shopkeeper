#include "ShopPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "ShopInteractionComponent.h"
#include "ShopTill.h"
#include "ShopTillControl.h"
#include "InputKeyEventArgs.h"
#include "ShopBrewingStation.h"
#include "ShopSupplyCrate.h"
#include "ShopOrderingPage.h"
#include "ShopMenu.h"
#include "Misc/ConfigCacheIni.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "AudioDevice.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Engine/GameViewportClient.h"
#include "ShopGameState.h"
#include "ShopCustomerFlow.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/PawnMovementComponent.h"
#include "UObject/ConstructorHelpers.h"

AShopPlayerController::AShopPlayerController()
{
    ShopInteraction = CreateDefaultSubobject<UShopInteractionComponent>(TEXT("ShopInteraction"));
    static ConstructorHelpers::FObjectFinder<UInputMappingContext> MovementContext(TEXT("/Game/Input/IMC_Default.IMC_Default"));
    if (MovementContext.Succeeded())
    {
        MovementMappingContext = MovementContext.Object;
    }
    static ConstructorHelpers::FObjectFinder<UInputMappingContext> MouseLookContext(TEXT("/Game/Input/IMC_MouseLook.IMC_MouseLook"));
    if (MouseLookContext.Succeeded())
    {
        MouseLookMappingContext = MouseLookContext.Object;
    }
}

void AShopPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent);
    UEnhancedInputLocalPlayerSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
    if (!ensure(EnhancedInput && InputSubsystem))
    {
        return;
    }

    if (MovementMappingContext)
    {
        InputSubsystem->AddMappingContext(MovementMappingContext, 0);
    }
    if (MouseLookMappingContext)
    {
        InputSubsystem->AddMappingContext(MouseLookMappingContext, 0);
    }

    InteractAction = NewObject<UInputAction>(this, TEXT("NativeInteractAction"));
    InteractAction->ValueType = EInputActionValueType::Boolean;
    InteractAction->bConsumeInput = true;
    InteractionMappingContext = NewObject<UInputMappingContext>(this, TEXT("NativeInteractionContext"));
    InteractionMappingContext->MapKey(InteractAction, EKeys::E);
    NextRecipeAction = NewObject<UInputAction>(this, TEXT("NativeNextRecipeAction"));
    NextRecipeAction->ValueType = EInputActionValueType::Boolean;
    NextRecipeAction->bConsumeInput = true;
    InteractionMappingContext->MapKey(NextRecipeAction, EKeys::R);
    SetDownAction = NewObject<UInputAction>(this, TEXT("NativeSetDownAction"));
    SetDownAction->ValueType = EInputActionValueType::Boolean;
    SetDownAction->bConsumeInput = true;
    InteractionMappingContext->MapKey(SetDownAction, EKeys::G);
    // Consume E ahead of the prototype's Blueprint interaction mapping.
    InputSubsystem->AddMappingContext(InteractionMappingContext, 100);
    EnhancedInput->BindAction(InteractAction, ETriggerEvent::Started, this, &AShopPlayerController::HandleInteract);
    EnhancedInput->BindAction(NextRecipeAction, ETriggerEvent::Started, this, &AShopPlayerController::HandleNextRecipe);
    EnhancedInput->BindAction(SetDownAction, ETriggerEvent::Started, this, &AShopPlayerController::HandleSetDown);
}

void AShopPlayerController::HandleSetDown()
{
    if (!bRestartPromptOpen && !bRestPromptOpen && !IsOrderingPageOpen() && !IsShopMenuOpen()) ShopInteraction->SetDownHeldItem();
}

void AShopPlayerController::HandleNextRecipe()
{
    if (bRestartPromptOpen || bRestPromptOpen || IsOrderingPageOpen() || IsShopMenuOpen()) return;
    if (AShopBrewingStation* Station = Cast<AShopBrewingStation>(ShopInteraction->GetInteractionTarget()))
    {
        Station->CycleRecipe(ShopInteraction);
    }
    else if (AShopSupplyCrate* Crate = Cast<AShopSupplyCrate>(ShopInteraction->GetInteractionTarget()))
    {
        Crate->CycleOffer(ShopInteraction);
    }
}

void AShopPlayerController::HandleInteract()
{
    if (bRestartPromptOpen || bRestPromptOpen || IsOrderingPageOpen() || IsShopMenuOpen()) return;
    if (AShopSupplyCrate* Desk = Cast<AShopSupplyCrate>(ShopInteraction->GetInteractionTarget())) OpenOrderingPage(Desk);
    else ShopInteraction->Interact();
}

bool AShopPlayerController::InputKey(const FInputKeyEventArgs& Params)
{
    if (IsShopMenuOpen()) return true; // Focused Slate menu handles its own keys while the world is paused.
    if (const AShopGameState* State = GetWorld()->GetGameState<AShopGameState>(); State && State->IsOvernight() && Params.Event != IE_Released) return true;
    if (Params.Event == IE_Pressed && Params.Key == EKeys::F1) return OpenShopMenu();
    if (Params.Event == IE_Axis && (Params.Key == EKeys::MouseX || Params.Key == EKeys::MouseY))
    {
        FInputKeyEventArgs Scaled = Params; Scaled.AmountDepressed *= LookSensitivity;
        return Super::InputKey(Scaled);
    }
    if (bRestPromptOpen)
    {
        if (Params.Event == IE_Pressed && Params.Key == EKeys::F10) SetRestPrompt(false);
        else if (Params.Event == IE_Pressed && Params.Key == EKeys::Enter)
        {
            if (AShopGameState* State = GetWorld()->GetGameState<AShopGameState>()) State->BeginOvernight(true);
            SetRestPrompt(false);
        }
        return Params.Event == IE_Released ? Super::InputKey(Params) : true;
    }
    if (IsOrderingPageOpen())
    {
        if (Params.Event == IE_Pressed) HandleOrderingKey(Params.Key);
        return Params.Event == IE_Released ? Super::InputKey(Params) : true;
    }
    if (Params.Event == IE_Pressed && Params.Key == EKeys::F10)
    {
        SetRestartPrompt(!bRestartPromptOpen);
        return true;
    }
    if (bRestartPromptOpen)
    {
        if (Params.Event == IE_Pressed && Params.Key == EKeys::F)
        {
            bFreshGameConfirmation = true;
            return true;
        }
        if (Params.Event == IE_Pressed && Params.Key == EKeys::Enter)
        {
            if (AShopGameState* State = GetWorld()->GetGameState<AShopGameState>())
            {
                if (bFreshGameConfirmation) State->StartFreshGame();
                else State->RestoreMorning();
            }
            SetRestartPrompt(false);
        }
        // Releases still reach Enhanced Input so held controls cannot stick.
        return Params.Event == IE_Released ? Super::InputKey(Params) : true;
    }
    if (Params.Event == IE_Pressed)
    {
        if (AShopGameState* State = GetWorld()->GetGameState<AShopGameState>())
        {
            if (Params.Key == EKeys::X)
            {
                for (TActorIterator<AShopCustomerFlow> It(GetWorld()); It; ++It) It->EndUnservedVisit();
                return true;
            }
            if (Params.Key == EKeys::N)
            {
                RequestRest();
                return true;
            }
        }
    }
    AActor* Target = ShopInteraction->GetInteractionTarget();
    if (AShopSupplyCrate* Crate = Cast<AShopSupplyCrate>(Target); Crate && Params.Event == IE_Pressed)
    {
        if (Params.Key == EKeys::F || Params.Key == EKeys::T)
        {
            Crate->ChangeQuantity(ShopInteraction, Params.Key == EKeys::T ? 1 : -1);
            return true;
        }
    }
    AShopTill* Till = Cast<AShopTill>(Target);
    if (AShopTillControl* Control = Cast<AShopTillControl>(Target))
    {
        Till = Control->GetTill();
    }
    if (IsValid(Till) && !ShopInteraction->GetHeldItem() && Params.Event == IE_Pressed)
    {
        const FKey Digits[] = { EKeys::Zero, EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four,
            EKeys::Five, EKeys::Six, EKeys::Seven, EKeys::Eight, EKeys::Nine };
        const FKey NumPad[] = { EKeys::NumPadZero, EKeys::NumPadOne, EKeys::NumPadTwo, EKeys::NumPadThree,
            EKeys::NumPadFour, EKeys::NumPadFive, EKeys::NumPadSix, EKeys::NumPadSeven, EKeys::NumPadEight, EKeys::NumPadNine };
        for (int32 Digit = 0; Digit <= 9; ++Digit)
        {
            if (Params.Key == Digits[Digit] || Params.Key == NumPad[Digit])
            {
                if (Till->GetPhase() == EShopTillPhase::MakingChange && Digit >= 1 && Digit <= 3)
                {
                    const int32 Denominations[] = {1, 10, 100};
                    const bool bRemove = IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift);
                    Till->UseControl(ShopInteraction, bRemove ? EShopTillControl::RemoveCoin : EShopTillControl::AddCoin, Denominations[Digit - 1]);
                }
                return true;
            }
        }
    }
    return Super::InputKey(Params);
}

void AShopPlayerController::OnUnPossess()
{
    CloseShopMenu();
    CloseOrderingPage();
    ShopInteraction->ReleaseHeldItem();
    Super::OnUnPossess();
}

void AShopPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    CloseShopMenu();
    CloseOrderingPage();
    if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
    {
        if (InteractionMappingContext)
        {
            InputSubsystem->RemoveMappingContext(InteractionMappingContext);
        }
        if (MouseLookMappingContext)
        {
            InputSubsystem->RemoveMappingContext(MouseLookMappingContext);
        }
    }
    Super::EndPlay(EndPlayReason);
}

void AShopPlayerController::SetRestartPrompt(bool bOpen)
{
    if (bRestartPromptOpen == bOpen) return;
    bRestartPromptOpen = bOpen;
    bFreshGameConfirmation = false;
    FlushPressedKeys();
    SetIgnoreMoveInput(bOpen);
    SetIgnoreLookInput(bOpen);
    if (APawn* ControlledPawn = GetPawn())
        if (UPawnMovementComponent* Movement = ControlledPawn->GetMovementComponent()) Movement->StopMovementImmediately();
}

bool AShopPlayerController::OpenOrderingPage(AShopSupplyCrate* Desk)
{
    const AShopGameState* State = GetWorld()->GetGameState<AShopGameState>();
    if (!IsLocalController() || !GetWorld()->GetGameViewport() || bRestartPromptOpen || bRestPromptOpen || IsOrderingPageOpen() || IsShopMenuOpen()
        || !State || !State->IsDayInitialized() || State->IsOvernight() || !IsValid(Desk)
        || ShopInteraction->GetHeldItem() || !ShopInteraction->CanReach(Desk)) return false;
    OrderingDesk = Desk;
    OrderingWidget = SNew(SShopOrderingPage).Controller(this).Desk(Desk);
    GetWorld()->GetGameViewport()->AddViewportWidgetContent(OrderingWidget.ToSharedRef(), 20);
    FlushPressedKeys();
    SetIgnoreMoveInput(true);
    SetIgnoreLookInput(true);
    if (APawn* ControlledPawn = GetPawn())
        if (UPawnMovementComponent* Movement = ControlledPawn->GetMovementComponent()) Movement->StopMovementImmediately();
    bShowMouseCursor = true;
    FInputModeGameAndUI Mode;
    Mode.SetWidgetToFocus(OrderingWidget);
    Mode.SetHideCursorDuringCapture(false);
    SetInputMode(Mode);
    return true;
}

void AShopPlayerController::CloseOrderingPage()
{
    if (!IsOrderingPageOpen()) return;
    if (UGameViewportClient* Viewport = GetWorld()->GetGameViewport()) Viewport->RemoveViewportWidgetContent(OrderingWidget.ToSharedRef());
    OrderingWidget.Reset();
    OrderingDesk.Reset();
    FlushPressedKeys();
    SetIgnoreMoveInput(false);
    SetIgnoreLookInput(false);
    bShowMouseCursor = false;
    SetInputMode(FInputModeGameOnly());
}

bool AShopPlayerController::HandleOrderingKey(FKey Key)
{
    if (!IsOrderingPageOpen()) return false;
    if (Key == EKeys::F10) { CloseOrderingPage(); return true; }
    AShopSupplyCrate* Desk = OrderingDesk.Get();
    if (!Desk || ShopInteraction->GetHeldItem() || !ShopInteraction->CanReach(Desk)) { CloseOrderingPage(); return true; }
    if (Key == EKeys::R) Desk->CycleOffer(ShopInteraction);
    else if (Key == EKeys::F || Key == EKeys::T) Desk->ChangeQuantity(ShopInteraction, Key == EKeys::T ? 1 : -1);
    else if (Key == EKeys::Enter) Desk->Interact_Implementation(ShopInteraction);
    return true;
}

void AShopPlayerController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);
    if (!bSettingsLoaded)
    {
        bSettingsLoaded = true;
        if (!GIsAutomationTesting)
        {
            GConfig->GetFloat(TEXT("ShopPreferences"), TEXT("LookSensitivity"), LookSensitivity, GGameUserSettingsIni);
            GConfig->GetFloat(TEXT("ShopPreferences"), TEXT("MasterVolume"), MasterVolume, GGameUserSettingsIni);
            SetLookSensitivity(LookSensitivity); SetMasterVolume(MasterVolume);
        }
    }
    if (!GIsAutomationTesting && !FParse::Param(FCommandLine::Get(), TEXT("ShopSkipWelcome")))
    {
        const AShopGameState* State = GetWorld()->GetGameState<AShopGameState>();
        if (State && State->GetDay() < 3) bChapterShown = false;
        if (State && State->IsDayInitialized() && !bWelcomeShown) bWelcomeShown = OpenShopMenu();
        if (State && State->GetDay() == 3 && State->GetDayPhase() == EShopDayPhase::Evening && !bChapterShown && !IsShopMenuOpen())
            bChapterShown = OpenShopMenu(TEXT("Chapter"));
    }
    if (IsOrderingPageOpen())
    {
        const AShopGameState* State = GetWorld()->GetGameState<AShopGameState>();
        if (!OrderingDesk.IsValid() || !State || State->IsOvernight() || ShopInteraction->GetHeldItem()
            || !ShopInteraction->CanReach(OrderingDesk.Get())) CloseOrderingPage();
    }
}

bool AShopPlayerController::RequestRest()
{
    AShopGameState* State = GetWorld()->GetGameState<AShopGameState>();
    if (!State || bRestPromptOpen || bRestartPromptOpen || IsOrderingPageOpen() || IsShopMenuOpen() || ShopInteraction->GetHeldItem()) return false;
    if (State->BeginOvernight()) return true;
    if (State->CanOrderSupplies() && State->HasConfirmedSupplies() && !State->GetRestWarning().IsEmpty())
    {
        SetRestPrompt(true);
        return true;
    }
    return false;
}

FString AShopPlayerController::GetRestWarningText() const
{
    const AShopGameState* State = GetWorld()->GetGameState<AShopGameState>();
    return State ? State->GetRestWarning() : FString();
}

void AShopPlayerController::SetRestPrompt(bool bOpen)
{
    if (bRestPromptOpen == bOpen) return;
    bRestPromptOpen = bOpen;
    FlushPressedKeys();
    SetIgnoreMoveInput(bOpen);
    SetIgnoreLookInput(bOpen);
    if (APawn* ControlledPawn = GetPawn())
        if (UPawnMovementComponent* Movement = ControlledPawn->GetMovementComponent()) Movement->StopMovementImmediately();
}


bool AShopPlayerController::OpenShopMenu(FName Page)
{
    const AShopGameState* State = GetWorld()->GetGameState<AShopGameState>();
    if (!IsLocalController() || !GetWorld()->GetGameViewport() || !State || !State->IsDayInitialized()
        || State->IsOvernight() || bRestPromptOpen || bRestartPromptOpen) return false;
    CloseOrderingPage();
    if (MenuWidget.IsValid()) GetWorld()->GetGameViewport()->RemoveViewportWidgetContent(MenuWidget.ToSharedRef());
    else
    {
        SetPause(true); FlushPressedKeys(); SetIgnoreMoveInput(true); SetIgnoreLookInput(true);
        if (APawn* ControlledPawn = GetPawn()) if (UPawnMovementComponent* Move = ControlledPawn->GetMovementComponent()) Move->StopMovementImmediately();
    }
    MenuPage = Page;
    MenuWidget = SNew(SShopMenu).Controller(this);
    GetWorld()->GetGameViewport()->AddViewportWidgetContent(MenuWidget.ToSharedRef(), 30);
    bShowMouseCursor = true;
    FInputModeGameAndUI Mode; Mode.SetWidgetToFocus(MenuWidget); Mode.SetHideCursorDuringCapture(false); SetInputMode(Mode);
    return true;
}
void AShopPlayerController::CloseShopMenu()
{
    if (!MenuWidget.IsValid()) return;
    if (UGameViewportClient* Viewport = GetWorld()->GetGameViewport()) Viewport->RemoveViewportWidgetContent(MenuWidget.ToSharedRef());
    MenuWidget.Reset(); SetPause(false); FlushPressedKeys(); SetIgnoreMoveInput(false); SetIgnoreLookInput(false);
    bShowMouseCursor = false; SetInputMode(FInputModeGameOnly());
}
void AShopPlayerController::MenuAction(FName Action)
{
    if (!IsShopMenuOpen()) return;
    AShopGameState* State = GetWorld()->GetGameState<AShopGameState>();
    if (!State) return;
    if (Action == TEXT("Resume")) { CloseShopMenu(); return; }
    if (Action == TEXT("ConfirmRestart") && MenuPage == TEXT("Restart")) { State->RestoreMorning(); CloseShopMenu(); return; }
    if (Action == TEXT("ConfirmFresh") && MenuPage == TEXT("Fresh")) { State->StartFreshGame(); bChapterShown = false; CloseShopMenu(); return; }
    if (Action == TEXT("ConfirmQuit") && MenuPage == TEXT("Quit")) { UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false); return; }
    if (MenuPage == TEXT("Journal"))
    {
        if (Action == TEXT("AcceptStory") || Action == TEXT("DeclineStory")) { State->ChooseLanternStory(Action == TEXT("AcceptStory")); OpenShopMenu(TEXT("Journal")); return; }
        if (Action == TEXT("Credit") && CanUseSupplierDesk()) { State->UseSupplierCredit(); OpenShopMenu(TEXT("Journal")); return; }
        if (Action == TEXT("Repay") && CanUseSupplierDesk()) { State->RepaySupplierCredit(); OpenShopMenu(TEXT("Journal")); return; }
    }
    const TSet<FName> Pages = {TEXT("Main"), TEXT("Help"), TEXT("Journal"), TEXT("Settings"), TEXT("Chapter"), TEXT("Credits"), TEXT("Restart"), TEXT("Fresh"), TEXT("Quit")};
    if (Pages.Contains(Action)) OpenShopMenu(Action);
}
FString AShopPlayerController::GetMenuText() const
{
    const AShopGameState* State = GetWorld()->GetGameState<AShopGameState>();
    if (!State) return FString();
    if (MenuPage == TEXT("Journal")) return State->GetJournalText();
    if (MenuPage == TEXT("Chapter")) return State->GetChapterText();
    if (MenuPage == TEXT("Restart")) return TEXT("Restart this morning? Today's brewing, sales, orders and story choices will roll back together. Your saved morning stays intact.");
    if (MenuPage == TEXT("Fresh")) return TEXT("Start a new shop on day 1? Progress resets, with a backup of your previous morning checkpoint.");
    if (MenuPage == TEXT("Quit")) return TEXT("Quit to desktop? Only morning checkpoints are saved. Today's work is not saved on exit. Rest at the supplier book to checkpoint the next morning, or quit and return to the last saved morning.");
    if (MenuPage == TEXT("Credits")) return TEXT("Fantasy Shopkeeper / The Lantern Walk\nDesign and development: the Fantasy Shopkeeper project, with OpenAI Codex assistance.\nOriginal procedural shop models: Blender. Procedural audio and native gameplay: project sources. Customer mannequins, animations and first-person template: Epic Games / Unreal Engine.\nSee the packaged NOTICES.txt for engine and third-party notices.\n\nThank you for keeping the lanterns lit.");
    if (MenuPage == TEXT("Settings")) return TEXT("Adjust at any time. Settings persist independently of morning checkpoints. F1 returns to the shop; F10 also closes this menu.");
    if (MenuPage == TEXT("Help")) return TEXT("WASD move / Mouse look / E interact / G set down / F1 pause and help\n\nPREPARE: Brew at the brass vessel with E. R changes recipe. Dawn Tonic costs 2 Moonleaf + 1 Water; Moonlit Ink costs 1 + 2. Pick up finished bottles and E on a shelf to stock them. Three visitors need 2 Tonics and 1 Ink.\n\nTRADE: E on the door sign opens. Shoppers collect bottles and queue. Read the price and payment on the counter. E on coin stacks adds change; E on the mat lanes removes it. Stamp PAID when exact. There is no time limit. X sends the front visitor home before pickup if you cannot serve them.\n\nEVENING: Close with the sign or serve all three. E on the supplier book opens tomorrow's orders. Change pack / quantity, then Sign & pay. Rest using its REST tab; unpack the parcel by the entrance next morning. Supplies never arrive the same day.\n\nPROGRESS: Buy drawers for 24 bits and the batch kit for 36 in the evening. Batch brewing costs ingredients per bottle; use singles for mixed orders. Requests are optional. Supplier credit is available in the book's account page when short of funds.\n\nSAVING: Rest creates the next morning checkpoint. Quitting or restarting rolls the whole current day back. F10 offers restart/new-game confirmation. No mid-day saving.");
    return FString::Printf(TEXT("A small shop. Three days. A neighbourhood finding its light.\n\nDay %d / %lld bits in your purse\nThe world is paused. Continue resumes the current session. Reopening the game restores the saved morning; today's work is saved only when you rest into the next morning.\n\nF1 opens this menu at any time."), State->GetDay(), State->GetCash());
}
void AShopPlayerController::SetLookSensitivity(float Value)
{
    LookSensitivity = FMath::IsFinite(Value) ? FMath::Clamp(Value, 0.25f, 2.f) : 1.f;
    if (!GIsAutomationTesting) { GConfig->SetFloat(TEXT("ShopPreferences"), TEXT("LookSensitivity"), LookSensitivity, GGameUserSettingsIni); GConfig->Flush(false, GGameUserSettingsIni); }
}
void AShopPlayerController::SetMasterVolume(float Value)
{
    MasterVolume = FMath::IsFinite(Value) ? FMath::Clamp(Value, 0.f, 1.f) : 0.7f;
    if (FAudioDeviceHandle Audio = GetWorld()->GetAudioDevice()) Audio->SetTransientPrimaryVolume(MasterVolume);
    if (!GIsAutomationTesting) { GConfig->SetFloat(TEXT("ShopPreferences"), TEXT("MasterVolume"), MasterVolume, GGameUserSettingsIni); GConfig->Flush(false, GGameUserSettingsIni); }
}
FString AShopPlayerController::GetGuideText() const
{
    const AShopGameState* State = GetWorld()->GetGameState<AShopGameState>();
    if (!State || State->GetDay() > 3) return FString();
    if (State->HasDelivery()) return TEXT("Morning: E on the sealed parcel by the entrance to unpack supplies.");
    if (State->GetDayPhase() == EShopDayPhase::Preparation) return TEXT("Prepare 2 Dawn Tonics + 1 Moonlit Ink. E brew / R recipe. Stock shelves, then E on the door sign.");
    if (State->GetDayPhase() == EShopDayPhase::Trading) return TEXT("Read price and tender on the desk. Count change by hand, then E on PAID. No rush. F1: help.");
    if (State->GetDayPhase() == EShopDayPhase::Closing) return TEXT("Finish everyone inside. X lets the front customer leave before pickup if you cannot serve them.");
    return TEXT("E on the supplier book: plan and pay for tomorrow. REST advances; only mornings are saved.");
}

bool AShopPlayerController::CanUseSupplierDesk() const
{
    if (!ShopInteraction || ShopInteraction->GetHeldItem()) return false;
    for (TActorIterator<AShopSupplyCrate> Desk(GetWorld()); Desk; ++Desk) if (ShopInteraction->CanReach(*Desk)) return true;
    return false;
}
