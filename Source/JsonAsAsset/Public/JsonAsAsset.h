// Copyright JAA Contributors 2024-2025

#pragma once

#include "CoreMinimal.h"

#if ENGINE_MAJOR_VERSION == 4
#include "Modules/ModuleInterface.h"
#endif

class UJsonAsAssetSettings;

class FJsonAsAssetModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    // Executes File Dialog
    void PluginButtonClicked();

private:
    void RegisterMenus();

    TSharedPtr<FUICommandList> PluginCommands;
    TSharedRef<SWidget> CreateToolbarDropdown();
    void CreateLocalFetchDropdown(FMenuBuilder MenuBuilder);

    bool bActionRequired;
    const UJsonAsAssetSettings* Settings;

    // Creates a dialog for a file
    TArray<FString> OpenFileDialog(FString Title, FString Type);
    bool IsProcessRunning(const FString& ProcessName);

#if ENGINE_MAJOR_VERSION == 4
    void AddToolbarExtension(FToolBarBuilder& Builder);
#endif
};