#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShopDayLighting.generated.h"
class ULightComponent;
class USkyLightComponent;

// Phase-driven light transitions; no clock or deadline advances the day.
UCLASS()
class FANTASYSHOPKEEPER_API AShopDayLighting : public AActor
{
    GENERATED_BODY()
public:
    AShopDayLighting();
    virtual void Tick(float DeltaSeconds) override;
    void BeginNight();
    void EndNight(bool bMorning);
    float GetNightBlend() const { return NightBlend; }
protected:
    virtual void BeginPlay() override;
private:
    UPROPERTY(Transient) TArray<TObjectPtr<ULightComponent>> DayLights;
    TArray<float> Intensities;
    UPROPERTY(Transient) TObjectPtr<ULightComponent> Sun;
    UPROPERTY(Transient) TObjectPtr<USkyLightComponent> Sky;
    FRotator SunRotation;
    float SkyIntensity = 0.6f;
    float NightBlend = 0;
    float Daylight = 0.75f;
    int32 PreviousDay = 0;
    UPROPERTY(Transient) TObjectPtr<class USoundWaveProcedural> NightSound;
};
