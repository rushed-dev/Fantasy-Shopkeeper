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
#include "Sound/SoundWaveProcedural.h"
#include "Kismet/GameplayStatics.h"

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


void AShopDayLighting::BeginNight()
{
    if (APlayerController* Player = GetWorld()->GetFirstPlayerController())
    {
        Player->SetIgnoreMoveInput(true);
        Player->SetIgnoreLookInput(true);
        if (Player->PlayerCameraManager) Player->PlayerCameraManager->StartCameraFade(0, 1, 1.4f, FLinearColor::Black, false, true);
    }
    // Original short wind/cricket cue. No external media or streaming service.
    NightSound = NewObject<USoundWaveProcedural>(this);
    NightSound->SetSampleRate(24000);
    NightSound->NumChannels = 1;
    NightSound->Duration = 3.5f;
    TArray<int16> Samples;
    Samples.SetNumUninitialized(84000);
    FRandomStream Noise(741);
    float Wind = 0;
    for (int32 Index = 0; Index < Samples.Num(); ++Index)
    {
        const float Time = Index / 24000.f;
        Wind = 0.98f * Wind + 0.02f * Noise.FRandRange(-1.f, 1.f);
        const float Envelope = FMath::Min(1.f, FMath::Min(Time / 0.4f, (3.5f - Time) / 0.7f));
        const float Pulse = FMath::Pow(FMath::Max(0.f, FMath::Sin(Time * 27.f)), 8.f);
        Samples[Index] = int16(Envelope * (Wind * 5000 + Pulse * FMath::Sin(Time * 14451.f) * 700));
    }
    NightSound->QueueAudio(reinterpret_cast<const uint8*>(Samples.GetData()), Samples.Num() * sizeof(int16));
    UGameplayStatics::PlaySound2D(this, NightSound, 0.5f);
}
void AShopDayLighting::EndNight(bool bMorning)
{
    if (APlayerController* Player = GetWorld()->GetFirstPlayerController())
    {
        Player->SetIgnoreMoveInput(false);
        Player->SetIgnoreLookInput(false);
        if (Player->PlayerCameraManager) Player->PlayerCameraManager->StartCameraFade(1, 0, 2.f, FLinearColor::Black, false, false);
    }
    if (bMorning)
    {
        PreviousDay = GetWorld()->GetGameState<AShopGameState>()->GetDay();
        NightBlend = 0;
        Daylight = 0.75f;
    }
}
