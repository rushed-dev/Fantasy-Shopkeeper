#include "ShopOrderingPage.h"
#include "ShopPlayerController.h"
#include "ShopSupplyCrate.h"
#include "ShopBrewingStation.h"
#include "ShopGameState.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"

void SShopOrderingPage::Construct(const FArguments& Args)
{
    Controller = Args._Controller;
    Desk = Args._Desk;
    const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Regular", 16);
    const FLinearColor Ink(0.12f, 0.09f, 0.06f);
    TSharedRef<SVerticalBox> Content = SNew(SVerticalBox);
    Content->AddSlot().AutoHeight().Padding(0, 0, 0, 12)
    [ SNew(STextBlock).Text(FText::FromString(TEXT("SUPPLIER  /  INGREDIENT ORDERS")))
        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 24)).ColorAndOpacity(Ink) ];
    Content->AddSlot().AutoHeight().Padding(0, 0, 0, 12)
    [ SNew(STextBlock).Font(Font).ColorAndOpacity(Ink).Text_Lambda([this]()
        {
            if (!Desk.IsValid()) return FText::GetEmpty();
            const AShopGameState* State = Desk->GetWorld()->GetGameState<AShopGameState>();
            TMap<FName, int64> Needs; int32 Visitors = 0;
            const bool bForecast = Desk->GetNextDayNeeds(Needs, Visitors);
            return FText::FromString(FString::Printf(TEXT("Next trading day: %d visits    |    Purse: %lld bits\n%s"), Visitors,
                State ? State->GetCash() : 0, bForecast ? TEXT("Need to brew deducts matching bottles on sale shelves.") : TEXT("Recipe forecast unavailable: check the planned customer recipes.")));
        }) ];
    TSharedRef<SGridPanel> Table = SNew(SGridPanel);
    const TCHAR* Headers[] = {TEXT("Ingredient"), TEXT("Stored"), TEXT("Need to brew"), TEXT("Paid / due"), TEXT("Capacity"), TEXT("After order"), TEXT("Short by")};
    for (int32 Column = 0; Column < 7; ++Column)
        Table->AddSlot(Column, 0).Padding(0, 0, 16, 12)
        [ SNew(STextBlock).Text(FText::FromString(Headers[Column])).Font(Font).ColorAndOpacity(Ink) ];
    TArray<FName> Ingredients;
    if (Desk.IsValid() && IsValid(Desk->Station)) Desk->Station->StartingIngredients.GetKeys(Ingredients);
    Ingredients.Sort(FNameLexicalLess());
    for (int32 Row = 0; Row < Ingredients.Num(); ++Row)
        for (int32 Column = 0; Column < 7; ++Column)
            Table->AddSlot(Column, Row + 1).Padding(0, 0, 16, 12)
            [ SNew(STextBlock).Font(Font).ColorAndOpacity(Ink).Text_Lambda([this, Ingredient = Ingredients[Row], Column]() { return IngredientCell(Ingredient, Column); }) ];
    Content->AddSlot().AutoHeight()[Table];
    Content->AddSlot().AutoHeight().Padding(0, 6, 0, 12)
    [ SNew(STextBlock).Font(Font).ColorAndOpacity(Ink).Text_Lambda([this]()
        {
            if (!Desk.IsValid()) return FText::GetEmpty();
            TMap<FName, int32> Order; int64 Cost = 0; Desk->GetOrder(Order, Cost);
            TArray<FName> Keys; Desk->Delivery.GetKeys(Keys); Keys.Sort(FNameLexicalLess());
            FString Pack;
            for (FName Key : Keys) Pack += FString::Printf(TEXT("%s%d %s"), Pack.IsEmpty() ? TEXT("") : TEXT(" + "), Desk->Delivery[Key], *Key.ToString());
            return FText::FromString(FString::Printf(TEXT("Selected pack: %s\nQuantity: %d packs    |    This order: %lld bits"), *Pack, Desk->GetPackCount(), Cost));
        }) ];
    TSharedRef<SHorizontalBox> Buttons = SNew(SHorizontalBox);
    auto Button = [this, &Buttons, Font](const TCHAR* Caption, FKey Key)
    {
        Buttons->AddSlot().AutoWidth().Padding(0, 0, 12, 0)
        [ SNew(SButton).IsFocusable(false).ContentPadding(FMargin(12, 10))
            .OnClicked_Lambda([this, Key]() { return UseKey(Key); })
            [ SNew(STextBlock).Text(FText::FromString(Caption)).Font(Font) ] ];
    };
    Button(TEXT("Change pack [R]"), EKeys::R);
    Button(TEXT("Less [F]"), EKeys::F);
    Button(TEXT("More [T]"), EKeys::T);
    Buttons->AddSlot().AutoWidth()
    [ SNew(SButton).IsFocusable(false).ContentPadding(FMargin(12, 10))
        .IsEnabled_Lambda([this]() { return Desk.IsValid() && Desk->CanSubmitOrder(); })
        .OnClicked_Lambda([this]() { return UseKey(EKeys::Enter); })
        [ SNew(STextBlock).Text(FText::FromString(TEXT("Sign & pay [Enter]"))).Font(Font) ] ];
    Content->AddSlot().AutoHeight().Padding(0, 0, 0, 14)[Buttons];
    Content->AddSlot().AutoHeight().Padding(0, 0, 0, 12)
    [ SNew(STextBlock).Font(Font).ColorAndOpacity(Ink).WrapTextAt(770).Text_Lambda([this]()
        {
            if (!Desk.IsValid()) return FText::GetEmpty();
            const FText Status = Desk->GetOrderStatus();
            return Status.IsEmpty() ? FText::FromString(TEXT("Ready to sign. Quantity 0 confirms no additional supplies.")) : Status;
        }) ];
    Content->AddSlot().AutoHeight().Padding(0, 0, 0, 14)
    [ SNew(STextBlock).Font(Font).ColorAndOpacity(Ink).Text(FText::FromString(TEXT("Paid orders arrive next morning. Nothing is delivered today.\nAfter paying, change pack or quantity to place another order."))) ];
    Content->AddSlot().AutoHeight().HAlign(HAlign_Right)
    [ SNew(SButton).IsFocusable(false).ContentPadding(FMargin(16, 8)).OnClicked_Lambda([this]() { return UseKey(EKeys::F10); })
        [ SNew(STextBlock).Text(FText::FromString(TEXT("Close book [F10]"))).Font(Font) ] ];
    ChildSlot
    [ SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(0, 0, 0, 0.65f)).Padding(24)
        [ SNew(SScaleBox).Stretch(EStretch::ScaleToFit).StretchDirection(EStretchDirection::DownOnly)
            [ SNew(SBox).WidthOverride(850)
                [ SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(0.86f, 0.77f, 0.57f)).Padding(24)[Content] ] ] ] ];
}

