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
    if (!bRestartPromptOpen && !IsOrderingPageOpen()) ShopInteraction->SetDownHeldItem();
}

void AShopPlayerController::HandleNextRecipe()
{
    if (bRestartPromptOpen || IsOrderingPageOpen()) return;
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
    if (bRestartPromptOpen || IsOrderingPageOpen()) return;
    if (AShopSupplyCrate* Desk = Cast<AShopSupplyCrate>(ShopInteraction->GetInteractionTarget())) OpenOrderingPage(Desk);
    else ShopInteraction->Interact();
}

bool AShopPlayerController::InputKey(const FInputKeyEventArgs& Params)
{
    if (const AShopGameState* State = GetWorld()->GetGameState<AShopGameState>(); State && State->IsOvernight() && Params.Event != IE_Released) return true;
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
                State->BeginOvernight();
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
    CloseOrderingPage();
    ShopInteraction->ReleaseHeldItem();
    Super::OnUnPossess();
}

void AShopPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
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
    if (!IsLocalController() || !GetWorld()->GetGameViewport() || bRestartPromptOpen || IsOrderingPageOpen()
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
    if (IsOrderingPageOpen())
    {
        const AShopGameState* State = GetWorld()->GetGameState<AShopGameState>();
        if (!OrderingDesk.IsValid() || !State || State->IsOvernight() || ShopInteraction->GetHeldItem()
            || !ShopInteraction->CanReach(OrderingDesk.Get())) CloseOrderingPage();
    }
}
