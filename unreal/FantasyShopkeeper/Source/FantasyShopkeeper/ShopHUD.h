#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ShopHUD.generated.h"

UCLASS()
class FANTASYSHOPKEEPER_API AShopHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void DrawHUD() override;
};