FReply SShopOrderingPage::UseKey(FKey Key)
{
    if (Controller.IsValid()) Controller->HandleOrderingKey(Key);
    return FReply::Handled();
}

FReply SShopOrderingPage::OnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
    if (Event.IsRepeat()) return FReply::Handled();
    return UseKey(Event.GetKey());
}

FText SShopOrderingPage::IngredientCell(FName Ingredient, int32 Column) const
{
    if (Column == 0) return FText::FromName(Ingredient);
    if (!Desk.IsValid() || !IsValid(Desk->Station)) return FText::GetEmpty();
    const AShopGameState* State = Desk->GetWorld()->GetGameState<AShopGameState>();
    if (!State) return FText::GetEmpty();
    TMap<FName, int64> Needs; int32 Visitors = 0;
    const bool bForecast = Desk->GetNextDayNeeds(Needs, Visitors);
    TMap<FName, int32> Order; int64 Cost = 0; Desk->GetOrder(Order, Cost);
    const int64 Stored = Desk->Station->GetIngredientQuantity(Ingredient);
    const int64 Paid = int64(State->GetPendingIngredientQuantity(Ingredient)) + State->GetDeliveredQuantity(Ingredient);
    // A signed selection is already part of Paid, so never count it twice.
    const int64 Projected = Stored + Paid + (Desk->HasSubmittedOrder() ? 0 : Order.FindRef(Ingredient));
    switch (Column)
    {
    case 1: return FText::AsNumber(Stored);
    case 2: return bForecast ? FText::AsNumber(Needs.FindRef(Ingredient)) : FText::FromString(TEXT("?"));
    case 3: return FText::AsNumber(Paid);
    case 4: return FText::AsNumber(Desk->Station->GetIngredientCapacity());
    case 5: return FText::AsNumber(Projected);
    case 6: return bForecast ? FText::AsNumber(FMath::Max<int64>(0, Needs.FindRef(Ingredient) - Projected)) : FText::FromString(TEXT("?"));
    default: return FText::GetEmpty();
    }
}
