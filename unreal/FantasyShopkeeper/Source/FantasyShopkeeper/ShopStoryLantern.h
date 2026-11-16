#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShopStoryLantern.generated.h"
class UStaticMeshComponent;
class UPointLightComponent;
UCLASS()
class AShopStoryLantern : public AActor
{
    GENERATED_BODY()
public:
    AShopStoryLantern();
    virtual void Tick(float DeltaSeconds) override;
    bool IsLit() const { return bLit; }
private:
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Frame;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Glow;
    UPROPERTY() TObjectPtr<UPointLightComponent> Light;
    bool bLit = false;
};
