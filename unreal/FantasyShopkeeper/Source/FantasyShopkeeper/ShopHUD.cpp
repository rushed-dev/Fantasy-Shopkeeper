#include "ShopHUD.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "ShopInteractionComponent.h"
#include "ShopPickupItem.h"
#include "ShopPlayerController.h"
#include "ShopShelfSlot.h"
#include "ShopBrewingStation.h"
#include "ShopCustomer.h"
#include "ShopGameState.h"
#include "ShopTill.h"
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

    const float CenterX = Canvas->ClipX * 0.5f;
    const float CenterY = Canvas->ClipY * 0.5f;
    if (!Controller->GetTillWidget())
    {
        DrawRect(FLinearColor::White, CenterX - 1.5f, CenterY - 1.5f, 3.0f, 3.0f);
    }

    if (const AShopGameState* State = GetWorld()->GetGameState<AShopGameState>())
    {
        const FString CashText = FText::Format(NSLOCTEXT("Shop", "CashDisplay", "Cash {0} bits"), FText::AsNumber(State->GetCash())).ToString();
        DrawRect(FLinearColor(0.02f, 0.02f, 0.02f, 0.85f), 12, 12, 220, 36);
        DrawText(CashText, FLinearColor::White, 22, 20, GEngine->GetMediumFont());
    }
    for (TActorIterator<AShopCustomerFlow> It(GetWorld()); It; ++It)
    {
        const FString Status = It->GetStatusText().ToString();
        float Width = 0;
        float Height = 0;
        GetTextSize(Status, Width, Height, GEngine->GetMediumFont());
        DrawRect(FLinearColor(0.02f, 0.02f, 0.02f, 0.85f), 12, 50, Width + 20, Height + 12);
        DrawText(Status, FLinearColor::White, 22, 56, GEngine->GetMediumFont());
        break;
    }

    if (Controller->GetTillWidget())
    {
        return;
    }
    const UShopInteractionComponent* Interaction = Controller->ShopInteraction;
    AActor* Target = Interaction->GetInteractionTarget();
    FString Prompt;
    if (const AShopShelfSlot* Slot = Cast<AShopShelfSlot>(Target))
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
        DrawRect(FLinearColor(0.02f, 0.02f, 0.02f, 0.85f), CenterX - Width * 0.5f - 10.0f, CenterY + 22.0f, Width + 20.0f, Height + 12.0f);
        DrawText(Prompt, FLinearColor::White, CenterX - Width * 0.5f, CenterY + 28.0f, GEngine->GetMediumFont());
    }
}
