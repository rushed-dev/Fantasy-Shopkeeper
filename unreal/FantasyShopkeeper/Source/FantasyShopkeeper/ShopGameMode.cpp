#include "ShopGameMode.h"

#include "GameFramework/Pawn.h"
#include "NavigationSystem.h"
#include "NavMesh/RecastNavMesh.h"
#include "ShopPlayerController.h"
#include "ShopHUD.h"
#include "ShopGameState.h"
#include "UObject/ConstructorHelpers.h"
#include "Misc/Paths.h"

AShopGameMode::AShopGameMode()
{
    PlayerControllerClass = AShopPlayerController::StaticClass();
    HUDClass = AShopHUD::StaticClass();
    GameStateClass = AShopGameState::StaticClass();
    static ConstructorHelpers::FClassFinder<APawn> Character(TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"));
    if (Character.Succeeded())
    {
        DefaultPawnClass = Character.Class;
    }
}

void AShopGameMode::StartPlay()
{
    UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    const ARecastNavMesh* NavMesh = Navigation ? Cast<ARecastNavMesh>(Navigation->GetDefaultNavDataInstance()) : nullptr;
    if (NavMesh && !NavMesh->GetNavMeshBounds().IsValid)
    {
        // A newly imported map can have bounds and settings without any tiles.
        Navigation->Build();
    }
    Super::StartPlay();
    if (AShopGameState* State = GetWorld()->GetGameState<AShopGameState>())
    {
        State->InitializeDay(GIsAutomationTesting ? FString() : FPaths::ProjectSavedDir() / TEXT("SaveGames/Morning.sav"));
    }
}
