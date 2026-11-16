#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
class AShopPlayerController;
class AShopSupplyCrate;

class SShopOrderingPage : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SShopOrderingPage) {}
        SLATE_ARGUMENT(AShopPlayerController*, Controller)
        SLATE_ARGUMENT(AShopSupplyCrate*, Desk)
    SLATE_END_ARGS()
    void Construct(const FArguments& Args);
    virtual bool SupportsKeyboardFocus() const override { return true; }
    virtual FReply OnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
private:
    TWeakObjectPtr<AShopPlayerController> Controller;
    TWeakObjectPtr<AShopSupplyCrate> Desk;
    FReply UseKey(FKey Key);
    FText IngredientCell(FName Ingredient, int32 Column) const;
};
