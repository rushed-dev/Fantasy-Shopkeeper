#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ShopPlayerController.generated.h"

class UShopInteractionComponent;
class UInputAction;
class UInputMappingContext;
class AShopTill;
class AShopSupplyCrate;
class SWidget;

UCLASS(Blueprintable)
class FANTASYSHOPKEEPER_API AShopPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    AShopPlayerController();
    bool CanUseSupplierDesk() const;
    bool OpenShopMenu(FName Page = TEXT("Main"));
    void CloseShopMenu();
    bool IsShopMenuOpen() const { return MenuWidget.IsValid(); }
    void MenuAction(FName Action);
    FName GetMenuPage() const { return MenuPage; }
    FString GetMenuText() const;
    int32 GetGraphicsQuality() const { return GraphicsQuality; }
    void SetGraphicsQuality(int32 Value);
    float GetLookSensitivity() const { return LookSensitivity; }
    float GetMasterVolume() const { return MasterVolume; }
    void SetLookSensitivity(float Value);
    void SetMasterVolume(float Value);
    FString GetGuideText() const;

    bool RequestRest();
    bool IsRestPromptOpen() const { return bRestPromptOpen; }
    FString GetRestWarningText() const;
    bool OpenOrderingPage(AShopSupplyCrate* Desk);
    void CloseOrderingPage();
    bool IsOrderingPageOpen() const { return OrderingWidget.IsValid(); }
    bool HandleOrderingKey(FKey Key);
    virtual void PlayerTick(float DeltaTime) override;
    bool IsFreshGameConfirmation() const { return bFreshGameConfirmation; }
    bool IsRestartPromptOpen() const { return bRestartPromptOpen; }
    virtual bool InputKey(const FInputKeyEventArgs& Params) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shop")
    TObjectPtr<UShopInteractionComponent> ShopInteraction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shop|Input")
    TObjectPtr<UInputMappingContext> MovementMappingContext;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shop|Input")
    TObjectPtr<UInputMappingContext> MouseLookMappingContext;

protected:
    virtual void SetupInputComponent() override;
    virtual void OnUnPossess() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    TSharedPtr<SWidget> MenuWidget;
    FName MenuPage = TEXT("Main");
    bool bWelcomeShown = false;
    bool bChapterShown = false;
    bool bSettingsLoaded = false;
    int32 GraphicsQuality = 2;
    float LookSensitivity = 1.f;
    float MasterVolume = 0.7f;
    TWeakObjectPtr<AShopSupplyCrate> OrderingDesk;
    TSharedPtr<SWidget> OrderingWidget;
    bool bRestPromptOpen = false;
    void SetRestPrompt(bool bOpen);
    bool bRestartPromptOpen = false;
    bool bFreshGameConfirmation = false;
    void SetRestartPrompt(bool bOpen);
    void HandleInteract();
    void HandleNextRecipe();
    void HandleSetDown();

    UPROPERTY(Transient)
    TObjectPtr<UInputAction> InteractAction;

    UPROPERTY(Transient)
    TObjectPtr<UInputAction> NextRecipeAction;

    UPROPERTY(Transient)
    TObjectPtr<UInputAction> SetDownAction;

    UPROPERTY(Transient)
    TObjectPtr<UInputMappingContext> InteractionMappingContext;

};
