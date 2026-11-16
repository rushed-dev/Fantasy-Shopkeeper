#include "ShopGameState.h"
#include "ShopBrewingStation.h"
#include "ShopSupplyCrate.h"
#include "EngineUtils.h"

bool AShopGameState::ChooseLanternStory(bool bAccept)
{
    if (!HasAuthority() || !bDayInitialized || bChangingEconomy || bOvernight || Day != 1
        || LanternStory != EShopLanternStory::Available) return false;
    LanternStory = bAccept ? EShopLanternStory::Accepted : EShopLanternStory::Declined;
    return true;
}

FString AShopGameState::GetJournalText() const
{
    FString Story;
    switch (LanternStory)
    {
    case EShopLanternStory::Available:
        Story = Day == 1
            ? TEXT("Fen's note: The Lantern Walk needs a light that won't attract moths with opinions. Moonlit Ink makes a useful ward. Could you have one bottle on your sale shelves for my visit tomorrow? I'll pay the usual 18 bits.\n\nThis replaces Fen's ordinary purchase, not an extra order. No deposit, donation or automatic reservation. Declining changes no prices or customer opportunities.")
            : TEXT("Fen borrowed the bakery's lantern for the walk. The shop remains open for ordinary trade."); break;
    case EShopLanternStory::Accepted: Story = TEXT("Promised to Fen: stock ONE Moonlit Ink for day 2. Fen will collect it and pay 18 bits through normal checkout. A shelved bottle is sufficient; nothing is reserved automatically. If you cannot supply it, Fen will borrow a lantern. No fine."); break;
    case EShopLanternStory::Declined: Story = TEXT("You left the lantern arrangements to Fen. The bakery can lend one. Fen still shops here at the usual prices."); break;
    case EShopLanternStory::Fulfilled: Story = TEXT("Fen bought the ink and marked the lantern. It now glows beside your entrance: a small part of the Lantern Walk, made here. No moths offering directions. Thank you for the help."); break;
    case EShopLanternStory::Missed: Story = TEXT("The ink wasn't collected on day 2. Fen borrowed the bakery's lantern: payment in buns, a complicated exchange rate. No fine; ordinary trade continues."); break;
    }
    return Story + FString::Printf(TEXT("\n\nSupplier account: %lld bits owed.\nRecovery terms: when supplies are short and your purse holds less than 12 bits, order 6 Moonleaf and 6 Water on account for next morning. Owe 18 bits total, no interest or deadline. One outstanding parcel loan. Order and repay at the supplier book in the evening."), SupplierDebt);
}

bool AShopGameState::CanUseSupplierCredit() const
{
    if (!CanOrderSupplies() || SupplierDebt != 0 || Cash >= 12 || SupplySpending > MAX_int64 - 18 || GetRestWarning().IsEmpty()) return false;
    TActorIterator<AShopSupplyCrate> Desk(GetWorld());
    return Desk && CanReceiveOrder(Desk->Station, {{TEXT("Moonleaf"), 6}, {TEXT("Spring Water"), 6}});
}

bool AShopGameState::UseSupplierCredit()
{
    if (!HasAuthority() || !CanUseSupplierCredit()) return false;
    TActorIterator<AShopSupplyCrate> Desk(GetWorld());
    TGuardValue<bool> Guard(bChangingEconomy, true);
    DeliveryStation = Desk->Station;
    PendingDelivery.FindOrAdd(TEXT("Moonleaf")) += 6;
    PendingDelivery.FindOrAdd(TEXT("Spring Water")) += 6;
    SupplierDebt = 18;
    SupplySpending += 18;
    bSuppliesConfirmed = true;
    CheckpointMessage = TEXT("Supplies ordered on account. Owe 18 bits; parcel arrives next morning. No interest or deadline.");
    return true;
}

bool AShopGameState::RepaySupplierCredit()
{
    if (!HasAuthority() || !CanOrderSupplies() || SupplierDebt != 18 || Cash < SupplierDebt) return false;
    TGuardValue<bool> Guard(bChangingEconomy, true);
    Cash -= SupplierDebt;
    SupplierDebt = 0;
    CheckpointMessage = TEXT("Supplier account repaid: 18 bits. Thank you.");
    return true;
}

FString AShopGameState::GetChapterText() const
{
    if (Day < 3 || (Day == 3 && DayPhase != EShopDayPhase::Evening))
        return TEXT("THE LANTERN WALK\nThree days to find your feet. Prepare and sell your potions, plan the next delivery, and decide whether to help Fen. No time limit. Ordinary trading is enough.");
    bool Storage = false, Batch = false;
    for (TActorIterator<AShopBrewingStation> It(GetWorld()); It; ++It) { Storage |= It->HasStorageUpgrade(); Batch |= It->HasBatchUpgrade(); }
    return FString::Printf(TEXT("THE LANTERN WALK\nYour first three days are complete. The neighbours gather for the walk. You have found your place behind the counter.\n\n%s\n\nYour shop: %s / %s\nPurse: %lld bits    Supplier account: %lld bits\n\nKeep trading as long as you like. Order tomorrow's supplies and rest to checkpoint this evening's progress. Restarting now still returns to this morning."),
        LanternStory == EShopLanternStory::Fulfilled ? TEXT("Fen's warded lantern shines beside your door.") : TEXT("Fen borrowed a lantern. Your ordinary trade still helped the neighbourhood."),
        Storage ? TEXT("extra drawers fitted") : TEXT("original drawers"), Batch ? TEXT("batch kit fitted") : TEXT("single-bottle bench"), Cash, SupplierDebt);
}
