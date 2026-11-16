#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "ShopMorningSave.generated.h"

// Morning snapshots contain values and stable map identities, never transactions.
USTRUCT()
struct FShopSavedStock
{
    GENERATED_BODY()
    UPROPERTY() FString ItemClass;
    UPROPERTY() FName Product;
    UPROPERTY() FText DisplayName;
    UPROPERTY() FTransform Transform;
    UPROPERTY() FString Slot;
};

USTRUCT()
struct FShopSavedIngredients
{
    GENERATED_BODY()
    UPROPERTY() FString Station;
    UPROPERTY() TMap<FName, int32> Quantities;
};

UCLASS()
class UShopMorningSave : public USaveGame
{
    GENERATED_BODY()
public:
    static bool Encode(UShopMorningSave* Save, TArray<uint8>& Bytes);
    static UShopMorningSave* Decode(const TArray<uint8>& Bytes);
    UPROPERTY() int32 Version = 2;
    UPROPERTY() int32 Day = 1;
    UPROPERTY() int64 Cash = 0;
    UPROPERTY() TArray<FShopSavedIngredients> Ingredients;
    UPROPERTY() TArray<FShopSavedStock> Stock;
    // Version 2 stores unopened parcels; version 1 mornings already had supplies in storage.
    UPROPERTY() FString DeliveryStation;
    UPROPERTY() TMap<FName, int32> Delivery;
};
