#include "ShopTillWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "ShopInteractionComponent.h"
#include "ShopPlayerController.h"
#include "ShopTill.h"

TSharedRef<SWidget> UShopTillWidget::RebuildWidget()
{
    WidgetTree = NewObject<UWidgetTree>(this);
    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
    WidgetTree->RootWidget = Canvas;
    UBorder* Panel = WidgetTree->ConstructWidget<UBorder>();
    Panel->SetBrushColor(FLinearColor(0.035f, 0.055f, 0.045f, 0.98f));
    Panel->SetPadding(FMargin(28));
    UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(Panel);
    PanelSlot->SetAnchors(FAnchors(0.5, 0.5));
    PanelSlot->SetAlignment(FVector2D(0.5, 0.5));
    PanelSlot->SetAutoSize(true);
    USizeBox* Sizing = WidgetTree->ConstructWidget<USizeBox>();
    Sizing->SetWidthOverride(590);
    Panel->SetContent(Sizing);
    UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
    Sizing->SetContent(Column);

    auto AddText = [&](const FString& Text, int32 Size)
    {
        UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
        Label->SetText(FText::FromString(Text));
        FSlateFontInfo Font = Label->GetFont();
        Font.Size = Size;
        Label->SetFont(Font);
        Label->SetColorAndOpacity(FSlateColor(FLinearColor(0.94f, 0.9f, 0.76f)));
        Label->SetAutoWrapText(true);
        Column->AddChildToVerticalBox(Label)->SetPadding(FMargin(0, 0, 0, 12));
        return Label;
    };
    auto Button = [&](const FName& Name, const FString& Text)
    {
        UButton* Result = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
        Result->SetBackgroundColor(FLinearColor(0.32f, 0.4f, 0.27f));
        UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
        Label->SetText(FText::FromString(Text));
        Label->SetColorAndOpacity(FSlateColor(FLinearColor::White));
        FSlateFontInfo Font = Label->GetFont();
        Font.Size = 16;
        Label->SetFont(Font);
        Result->SetContent(Label);
        return Result;
    };

    AddText(TEXT("Shop till"), 28);
    OrderText = AddText(TEXT(""), 20);
    AddText(TEXT("Enter the basket total in bits"), 16);
    TotalInput = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("BasketTotalInput"));
    TotalInput->SetHintText(FText::FromString(TEXT("Total")));
    TotalInput->SetSelectAllTextWhenFocused(true);
    TotalInput->OnTextChanged.AddDynamic(this, &UShopTillWidget::TotalChanged);
    TotalInput->OnTextCommitted.AddDynamic(this, &UShopTillWidget::TotalCommitted);
    Column->AddChildToVerticalBox(TotalInput)->SetPadding(FMargin(0, 0, 0, 12));
    AcceptButton = Button(TEXT("AcceptPayment"), TEXT("Accept customer payment"));
    AcceptButton->OnClicked.AddDynamic(this, &UShopTillWidget::AcceptPayment);
    Column->AddChildToVerticalBox(AcceptButton)->SetPadding(FMargin(0, 0, 0, 14));
    PaymentText = AddText(TEXT(""), 18);
    TrayText = AddText(TEXT(""), 18);

    UHorizontalBox* AddRow = WidgetTree->ConstructWidget<UHorizontalBox>();
    UHorizontalBox* RemoveRow = WidgetTree->ConstructWidget<UHorizontalBox>();
    Column->AddChildToVerticalBox(AddRow)->SetPadding(FMargin(0, 0, 0, 8));
    Column->AddChildToVerticalBox(RemoveRow)->SetPadding(FMargin(0, 0, 0, 12));
    UButton* Bit = Button(TEXT("AddBit"), TEXT("Add 1 bit"));
    UButton* Mark = Button(TEXT("AddMark"), TEXT("Add 1 mark (10)"));
    UButton* Crown = Button(TEXT("AddCrown"), TEXT("Add 1 crown (100)"));
    UButton* UndoBit = Button(TEXT("RemoveBit"), TEXT("Take back 1 bit"));
    UButton* UndoMark = Button(TEXT("RemoveMark"), TEXT("Take back 1 mark"));
    UButton* UndoCrown = Button(TEXT("RemoveCrown"), TEXT("Take back 1 crown"));
    Bit->OnPressed.AddDynamic(this, &UShopTillWidget::AddBit);
    Mark->OnPressed.AddDynamic(this, &UShopTillWidget::AddMark);
    Crown->OnPressed.AddDynamic(this, &UShopTillWidget::AddCrown);
    UndoBit->OnPressed.AddDynamic(this, &UShopTillWidget::RemoveBit);
    UndoMark->OnPressed.AddDynamic(this, &UShopTillWidget::RemoveMark);
    UndoCrown->OnPressed.AddDynamic(this, &UShopTillWidget::RemoveCrown);
    CoinButtons = {Bit, Mark, Crown, UndoBit, UndoMark, UndoCrown};
    for (UButton* Entry : CoinButtons)
    {
        Entry->OnReleased.AddDynamic(this, &UShopTillWidget::StopCoinHold);
    }
    for (UButton* Entry : {Bit, Mark, Crown})
    {
        UHorizontalBoxSlot* ChildSlot = AddRow->AddChildToHorizontalBox(Entry);
        ChildSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        ChildSlot->SetPadding(FMargin(0, 0, 8, 0));
    }
    for (UButton* Entry : {UndoBit, UndoMark, UndoCrown})
    {
        UHorizontalBoxSlot* ChildSlot = RemoveRow->AddChildToHorizontalBox(Entry);
        ChildSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        ChildSlot->SetPadding(FMargin(0, 0, 8, 0));
    }
    AddText(TEXT("1 / 2 / 3 add coins. Hold Shift to take them back."), 14);
    ConfirmButton = Button(TEXT("ConfirmSale"), TEXT("Confirm sale"));
    ConfirmButton->OnClicked.AddDynamic(this, &UShopTillWidget::Confirm);
    Column->AddChildToVerticalBox(ConfirmButton)->SetPadding(FMargin(0, 0, 0, 14));
    FeedbackText = AddText(TEXT(""), 16);
    UButton* Back = Button(TEXT("CloseTill"), TEXT("Back to shop (Esc)"));
    Back->OnClicked.AddDynamic(this, &UShopTillWidget::Close);
    Column->AddChildToVerticalBox(Back);
    SetIsFocusable(true);
    return Super::RebuildWidget();
}

