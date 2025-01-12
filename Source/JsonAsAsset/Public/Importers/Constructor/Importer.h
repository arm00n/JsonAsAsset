// Copyright JAA Contributors 2024-2025

#pragma once

#include "CoreMinimal.h"
#include "ContentBrowserModule.h"
#include "Dom/JsonObject.h"
#include "../../Utilities/ObjectUtilities.h"
#include "../../Utilities/PropertyUtilities.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Utilities/AppStyleCompatibility.h"
#include "IContentBrowserSingleton.h"
#include "Widgets/Notifications/SNotificationList.h"

extern TArray<FString> ImporterAcceptedTypes;

struct FExportData {
    FExportData(const FName Type, const FName Outer, const TSharedPtr<FJsonObject>& Json) {
        this->Type = Type;
        this->Outer = Outer;
        this->Json = Json.Get();
    }

    FExportData(const FString& Type, const FString& Outer, const TSharedPtr<FJsonObject>& Json) {
        this->Type = FName(Type);
        this->Outer = FName(Outer);
        this->Json = Json.Get();
    }

    FExportData(const FString& Type, const FString& Outer, FJsonObject* Json) {
        this->Type = FName(Type);
        this->Outer = FName(Outer);
        this->Json = Json;
    }

    FName Type;
    FName Outer;
    FJsonObject* Json;
};

// Global handler for converting JSON to assets
class IImporter {
public:
    IImporter()
        : PropertySerializer(nullptr), GObjectSerializer(nullptr),
          Package(nullptr), OutermostPkg(nullptr) {}

    IImporter(const FString& FileName, const FString& FilePath, 
              const TSharedPtr<FJsonObject>& JsonObject, UPackage* Package, 
              UPackage* OutermostPkg, const TArray<TSharedPtr<FJsonValue>>& AllJsonObjects = {});

    virtual ~IImporter() {}

    // Import the data of the supported type, return if successful or not
    virtual bool ImportData() { return false; }

protected:
    UPropertySerializer* PropertySerializer;
    UObjectSerializer* GObjectSerializer;

public:
    static TArray<FString> GetAcceptedTypes() {
        return ImporterAcceptedTypes;
    }

    template <class T = UObject>
    void LoadObject(const TSharedPtr<FJsonObject>* PackageIndex, TObjectPtr<T>& Object);

    template <class T = UObject>
    TArray<TObjectPtr<T>> LoadObject(const TArray<TSharedPtr<FJsonValue>>& PackageArray, TArray<TObjectPtr<T>> Array);

    static bool CanImport(const FString& ImporterType) {
        return ImporterAcceptedTypes.Contains(ImporterType)
        || ImporterType.StartsWith("Sound") && ImporterType != "SoundWave" && !ImporterType.StartsWith("SoundNode"); 
    }

    static bool CanImportAny(TArray<FString>& Types) {
        for (FString& Type : Types) {
            if (CanImport(Type)) return true;
        }
        return false;
    }

    void ImportReference(const FString& File);
    bool ImportAssetReference(const FString& GamePath);
    bool ImportExports(TArray<TSharedPtr<FJsonValue>> Exports, FString File, bool bHideNotifications = false);

    TArray<TSharedPtr<FJsonValue>> GetObjectsWithTypeStartingWith(const FString& StartsWithStr) {
        TArray<TSharedPtr<FJsonValue>> FilteredObjects;

        for (const TSharedPtr<FJsonValue>& JsonObjectValue : AllJsonObjects) {
            if (JsonObjectValue->Type == EJson::Object) {
                TSharedPtr<FJsonObject> JsonObjectType = JsonObjectValue->AsObject();

                if (JsonObjectType.IsValid() && JsonObjectType->HasField("Type")) {
                    FString TypeValue = JsonObjectType->GetStringField("Type");

                    // Check if the "Type" field starts with the specified string
                    if (TypeValue.StartsWith(StartsWithStr)) {
                        FilteredObjects.Add(JsonObjectValue);
                    }
                }
            }
        }

        return FilteredObjects;
    }

    TArray<FAssetData> GetAssetsInSelectedFolder() {
        TArray<FAssetData> AssetDataList;

        // Get the Content Browser Module
        FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

        TArray<FString> SelectedFolders;
        ContentBrowserModule.Get().GetSelectedPathViewFolders(SelectedFolders);

        if (SelectedFolders.Num() == 0) {
            return AssetDataList;
        }

        FString CurrentFolder = SelectedFolders[0];

        // Check if the folder is the root folder
        if (CurrentFolder == "/Game") {
            return AssetDataList; // Return empty array to prevent further actions
        }

        // Get the Asset Registry Module
        FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
        AssetRegistryModule.Get().SearchAllAssets(true);

        // Get all assets in the folder and its subfolders
        AssetRegistryModule.Get().GetAssetsByPath(FName(*CurrentFolder), AssetDataList, true);

        return AssetDataList;
    }

    TSharedPtr<FJsonObject> GetExport(FJsonObject* PackageIndex);

    // Notification Functions
    virtual void AppendNotification(const FText& Text, const FText& SubText, float ExpireDuration, SNotificationItem::ECompletionState CompletionState, bool bUseSuccessFailIcons = false, float WidthOverride = 500);
    virtual void AppendNotification(const FText& Text, const FText& SubText, float ExpireDuration, const FSlateBrush* SlateBrush, SNotificationItem::ECompletionState CompletionState, bool bUseSuccessFailIcons = false, float WidthOverride = 500);

    TSharedPtr<FJsonObject> RemovePropertiesShared(TSharedPtr<FJsonObject> Input, TArray<FString> RemovedProperties) const;
    TSharedPtr<FJsonObject> KeepPropertiesShared(TSharedPtr<FJsonObject> Input, TArray<FString> WhitelistProperties) const;

protected:
    bool HandleAssetCreation(UObject* Asset) const;
    void SavePackage();

    // Simple handler for JsonArray
    void ProcessJsonArrayField(const TSharedPtr<FJsonObject>& ObjectField, const FString& ArrayFieldName, TFunction<void(const TSharedPtr<FJsonObject>&)> ProcessObjectFunction) {
        const TArray<TSharedPtr<FJsonValue>>* JsonArray;
        if (ObjectField->TryGetArrayField(ArrayFieldName, JsonArray)) {
            for (const auto& JsonValue : *JsonArray) {
                if (const TSharedPtr<FJsonObject> JsonItem = JsonValue->AsObject()) {
                    ProcessObjectFunction(JsonItem);
                }
            }
        }
    }

    TMap<FName, FExportData> CreateExports();

    // Handle edit changes, and add it to the content browser
    // Shortcut to calling SavePackage and HandleAssetCreation
    bool OnAssetCreation(UObject* Asset);

    template <class T = UObject>
    TObjectPtr<T> DownloadWrapper(TObjectPtr<T> InObject, FString Type, FString Name, FString Path);

    static FName GetExportNameOfSubobject(const FString& PackageIndex);
    TArray<TSharedPtr<FJsonValue>> FilterExportsByOuter(const FString& Outer);
    TSharedPtr<FJsonValue> GetExportByObjectPath(const TSharedPtr<FJsonObject>& Object);

    FORCEINLINE UObjectSerializer* GetObjectSerializer() const { return GObjectSerializer; }

    FString FileName;
    FString FilePath;
    TSharedPtr<FJsonObject> JsonObject;
    UPackage* Package;
    UPackage* OutermostPkg;

    TArray<TSharedPtr<FJsonValue>> AllJsonObjects;
};