#include "ShopMenu.h"
#include "ShopPlayerController.h"
#include "ShopGameState.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSlider.h"

void SShopMenu::Construct(const FArguments& Args)
{
    Controller = Args._Controller;
    const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Regular", 18);
    TSharedRef<SVerticalBox> Content = SNew(SVerticalBox);
    Content->AddSlot().AutoHeight().Padding(0, 0, 0, 18)
    [ SNew(STextBlock).Text(FText::FromString(TEXT("FANTASY SHOPKEEPER / THE LANTERN WALK"))).Font(FCoreStyle::GetDefaultFontStyle("Bold", 23)) ];
    Content->AddSlot().AutoHeight().Padding(0, 0, 0, 18)
    [ SNew(STextBlock).Text_Lambda([this]() { return FText::FromString(Controller.IsValid() ? Controller->GetMenuText() : FString()); }).Font(Font).WrapTextAt(780) ];
    auto Button = [&](const TCHAR* Label, FName Action)
    {
        Content->AddSlot().AutoHeight().Padding(0, 3)
        [ SNew(SButton).ContentPadding(FMargin(12, 7)).OnClicked_Lambda([this, Action]() { if (Controller.IsValid()) Controller->MenuAction(Action); return FReply::Handled(); })
            [ SNew(STextBlock).Text(FText::FromString(Label)).Font(Font) ] ];
    };
    const FName Page = Controller->GetMenuPage();
    if (Page == TEXT("Main"))
    {
        Button(TEXT("Enter / resume the shop"), TEXT("Resume"));
        Button(TEXT("How to keep shop"), TEXT("Help"));
        Button(TEXT("Requests and supplier account"), TEXT("Journal"));
        Button(TEXT("The first three days"), TEXT("Chapter"));
        Button(TEXT("Settings"), TEXT("Settings"));
        Button(TEXT("Restart this morning..."), TEXT("Restart"));
        Button(TEXT("Start a new shop..."), TEXT("Fresh"));
        Button(TEXT("Credits"), TEXT("Credits"));
        Button(TEXT("Quit..."), TEXT("Quit"));
    }
    else if (Page == TEXT("Journal"))
    {
        const AShopGameState* State = Controller->GetWorld()->GetGameState<AShopGameState>();
        if (State && State->GetDay() == 1 && State->GetLanternStory() == EShopLanternStory::Available)
        {
            Button(TEXT("Promise one Moonlit Ink for day 2 (paid 18 bits)"), TEXT("AcceptStory"));
            Button(TEXT("Leave the lantern arrangements to Fen"), TEXT("DeclineStory"));
        }
        if (State && Controller->CanUseSupplierDesk() && State->CanUseSupplierCredit()) Button(TEXT("Order on account: 6 Moonleaf + 6 Water / owe 18 bits"), TEXT("Credit"));
        if (State && Controller->CanUseSupplierDesk() && State->CanOrderSupplies() && State->GetSupplierDebt() > 0 && State->GetCash() >= 18) Button(TEXT("Repay supplier: 18 bits"), TEXT("Repay"));
        Button(TEXT("Back / decide later"), TEXT("Main"));
    }
    else if (Page == TEXT("Settings"))
    {
        Content->AddSlot().AutoHeight().Padding(0, 0, 0, 6)[SNew(STextBlock).Font(Font).Text_Lambda([this]()
            { return FText::FromString(Controller->GetGraphicsQuality() == 2 ? TEXT("Graphics: High") : Controller->GetGraphicsQuality() == 3 ? TEXT("Graphics: Epic") : TEXT("Graphics: Custom")); })];
        Button(TEXT("High graphics (recommended)"), TEXT("QualityHigh"));
        Button(TEXT("Epic graphics"), TEXT("QualityEpic"));
        Content->AddSlot().AutoHeight()[SNew(STextBlock).Font(Font).Text_Lambda([this]() { return FText::FromString(FString::Printf(TEXT("Mouse sensitivity: %.2f"), Controller->GetLookSensitivity())); })];
        Content->AddSlot().AutoHeight().Padding(0, 10)[SNew(SSlider).MinValue(0.25f).MaxValue(2.f).Value(Controller->GetLookSensitivity()).OnValueChanged_Lambda([this](float Value) { if (Controller.IsValid()) Controller->SetLookSensitivity(Value); })];
        Content->AddSlot().AutoHeight()[SNew(STextBlock).Font(Font).Text_Lambda([this]() { return FText::FromString(FString::Printf(TEXT("Master volume: %.0f%%"), 100 * Controller->GetMasterVolume())); })];
        Content->AddSlot().AutoHeight().Padding(0, 10)[SNew(SSlider).MinValue(0.f).MaxValue(1.f).Value(Controller->GetMasterVolume()).OnValueChanged_Lambda([this](float Value) { if (Controller.IsValid()) Controller->SetMasterVolume(Value); })];
        Button(TEXT("Back"), TEXT("Main"));
    }
    else if (Page == TEXT("Restart") || Page == TEXT("Fresh") || Page == TEXT("Quit"))
    {
        Button(TEXT("Confirm"), Page == TEXT("Restart") ? TEXT("ConfirmRestart") : Page == TEXT("Fresh") ? TEXT("ConfirmFresh") : TEXT("ConfirmQuit"));
        Button(TEXT("Cancel"), TEXT("Main"));
    }
    else Button(TEXT("Back"), TEXT("Main"));
    ChildSlot [ SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(0.035f, 0.025f, 0.015f, 0.95f)).Padding(24)
        [ SNew(SScaleBox).Stretch(EStretch::ScaleToFit).StretchDirection(EStretchDirection::DownOnly)
            [ SNew(SBox).WidthOverride(850).MaxDesiredHeight(650)[ SNew(SBorder).Padding(24)[ SNew(SScrollBox) + SScrollBox::Slot()[Content] ] ] ] ] ];
}
FReply SShopMenu::OnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
    if (Event.GetKey() == EKeys::F1 || Event.GetKey() == EKeys::F10)
    { if (Controller.IsValid()) Controller->CloseShopMenu(); }
    return FReply::Handled();
}
