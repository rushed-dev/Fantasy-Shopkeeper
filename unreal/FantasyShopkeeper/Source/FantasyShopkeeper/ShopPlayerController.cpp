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
#include "ShopGameState.h"
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
    ShopInteraction->SetDownHeldItem();
}

void AShopPlayerController::HandleNextRecipe()
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

void AShopPlayerController::HandleInteract()
{
    ShopInteraction->Interact();
}

bool AShopPlayerController::InputKey(const FInputKeyEventArgs& Params)
{
    if (Params.Event == IE_Pressed)
    {
        if (AShopGameState* State = GetWorld()->GetGameState<AShopGameState>())
        {
            if (Params.Key == EKeys::O)
            {
                if (State->GetDayPhase() == EShopDayPhase::Preparation) State->OpenShop();
                else State->CloseShop();
                return true;
            }
            if (Params.Key == EKeys::N)
            {
                State->AdvanceMorning();
                return true;
            }
        }
    }
    AActor* Target = ShopInteraction->GetInteractionTarget();
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
                else
                {
                    Till->UseControl(ShopInteraction, EShopTillControl::Digit, Digit);
                }
                return true;
            }
        }
        if (Params.Key == EKeys::Delete || Params.Key == EKeys::BackSpace)
        {
            Till->UseControl(ShopInteraction, EShopTillControl::Clear);
            return true;
        }
        if (Params.Key == EKeys::Enter)
        {
            Till->UseControl(ShopInteraction, EShopTillControl::AcceptPayment);
            return true;
        }
    }
    return Super::InputKey(Params);
}

void AShopPlayerController::OnUnPossess()
{
    ShopInteraction->ReleaseHeldItem();
    Super::OnUnPossess();
}

void AShopPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
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
