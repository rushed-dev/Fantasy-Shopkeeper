#include "ShopStoryLantern.h"
#include "ShopGameState.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "UObject/ConstructorHelpers.h"

AShopStoryLantern::AShopStoryLantern()
{
    PrimaryActorTick.bCanEverTick = true; PrimaryActorTick.TickInterval = 0.2f;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    Frame = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BrassLantern")); Frame->SetupAttachment(RootComponent);
    Glow = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WardedLight")); Glow->SetupAttachment(RootComponent);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Brass(TEXT("/Game/Shop/Art/Materials/M_Brass.M_Brass"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> WarmGlow(TEXT("/Game/Shop/Art/Materials/M_LampGlow.M_LampGlow"));
    Frame->SetStaticMesh(Cylinder.Object); Frame->SetMaterial(0, Brass.Object);
    Frame->SetRelativeScale3D(FVector(0.3, 0.3, 0.05)); Frame->SetRelativeLocation(FVector(0, 0, -12));
    Glow->SetStaticMesh(Sphere.Object); Glow->SetMaterial(0, WarmGlow.Object); Glow->SetRelativeScale3D(FVector(0.19, 0.19, 0.27));
    Frame->SetCollisionEnabled(ECollisionEnabled::NoCollision); Glow->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Frame->SetCanEverAffectNavigation(false); Glow->SetCanEverAffectNavigation(false);
    Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("LanternLight")); Light->SetupAttachment(RootComponent);
    Light->SetIntensity(140); Light->SetAttenuationRadius(180); Light->SetLightColor(FLinearColor(1.f, 0.65f, 0.25f)); Light->SetCastShadows(false);
    SetActorHiddenInGame(true);
}
void AShopStoryLantern::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const AShopGameState* State = GetWorld()->GetGameState<AShopGameState>();
    bLit = State && State->GetLanternStory() == EShopLanternStory::Fulfilled;
    SetActorHiddenInGame(!bLit); Light->SetVisibility(bLit);
}
