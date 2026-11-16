#include "ShopGameState.h"

#include "ShopMorningSave.h"
#include "ShopBrewingStation.h"
#include "ShopBrewingRecipe.h"
#include "ShopCustomer.h"
#include "ShopCustomerFlow.h"
#include "ShopPickupItem.h"
#include "ShopShelfSlot.h"
#include "ShopShelfSection.h"
#include "ShopTill.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#if PLATFORM_WINDOWS
#include "Windows/WindowsHWrapper.h"
#endif

bool UShopMorningSave::Encode(UShopMorningSave* Save, TArray<uint8>& Bytes)
{
    TArray<uint8> Payload;
    if (!Save || !UGameplayStatics::SaveGameToMemory(Save, Payload)) return false;
    const uint8 Magic[] = {'F', 'S', 'K', 'M', 'O', 'R', 'N', '1'};
    uint8 Digest[20];
    FSHA1::HashBuffer(Payload.GetData(), Payload.Num(), Digest);
    Bytes.Reset();
    Bytes.Append(Magic, 8);
    Bytes.Append(Digest, 20);
    Bytes.Append(Payload);
    return true;
}

UShopMorningSave* UShopMorningSave::Decode(const TArray<uint8>& Bytes)
{
    const uint8 Magic[] = {'F', 'S', 'K', 'M', 'O', 'R', 'N', '1'};
    if (Bytes.Num() <= 28 || FMemory::Memcmp(Bytes.GetData(), Magic, 8) != 0) return nullptr;
    uint8 Digest[20];
    FSHA1::HashBuffer(Bytes.GetData() + 28, Bytes.Num() - 28, Digest);
    if (FMemory::Memcmp(Bytes.GetData() + 8, Digest, 20) != 0) return nullptr;
    TArray<uint8> Payload;
    Payload.Append(Bytes.GetData() + 28, Bytes.Num() - 28);
    return Cast<UShopMorningSave>(UGameplayStatics::LoadGameFromMemory(Payload));
}

namespace
{
    FString SlotKey(const AShopShelfSlot* Slot)
    {
        if (const AShopShelfSection* Section = Slot->GetSection())
        {
            for (int32 Index = 0; Index < 8; ++Index)
            {
                if (Section->GetSlot(Index) == Slot)
                {
                    return Section->GetName() + TEXT(":") + FString::FromInt(Index);
                }
            }
            return FString();
        }
        return Slot->GetName();
    }

    TMap<FString, AShopShelfSlot*> GatherSlots(UWorld* World)
    {
        TMap<FString, AShopShelfSlot*> Slots;
        for (TActorIterator<AShopShelfSlot> It(World); It; ++It)
        {
            Slots.Add(SlotKey(*It), *It);
        }
        return Slots;
    }

    AShopCustomerFlow* FindFlow(UWorld* World)
    {
        TActorIterator<AShopCustomerFlow> It(World);
        return It ? *It : nullptr;
    }

