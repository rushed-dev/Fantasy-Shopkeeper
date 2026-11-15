#include "ShopBrewingRecipe.h"

#include "ShopPickupItem.h"

bool UShopBrewingRecipe::IsRecipeValid() const
{
    if (ProductId.IsNone() || IngredientCosts.IsEmpty() || !OutputItemClass
        || OutputItemClass->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists))
    {
        return false;
    }
    for (const TPair<FName, int32>& Cost : IngredientCosts)
    {
        if (Cost.Key.IsNone() || Cost.Value <= 0)
        {
            return false;
        }
    }
    return true;
}
