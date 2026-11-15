#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Types/SlateEnums.h"
#include "ShopTillWidget.generated.h"

class AShopTill;
class UTextBlock;
class UEditableTextBox;
class UButton;

UCLASS()
class FANTASYSHOPKEEPER_API UShopTillWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void SetTill(AShopTill* InTill);
    void FocusInput();

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaSeconds) override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;

private:
    void Refresh();
    void ChangeCoin(int32 Denomination, bool Remove);
    void StartCoinHold(int32 Denomination, bool Remove);
    UFUNCTION() void StopCoinHold();
    UFUNCTION() void TotalChanged(const FText& Text);
    UFUNCTION() void TotalCommitted(const FText& Text, ETextCommit::Type Method);
    UFUNCTION() void AcceptPayment();
    UFUNCTION() void AddBit();
    UFUNCTION() void AddMark();
    UFUNCTION() void AddCrown();
    UFUNCTION() void RemoveBit();
    UFUNCTION() void RemoveMark();
    UFUNCTION() void RemoveCrown();
    UFUNCTION() void Confirm();
    UFUNCTION() void Close();

    UPROPERTY(Transient) TWeakObjectPtr<AShopTill> Till;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> OrderText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> PaymentText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> TrayText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> FeedbackText;
    UPROPERTY(Transient) TObjectPtr<UEditableTextBox> TotalInput;
    UPROPERTY(Transient) TObjectPtr<UButton> AcceptButton;
    UPROPERTY(Transient) TObjectPtr<UButton> ConfirmButton;
    UPROPERTY(Transient) TArray<TObjectPtr<UButton>> CoinButtons;
    int32 HeldDenomination = 0;
    bool bRemoveHeldCoin = false;
    float RepeatAfter = 0;
};
