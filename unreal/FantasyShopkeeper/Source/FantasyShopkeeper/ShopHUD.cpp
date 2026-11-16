#include "ShopHUD.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "ShopInteractionComponent.h"
#include "ShopPickupItem.h"
#include "ShopPlayerController.h"
#include "ShopShelfSlot.h"
#include "ShopShelfSection.h"
#include "ShopBrewingStation.h"
#include "ShopCustomer.h"
#include "ShopGameState.h"
#include "ShopTill.h"
#include "ShopDoorSign.h"
#include "ShopSupplyControl.h"
#include "ShopStorageUpgrade.h"
#include "ShopBrewingUpgrade.h"
#include "ShopTillControl.h"
#include "ShopSupplyCrate.h"
#include "ShopCustomerFlow.h"
#include "EngineUtils.h"
#include "Engine/World.h"

void AShopHUD::DrawHUD()
{
    Super::DrawHUD();
    const AShopPlayerController* Controller = Cast<AShopPlayerController>(PlayerOwner);
    if (!Canvas || !Controller || !Controller->GetPawn())
    {
        return;
    }

    if (const AShopGameState* State = GetWorld()->GetGameState<AShopGameState>(); State && State->IsOvernight()) return;

    const float CenterX = Canvas->ClipX * 0.5f;
    const float CenterY = Canvas->ClipY * 0.5f;
    if (Controller->IsRestartPromptOpen())
    {
        DrawRect(FLinearColor(0.02f, 0.02f, 0.02f, 0.94f), CenterX - 300, CenterY - 90, 600, 180);
        const bool bFresh = Controller->IsFreshGameConfirmation();
        DrawText(bFresh ? TEXT("Start a fresh shop on day 1?") : TEXT("Restart this morning?"), FLinearColor::White, CenterX - 275, CenterY - 65, GEngine->GetMediumFont());
        DrawText(bFresh ? TEXT("All progress resets to the starting shop and ingredients.") : TEXT("Today's sales, brewing, orders and upgrades will be undone."), FLinearColor(0.9f, 0.8f, 0.55f), CenterX - 275, CenterY - 25, GEngine->GetMediumFont());
        DrawText(bFresh ? TEXT("Your previous morning checkpoint will be backed up.") : TEXT("Cash, ingredients and bottles return to the saved morning."), FLinearColor::White, CenterX - 275, CenterY, GEngine->GetMediumFont());
        DrawText(bFresh ? TEXT("Enter  Start day 1                 Esc  Keep playing") : TEXT("Enter  Restart day     F  New game     Esc  Keep playing"), FLinearColor::White, CenterX - 275, CenterY + 45, GEngine->GetMediumFont());
        return;
    }
    DrawText(TEXT("Esc  Day / New game"), FLinearColor::White, Canvas->ClipX - 175, 20, GEngine->GetSmallFont());
    DrawRect(FLinearColor::White, CenterX - 1.5f, CenterY - 1.5f, 3.0f, 3.0f);

    if (const AShopGameState* State = GetWorld()->GetGameState<AShopGameState>())
    {
        const FString CashText = FText::Format(NSLOCTEXT("Shop", "CashDisplay", "Cash {0} bits"), FText::AsNumber(State->GetCash())).ToString();
        DrawRect(FLinearColor(0.02f, 0.02f, 0.02f, 0.85f), 12, 12, 220, 36);
        DrawText(CashText, FLinearColor::White, 22, 20, GEngine->GetMediumFont());
        if (State->IsDayInitialized())
        {
            const FString DayText = State->GetDayText();
            float Width = 0, Height = 0;
            GetTextSize(DayText, Width, Height, GEngine->GetMediumFont());
            DrawRect(FLinearColor(0.02f, 0.02f, 0.02f, 0.85f), 12, 86, Width + 20, Height + 12);
            DrawText(DayText, FLinearColor::White, 22, 92, GEngine->GetMediumFont());
            const FString Message = State->GetCheckpointMessage();
            if (!Message.IsEmpty())
            {
                GetTextSize(Message, Width, Height, GEngine->GetMediumFont());
                DrawRect(FLinearColor(0.02f, 0.02f, 0.02f, 0.85f), 12, 124, Width + 20, Height + 12);
                DrawText(Message, FLinearColor(0.9f, 0.8f, 0.55f), 22, 130, GEngine->GetMediumFont());
            }
        }
    }
    for (TActorIterator<AShopCustomerFlow> It(GetWorld()); It; ++It)
    {
        if (const AShopCustomer* Speaker = It->GetActiveCustomer(); Speaker && !Speaker->GetSpokenLine().IsEmpty()
            && FVector::DistSquared(Controller->GetPawn()->GetActorLocation(), Speaker->GetActorLocation()) < FMath::Square(900.f))
        {
            // Nearby subtitles imply speech without a floating world-space board or input capture.
            const FString Line = FString::Printf(TEXT("Customer: %s"), *Speaker->GetSpokenLine().ToString());
            float SpeechWidth = 0, SpeechHeight = 0;
            GetTextSize(Line, SpeechWidth, SpeechHeight, GEngine->GetMediumFont());
            const float Opacity = Speaker->GetSpeechOpacity();
            const float SpeechX = (Canvas->ClipX - SpeechWidth) * 0.5f;
            const float SpeechY = Canvas->ClipY - SpeechHeight - 100.f;
            DrawRect(FLinearColor(0.02f, 0.02f, 0.02f, 0.65f * Opacity), SpeechX - 12, SpeechY - 6, SpeechWidth + 24, SpeechHeight + 12);
            DrawText(Line, FLinearColor(0.96f, 0.89f, 0.72f, Opacity), SpeechX, SpeechY, GEngine->GetMediumFont());
        }
        const FString Status = It->GetStatusText().ToString();
        float Width = 0;
        float Height = 0;
        GetTextSize(Status, Width, Height, GEngine->GetMediumFont());
        DrawRect(FLinearColor(0.02f, 0.02f, 0.02f, 0.85f), 12, 50, Width + 20, Height + 12);
        DrawText(Status, FLinearColor::White, 22, 56, GEngine->GetMediumFont());
        break;
    }

    const UShopInteractionComponent* Interaction = Controller->ShopInteraction;
    AActor* Target = Interaction->GetInteractionTarget();
    FString Prompt;
    if (const AShopBrewingUpgrade* Kit = Cast<AShopBrewingUpgrade>(Target))
    {
        Prompt = Interaction->GetHeldItem() ? TEXT("Put down the held item to use the bottling kit") : Kit->GetInteractionPrompt().ToString();
    }
    else if (const AShopStorageUpgrade* Storage = Cast<AShopStorageUpgrade>(Target))
    {
        Prompt = Interaction->GetHeldItem() ? TEXT("Put down the held item to buy storage") : Storage->GetInteractionPrompt().ToString();
    }
    else if (const AShopSupplyControl* SupplyControl = Cast<AShopSupplyControl>(Target))
    {
        Prompt = Interaction->GetHeldItem() ? TEXT("Put down the held item first") : SupplyControl->GetInteractionPrompt().ToString();
    }
    else if (const AShopDoorSign* Sign = Cast<AShopDoorSign>(Target))
    {
        Prompt = Interaction->GetHeldItem() ? TEXT("Put down the held item to turn the sign") : Sign->GetInteractionPrompt().ToString();
    }
    else if (const AShopShelfSection* Section = Cast<AShopShelfSection>(Target))
    {
        Prompt = Section->GetInteractionPrompt(Controller->ShopInteraction).ToString();
    }
    else if (const AShopShelfSlot* Slot = Cast<AShopShelfSlot>(Target))
    {
        if (Interaction->GetHeldItem())
        {
            Prompt = Slot->GetOccupyingItem() ? TEXT("Slot occupied") : TEXT("E  Place item");
        }
        else if (Slot->GetOccupyingItem())
        {
            Prompt = FString::Printf(TEXT("E  Pick up %s"), *Slot->GetOccupyingItem()->DisplayName.ToString());
        }
    }
    else if (const AShopPickupItem* Item = Cast<AShopPickupItem>(Target))
    {
        Prompt = Interaction->GetHeldItem() ? TEXT("Hands full") : FString::Printf(TEXT("E  Pick up %s"), *Item->DisplayName.ToString());
    }
    else if (const AShopBrewingStation* Station = Cast<AShopBrewingStation>(Target))
    {
        Prompt = Station->GetInteractionPrompt(Controller->ShopInteraction).ToString();
    }
    else if (const AShopCustomer* Customer = Cast<AShopCustomer>(Target))
    {
        Prompt = Customer->GetInteractionPrompt().ToString();
    }
    else if (const AShopSupplyCrate* Crate = Cast<AShopSupplyCrate>(Target))
    {
        Prompt = Crate->GetInteractionPrompt(Controller->ShopInteraction).ToString();
    }
    else if (const AShopTillControl* Control = Cast<AShopTillControl>(Target))
    {
        Prompt = Control->GetInteractionPrompt().ToString();
    }
    else if (const AShopTill* Till = Cast<AShopTill>(Target))
    {
        Prompt = Interaction->GetHeldItem() ? TEXT("Put down the held item to use the till") : Till->GetInteractionPrompt().ToString();
    }

    if (Interaction->GetHeldItem())
    {
        if (!Prompt.IsEmpty())
        {
            Prompt += TEXT(" | ");
        }
        Prompt += TEXT("G  Set down on a clear floor or table");
    }
    if (!Prompt.IsEmpty())
    {
        float Width = 0.0f;
        float Height = 0.0f;
        GetTextSize(Prompt, Width, Height, GEngine->GetMediumFont());
        const bool bReadingSupplies = Target && (Target->IsA<AShopSupplyCrate>() || Target->IsA<AShopSupplyControl>() || Target->IsA<AShopStorageUpgrade>() || Target->IsA<AShopBrewingUpgrade>());
        const float PromptY = bReadingSupplies ? Canvas->ClipY - Height - 30.f : CenterY + 28.f;
        DrawRect(FLinearColor(0.02f, 0.02f, 0.02f, 0.85f), CenterX - Width * 0.5f - 10.0f, PromptY - 6.f, Width + 20.0f, Height + 12.0f);
        DrawText(Prompt, FLinearColor::White, CenterX - Width * 0.5f, PromptY, GEngine->GetMediumFont());
    }
}
