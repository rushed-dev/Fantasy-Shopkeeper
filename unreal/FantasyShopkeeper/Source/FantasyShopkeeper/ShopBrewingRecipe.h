#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ShopBrewingRecipe.generated.h"

class AShopPickupItem;
class USoundBase;

UCLASS(BlueprintType)
class FANTASYSHOPKEEPER_API UShopBrewingRecipe : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
    FName ProductId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
    TMap<FName, int32> IngredientCosts;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
    TSubclassOf<AShopPickupItem> OutputItemClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe|Presentation")
    TObjectPtr<USoundBase> BrewSound;

    bool IsRecipeValid() const;
};