    bool ReplaceCheckpoint(const FString& Destination, const FString& Temporary)
    {
#if PLATFORM_WINDOWS
        // FileManager::Move deletes the old file first. A same-volume Windows
        // replacement keeps the previous checkpoint intact if the rename fails.
        const FString To = IFileManager::Get().ConvertToAbsolutePathForExternalAppForWrite(*Destination);
        const FString From = IFileManager::Get().ConvertToAbsolutePathForExternalAppForWrite(*Temporary);
        return MoveFileExW(*From, *To, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
        return false; // Add and verify an atomic replacement when another platform ships.
#endif
    }
}

void AShopGameState::InitializeDay(const FString& SavePath)
{
    if (bDayInitialized || !HasAuthority()) return;
    bDayInitialized = true;
    CheckpointPath = SavePath;
    if (AShopCustomerFlow* Flow = FindFlow(GetWorld())) Flow->ResetMorning();
    // Older maps may still contain a placed demonstration customer.
    for (TActorIterator<AShopCustomer> It(GetWorld()); It; ++It) It->Destroy();
    for (TActorIterator<AShopTill> It(GetWorld()); It; ++It) It->AssignCustomer(nullptr);
    if (!CheckpointPath.IsEmpty() && IFileManager::Get().FileExists(*CheckpointPath))
    {
        RestoreMorning();
    }
    else
    {
        WriteMorning(CaptureMorning(Day));
    }
}

bool AShopGameState::CanStartVisit() const
{
    return bDayInitialized && !bChangingEconomy && DayPhase == EShopDayPhase::Trading;
}

bool AShopGameState::OpenShop()
{
    AShopCustomerFlow* Flow = FindFlow(GetWorld());
    if (!HasAuthority() || !bDayInitialized || bChangingEconomy || DayPhase != EShopDayPhase::Preparation
        || !Flow || Flow->VisitsPerDay < 1 || Flow->VisitsPerDay > 100) return false;
    DayPhase = EShopDayPhase::Trading;
    return true;
}

bool AShopGameState::CloseShop()
{
    if (!HasAuthority() || !bDayInitialized || bChangingEconomy || DayPhase != EShopDayPhase::Trading) return false;
    DayPhase = EShopDayPhase::Closing;
    UpdateDay();
    return true;
}

void AShopGameState::UpdateDay()
{
    if (!HasAuthority() || !bDayInitialized || bChangingEconomy) return;
    const AShopCustomerFlow* Flow = FindFlow(GetWorld());
    if (!Flow || Flow->GetActiveCustomer()) return;
    if (DayPhase == EShopDayPhase::Closing
        || (DayPhase == EShopDayPhase::Trading && Flow->GetStartedVisits() >= Flow->VisitsPerDay))
    {
        DayPhase = EShopDayPhase::Evening;
    }
}

bool AShopGameState::AdvanceMorning()
{
    if (!HasAuthority() || !bDayInitialized || bChangingEconomy || DayPhase != EShopDayPhase::Evening || Day == MAX_int32) return false;
    TGuardValue<bool> Guard(bChangingEconomy, true);
    UShopMorningSave* Save = CaptureMorning(Day + 1);
    if (!Save)
    {
        CheckpointMessage = TEXT("Put down carried items before the next morning.");
        return false;
    }
    if (!WriteMorning(Save)) return false;
    ++Day;
    Takings = SupplySpending = 0;
    DayPhase = EShopDayPhase::Preparation;
    if (AShopCustomerFlow* Flow = FindFlow(GetWorld())) Flow->ResetMorning();
    return true;
}

UShopMorningSave* AShopGameState::CaptureMorning(int32 MorningDay) const
{
    UShopMorningSave* Save = NewObject<UShopMorningSave>();
    Save->Day = MorningDay;
    Save->Cash = Cash;
    for (TActorIterator<AShopBrewingStation> It(GetWorld()); It; ++It)
    {
        if (It->bBrewing) return nullptr;
        FShopSavedIngredients Entry;
        Entry.Station = It->GetName();
        Entry.Quantities = It->Ingredients;
        Save->Ingredients.Add(Entry);
    }
    for (TActorIterator<AShopPickupItem> It(GetWorld()); It; ++It)
    {
        if (It->GetCarrier()) return nullptr;
        FShopSavedStock Entry;
        Entry.ItemClass = It->GetClass()->GetPathName();
        Entry.Product = It->ProductId;
        Entry.DisplayName = It->DisplayName;
        Entry.Transform = It->GetActorTransform();
        if (It->GetCurrentSlot()) Entry.Slot = SlotKey(It->GetCurrentSlot());
        Save->Stock.Add(Entry);
    }
    return Save;
}

bool AShopGameState::ValidateMorning(const UShopMorningSave* Save) const
{
    if (!Save || Save->Version != 1 || Save->Day < 1 || Save->Cash < 0 || Save->Stock.Num() > 2048) return false;
    TMap<FString, AShopBrewingStation*> Stations;
    // Only current recipe/item classes are trusted; a save cannot instantiate arbitrary actors.
    TMap<FString, TSet<FName>> Products;
    for (TActorIterator<AShopPickupItem> It(GetWorld()); It; ++It)
        Products.FindOrAdd(It->GetClass()->GetPathName()).Add(It->ProductId);
    for (TActorIterator<AShopBrewingStation> It(GetWorld()); It; ++It)
    {
        Stations.Add(It->GetName(), *It);
        TArray<TObjectPtr<UShopBrewingRecipe>> Recipes = It->AvailableRecipes;
        Recipes.Add(It->Recipe);
        for (const UShopBrewingRecipe* Recipe : Recipes)
            if (IsValid(Recipe) && Recipe->IsRecipeValid())
                Products.FindOrAdd(Recipe->OutputItemClass->GetPathName()).Add(Recipe->ProductId);
    }
    if (Save->Ingredients.Num() != Stations.Num()) return false;
    TSet<FString> SeenStations;
    for (const FShopSavedIngredients& Entry : Save->Ingredients)
    {
        AShopBrewingStation* const* Station = Stations.Find(Entry.Station);
        if (!Station || SeenStations.Contains(Entry.Station) || Entry.Quantities.Num() != (*Station)->StartingIngredients.Num()) return false;
        SeenStations.Add(Entry.Station);
        for (const auto& Quantity : Entry.Quantities)
            if (!(*Station)->StartingIngredients.Contains(Quantity.Key) || Quantity.Value < 0 || Quantity.Value > (*Station)->IngredientCapacity) return false;
    }
    const auto Slots = GatherSlots(GetWorld());
    TSet<FString> Occupied;
    TMap<AShopShelfSection*, FName> Locks;
    for (const FShopSavedStock& Entry : Save->Stock)
    {
        const TSet<FName>* Allowed = Products.Find(Entry.ItemClass);
        const FVector Scale = Entry.Transform.GetScale3D();
        if (!Allowed || !Allowed->Contains(Entry.Product) || Entry.Product.IsNone()
            || !Entry.Transform.IsValid() || Scale.GetMin() <= 0 || Scale.GetMax() > 100
            || Entry.Transform.GetLocation().GetAbsMax() > 1000000) return false;
        if (!Entry.Slot.IsEmpty())
        {
            AShopShelfSlot* const* Slot = Slots.Find(Entry.Slot);
            if (!Slot || Occupied.Contains(Entry.Slot)) return false;
            Occupied.Add(Entry.Slot);
            if (AShopShelfSection* Section = (*Slot)->GetSection())
            {
                if (Locks.Contains(Section) && Locks[Section] != Entry.Product) return false;
                Locks.Add(Section, Entry.Product);
            }
        }
    }
    return true;
}

bool AShopGameState::WriteMorning(UShopMorningSave* Save)
{
    if (bCheckpointBlocked)
    {
        CheckpointMessage = TEXT("Invalid checkpoint preserved. Move Saved/SaveGames/Morning.sav aside before restarting.");
        return false;
    }
    if (!ValidateMorning(Save))
    {
        CheckpointMessage = TEXT("Morning could not be saved: stock or ingredients are invalid.");
        return false;
    }
    // An empty path is an explicitly isolated, in-memory automation session.
    if (CheckpointPath.IsEmpty()) return true;
    TArray<uint8> Bytes;
    const FString Temporary = CheckpointPath + TEXT(".tmp");
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(CheckpointPath), true);
    if (!UShopMorningSave::Encode(Save, Bytes)
        || !FFileHelper::SaveArrayToFile(Bytes, *Temporary)
        || !ReplaceCheckpoint(CheckpointPath, Temporary))
    {
        CheckpointMessage = TEXT("Morning save failed. Still in the current day; try again.");
        return false;
    }
    CheckpointMessage = TEXT("Morning checkpoint saved. Reopening restores this morning.");
    return true;
}