void UShopTillWidget::SetTill(AShopTill* InTill)
{
    Till = InTill;
    if (TotalInput && Till.IsValid())
    {
        TotalInput->SetText(FText::FromString(Till->EnteredTotalText));
        Refresh();
    }
}

void UShopTillWidget::Refresh()
{
    if (!Till.IsValid() || !OrderText)
    {
        return;
    }
    const bool Entering = Till->GetPhase() == EShopTillPhase::EnteringTotal;
    const bool Changing = Till->GetPhase() == EShopTillPhase::MakingChange;
    OrderText->SetText(FText::Format(NSLOCTEXT("Shop", "TillOrderLine", "{0}    {1} bits"), Till->GetProductName(), FText::AsNumber(Till->GetQuotedPrice())));
    PaymentText->SetText(Entering ? FText::FromString(TEXT("Waiting for the total"))
        : FText::Format(NSLOCTEXT("Shop", "TillTender", "Customer payment    {0} bits"), FText::AsNumber(Till->GetTender())));
    TrayText->SetText(FText::Format(NSLOCTEXT("Shop", "TillChangeTray", "Change tray    {0} bits\n{1} bits  /  {2} marks  /  {3} crowns"),
        FText::AsNumber(Till->GetChangeTotal()), FText::AsNumber(Till->GetCoinCount(1)),
        FText::AsNumber(Till->GetCoinCount(10)), FText::AsNumber(Till->GetCoinCount(100))));
    FeedbackText->SetText(Till->GetFeedback());
    TotalInput->SetIsEnabled(Entering);
    AcceptButton->SetIsEnabled(Entering);
    ConfirmButton->SetIsEnabled(Changing);
    for (UButton* Entry : CoinButtons)
    {
        Entry->SetIsEnabled(Changing);
    }
}

