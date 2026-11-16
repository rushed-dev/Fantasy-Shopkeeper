#include "ShopDayLighting.h"
#include "ShopGameState.h"
#include "EngineUtils.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/RectLight.h"
#include "Components/LightComponent.h"
#include "Components/SkyLightComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"

AShopDayLighting::AShopDayLighting()
{
    PrimaryActorTick.bCanEverTick = true;
}
void AShopDayLighting::BeginPlay()
{
    Super::BeginPlay();
    for (TActorIterator<ADirectionalLight> It(GetWorld()); It; ++It)
    {
        Sun = It->GetLightComponent();
        Sun->SetMobility(EComponentMobility::Movable);
        SunRotation = Sun->GetComponentRotation();
        DayLights.Add(Sun);
        Intensities.Add(Sun->Intensity);
        break;
    }
    for (TActorIterator<ARectLight> It(GetWorld()); It; ++It)
    {
        // The two exterior window fills follow daylight, not the pendant lamps.
        const FVector Position = It->GetActorLocation();
        if (Position.X > 900 || Position.Y > 600)
        {
            ULightComponent* Light = It->GetLightComponent();
            Light->SetMobility(EComponentMobility::Movable);
            DayLights.Add(Light);
            Intensities.Add(Light->Intensity);
        }
    }
    for (TActorIterator<ASkyLight> It(GetWorld()); It; ++It)
    {
        Sky = It->GetLightComponent();
        Sky->SetMobility(EComponentMobility::Movable);
        SkyIntensity = Sky->Intensity;
        break;
    }
}
void AShopDayLighting::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const AShopGameState* State = GetWorld()->GetGameState<AShopGameState>();
    if (!State || !State->IsDayInitialized()) return;
    if (PreviousDay > 0 && PreviousDay != State->GetDay())
    {
        if (APlayerController* Player = GetWorld()->GetFirstPlayerController(); Player && Player->PlayerCameraManager)
            Player->PlayerCameraManager->StartCameraFade(1, 0, 2.5f, FLinearColor::Black, false, false);
        NightBlend = 0;
        Daylight = 0.75f;
    }
    PreviousDay = State->GetDay();
    const bool bEvening = State->GetDayPhase() == EShopDayPhase::Evening;
    NightBlend = FMath::FInterpConstantTo(NightBlend, bEvening ? 1.f : 0.f, DeltaSeconds, 0.25f);
    Daylight = FMath::FInterpConstantTo(Daylight, State->GetDayPhase() == EShopDayPhase::Preparation ? 0.75f : 1.f, DeltaSeconds, 0.2f);
    const float Sunlight = FMath::Lerp(Daylight, 0.00001f, NightBlend);
    for (int32 Index = 0; Index < DayLights.Num(); ++Index)
        if (IsValid(DayLights[Index])) DayLights[Index]->SetIntensity(Intensities[Index] * Sunlight);
    if (Sun)
    {
        FRotator Rotation = SunRotation;
        Rotation.Pitch = FMath::Lerp(SunRotation.Pitch, 15.f, NightBlend);
        Sun->SetWorldRotation(Rotation);
    }
    if (Sky) Sky->SetIntensity(SkyIntensity * FMath::Lerp(Daylight, 0.06f, NightBlend));
}
