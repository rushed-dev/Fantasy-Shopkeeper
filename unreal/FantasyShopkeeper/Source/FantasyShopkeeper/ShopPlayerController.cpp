#include "ShopPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "ShopInteractionComponent.h"
#include "ShopTill.h"
#include "ShopTillWidget.h"
#include "ShopBrewingStation.h"
#include "ShopSupplyCrate.h"
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
    if (!TillWidget)
    {
        ShopInteraction->SetDownHeldItem();
    }
}

void AShopPlayerController::HandleNextRecipe()
{
    if (!TillWidget)
    {
        if (AShopBrewingStation* Station = Cast<AShopBrewingStation>(ShopInteraction->GetInteractionTarget()))
        {
            Station->CycleRecipe(ShopInteraction);
        }
        else if (AShopSupplyCrate* Crate = Cast<AShopSupplyCrate>(ShopInteraction->GetInteractionTarget()))
        {
            Crate->CycleOffer(ShopInteraction);
        }
    }
}

void AShopPlayerController::HandleInteract()
{
    if (!TillWidget)
    {
        ShopInteraction->Interact();
    }
}

bool AShopPlayerController::OpenTill(AShopTill* Till)
{
    if (TillWidget || !IsValid(Till) || !IsLocalController())
    {
        return false;
    }
    TillWidget = CreateWidget<UShopTillWidget>(this, UShopTillWidget::StaticClass());
    if (!TillWidget)
    {
        return false;
    }
    TillWidget->AddToViewport(100);
    TillWidget->SetTill(Till);
    FInputModeUIOnly Mode;
    Mode.SetWidgetToFocus(TillWidget->TakeWidget());
    Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    SetInputMode(Mode);
    bShowMouseCursor = true;
    SetIgnoreMoveInput(true);
    SetIgnoreLookInput(true);
    FlushPressedKeys();
    if (GetPawn() && GetPawn()->GetMovementComponent())
    {
        GetPawn()->GetMovementComponent()->StopMovementImmediately();
    }
    TillWidget->FocusInput();
    return true;
}

void AShopPlayerController::CloseTill()
{
    if (!TillWidget)
    {
        return;
    }
    TillWidget->RemoveFromParent();
    TillWidget = nullptr;
    SetIgnoreMoveInput(false);
    SetIgnoreLookInput(false);
    bShowMouseCursor = false;
    SetInputMode(FInputModeGameOnly());
    FlushPressedKeys();
}

void AShopPlayerController::OnUnPossess()
{
    CloseTill();
    ShopInteraction->ReleaseHeldItem();
    Super::OnUnPossess();
}

void AShopPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    CloseTill();
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