void UShopTillWidget::FocusInput()
{
    if (TotalInput && Till.IsValid() && Till->GetPhase() == EShopTillPhase::EnteringTotal)
    {
        TotalInput->SetKeyboardFocus();
    }
    else
    {
        SetKeyboardFocus();
    }
}

void UShopTillWidget::NativeTick(const FGeometry& Geometry, float DeltaSeconds)
{
    Super::NativeTick(Geometry, DeltaSeconds);
    if (!Till.IsValid())
    {
        Close();
        return;
    }
    Refresh();
    if (HeldDenomination != 0)
    {
        RepeatAfter -= DeltaSeconds;
        if (RepeatAfter <= 0)
        {
            ChangeCoin(HeldDenomination, bRemoveHeldCoin);
            RepeatAfter = 0.09f;
        }
    }
}

void UShopTillWidget::TotalChanged(const FText& Text)
{
    if (Till.IsValid())
    {
        Till->EnteredTotalText = Text.ToString();
    }
}

void UShopTillWidget::TotalCommitted(const FText& Text, ETextCommit::Type Method)
{
    if (Method == ETextCommit::OnEnter)
    {
        AcceptPayment();
    }
}

void UShopTillWidget::AcceptPayment()
{
    if (Till.IsValid())
    {
        Till->SubmitTotal(TotalInput->GetText().ToString());
        Refresh();
        if (Till->GetPhase() == EShopTillPhase::MakingChange)
        {
            SetKeyboardFocus();
        }
    }
}

void UShopTillWidget::ChangeCoin(int32 Denomination, bool Remove)
{
    if (Till.IsValid())
    {
        if (Remove)
        {
            Till->RemoveChangeCoin(Denomination);
        }
        else
        {
            Till->AddChangeCoin(Denomination);
        }
        Refresh();
    }
}

void UShopTillWidget::StartCoinHold(int32 Denomination, bool Remove)
{
    HeldDenomination = Denomination;
    bRemoveHeldCoin = Remove;
    RepeatAfter = 0.35f;
    ChangeCoin(Denomination, Remove);
}

void UShopTillWidget::StopCoinHold() { HeldDenomination = 0; }
void UShopTillWidget::AddBit() { StartCoinHold(1, false); }
void UShopTillWidget::AddMark() { StartCoinHold(10, false); }
void UShopTillWidget::AddCrown() { StartCoinHold(100, false); }
void UShopTillWidget::RemoveBit() { StartCoinHold(1, true); }
void UShopTillWidget::RemoveMark() { StartCoinHold(10, true); }
void UShopTillWidget::RemoveCrown() { StartCoinHold(100, true); }

void UShopTillWidget::Confirm()
{
    AShopPlayerController* Controller = Cast<AShopPlayerController>(GetOwningPlayer());
    if (Till.IsValid() && Controller)
    {
        Till->ConfirmSale(Controller->ShopInteraction);
        Refresh();
    }
}

void UShopTillWidget::Close()
{
    if (AShopPlayerController* Controller = Cast<AShopPlayerController>(GetOwningPlayer()))
    {
        Controller->CloseTill();
    }
}

FReply UShopTillWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
    if (Event.GetKey() == EKeys::Escape)
    {
        Close();
        return FReply::Handled();
    }
    if (Till.IsValid() && Till->GetPhase() == EShopTillPhase::MakingChange)
    {
        if (Event.GetKey() == EKeys::One || Event.GetKey() == EKeys::Two || Event.GetKey() == EKeys::Three)
        {
            const int32 Denomination = Event.GetKey() == EKeys::One ? 1 : Event.GetKey() == EKeys::Two ? 10 : 100;
            ChangeCoin(Denomination, Event.IsShiftDown());
            return FReply::Handled();
        }
    }
    return Super::NativeOnPreviewKeyDown(Geometry, Event);
}
