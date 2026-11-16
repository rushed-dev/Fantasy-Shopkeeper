#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
class AShopPlayerController;
class SShopMenu : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SShopMenu) {} SLATE_ARGUMENT(AShopPlayerController*, Controller) SLATE_END_ARGS()
    void Construct(const FArguments& Args);
    virtual bool SupportsKeyboardFocus() const override { return true; }
    virtual FReply OnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
private:
    TWeakObjectPtr<AShopPlayerController> Controller;
};