bool AShopGameState::RestoreMorning()
{
    if (!HasAuthority() || bChangingEconomy || !bDayInitialized || CheckpointPath.IsEmpty()) return false;
    TGuardValue<bool> Guard(bChangingEconomy, true);
    TArray<uint8> Bytes;
    const int64 Size = IFileManager::Get().FileSize(*CheckpointPath);
    UShopMorningSave* Save = nullptr;
    if (Size > 0 && Size <= 4 * 1024 * 1024 && FFileHelper::LoadFileToArray(Bytes, *CheckpointPath))
        Save = UShopMorningSave::Decode(Bytes);
    if (!ValidateMorning(Save))
    {
        bCheckpointBlocked = true;
        CheckpointMessage = TEXT("Invalid checkpoint preserved. Move Saved/SaveGames/Morning.sav aside before restarting.");
        return false;
    }
    // Stage all outputs before touching the live world. Failed spawns leave it intact.
    TArray<AShopPickupItem*> OldItems;
    for (TActorIterator<AShopPickupItem> It(GetWorld()); It; ++It) OldItems.Add(*It);
    TArray<AShopPickupItem*> NewItems;
    for (const FShopSavedStock& Entry : Save->Stock)
    {
        UClass* Class = FindObject<UClass>(nullptr, *Entry.ItemClass);
        AShopPickupItem* Item = GetWorld()->SpawnActorDeferred<AShopPickupItem>(Class, Entry.Transform,
            nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
        if (Item)
        {
            Item->ProductId = Entry.Product;
            Item->DisplayName = Entry.DisplayName;
            Item->FinishSpawning(Entry.Transform);
        }
        if (!IsValid(Item) || Item->GetCarrier() || Item->GetCurrentSlot())
        {
            if (IsValid(Item)) Item->Destroy();
            for (AShopPickupItem* Staged : NewItems) Staged->Destroy();
            CheckpointMessage = TEXT("Could not restore stock. Current shop unchanged.");
            return false;
        }
        NewItems.Add(Item);
    }
    for (AShopPickupItem* Item : OldItems) Item->Destroy();
    if (AShopCustomerFlow* Flow = FindFlow(GetWorld())) Flow->ResetMorning();
    for (TActorIterator<AShopCustomer> It(GetWorld()); It; ++It) It->Destroy();
    for (TActorIterator<AShopTill> It(GetWorld()); It; ++It) It->AssignCustomer(nullptr);
    const auto Slots = GatherSlots(GetWorld());
    for (int32 Index = 0; Index < NewItems.Num(); ++Index)
    {
        if (!Save->Stock[Index].Slot.IsEmpty())
            ensure(NewItems[Index]->PlaceAtSlot(Slots[Save->Stock[Index].Slot], true));
    }
    for (TActorIterator<AShopBrewingStation> It(GetWorld()); It; ++It)
        for (const FShopSavedIngredients& Entry : Save->Ingredients)
            if (Entry.Station == It->GetName()) It->Ingredients = Entry.Quantities;
    Cash = Save->Cash;
    Day = Save->Day;
    Takings = SupplySpending = 0;
    DayPhase = EShopDayPhase::Preparation;
    bCheckpointBlocked = false;
    CheckpointMessage = TEXT("Morning restored. All later stock and cash changes rolled back together.");
    return true;
}

FString AShopGameState::GetDayText() const
{
    const AShopCustomerFlow* Flow = FindFlow(GetWorld());
    const int32 Started = Flow ? Flow->GetStartedVisits() : 0;
    const int32 Limit = Flow ? Flow->VisitsPerDay : 0;
    switch (DayPhase)
    {
    case EShopDayPhase::Preparation:
        return FString::Printf(TEXT("Day %d | Preparation | O: Open shop (%d visits)"), Day, Limit);
    case EShopDayPhase::Trading:
        return FString::Printf(TEXT("Day %d | Trading | %d/%d visits started | O: Close, skip %d unstarted visits"), Day, Started, Limit, FMath::Max(0, Limit - Started));
    case EShopDayPhase::Closing:
        return TEXT("Closing | Finish the active customer; no new arrivals. Brew missing stock if needed.");
    default:
        return FString::Printf(TEXT("Day %d | Evening | Takings %lld | Supplies %lld | Cash %lld bits | N: Next morning"), Day, Takings, SupplySpending, Cash);
    }
}
