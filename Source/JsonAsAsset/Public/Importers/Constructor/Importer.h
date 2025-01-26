﻿// Copyright JAA Contributors 2024-2025

#pragma once

#include "../../Utilities/PropertyUtilities.h"
#include "../../Utilities/ObjectUtilities.h"
#include "Utilities/AppStyleCompatibility.h"
#include "Utilities/EngineUtilities.h"
#include "Utilities/JsonUtilities.h"
#include "Dom/JsonObject.h"
#include "CoreMinimal.h"

extern TArray<FString> ImporterAcceptedTypes;

// Global handler for converting JSON to assets
class IImporter {
public:
    /* Constructors ---------------------------------------------------------------------- */
    IImporter()
        : Package(nullptr), OutermostPkg(nullptr),
          PropertySerializer(nullptr), GObjectSerializer(nullptr) {}

    //  Importer Constructor
    IImporter(const FString& FileName, const FString& FilePath, 
              const TSharedPtr<FJsonObject>& JsonObject, UPackage* Package, 
              UPackage* OutermostPkg, const TArray<TSharedPtr<FJsonValue>>& AllJsonObjects = {});

    virtual ~IImporter() {}

protected:
    /* Class variables ------------------------------------------------------------------ */
    TArray<TSharedPtr<FJsonValue>> AllJsonObjects;
    TSharedPtr<FJsonObject> JsonObject;
    FString FileName;
    FString FilePath;
    UPackage* Package;
    UPackage* OutermostPkg;

    /* ----------------------------------------------------------------------------------- */
    
public:
    /*
    * Overriden in child classes.
    * Returns false if failed.
    */
    virtual bool ImportData() {
        return false;
    }

public:
    /* Accepted Types ---------------------------------------------------------------------- */
    static TArray<FString> GetAcceptedTypes() {
        return ImporterAcceptedTypes;
    }

    static bool CanImport(const FString& ImporterType) {
        return ImporterAcceptedTypes.Contains(ImporterType)

        /* Added because we want to support as much sound classes as possible. SoundNode and SoundWave shouldn't be handled here */
        || ImporterType.StartsWith("Sound") && ImporterType != "SoundWave" && !ImporterType.StartsWith("SoundNode"); 
    }

    static bool CanImportAny(TArray<FString>& Types) {
        for (FString& Type : Types) {
            if (CanImport(Type)) return true;
        }
        return false;
    }

public:
    /* LoadObject functions ---------------------------------------------------------------------- */
    template<class T = UObject>
    void LoadObject(const TSharedPtr<FJsonObject>* PackageIndex, TObjectPtr<T>& Object);

    template<class T = UObject>
    TArray<TObjectPtr<T>> LoadObject(const TArray<TSharedPtr<FJsonValue>>& PackageArray, TArray<TObjectPtr<T>> Array);

    /* LoadObject functions ---------------------------------------------------------------------- */
public:
    void ImportReference(const FString& File);
    bool ImportAssetReference(const FString& GamePath);
    bool ImportExports(TArray<TSharedPtr<FJsonValue>> Exports, FString File, bool bHideNotifications = false);

public:
    TArray<TSharedPtr<FJsonValue>> GetObjectsWithTypeStartingWith(const FString& StartsWithStr);

    TSharedPtr<FJsonObject> GetExport(FJsonObject* PackageIndex);
    
protected:
    bool HandleAssetCreation(UObject* Asset) const;
    void SavePackage();

    TMap<FName, FExportData> CreateExports();

    // Handle edit changes, and add it to the content browser
    // Shortcut to calling SavePackage and HandleAssetCreation
    bool OnAssetCreation(UObject* Asset);

    template <class T = UObject>
    TObjectPtr<T> DownloadWrapper(TObjectPtr<T> InObject, FString Type, FString Name, FString Path);

    static FName GetExportNameOfSubobject(const FString& PackageIndex);
    TArray<TSharedPtr<FJsonValue>> FilterExportsByOuter(const FString& Outer);
    TSharedPtr<FJsonValue> GetExportByObjectPath(const TSharedPtr<FJsonObject>& Object);
public:
    /* ------------------------------------ Object Serializer and Property Serializer ------------------------------------ */
    FORCEINLINE UObjectSerializer* GetObjectSerializer() const { return GObjectSerializer; }

protected:
    UPropertySerializer* PropertySerializer;
    UObjectSerializer* GObjectSerializer;
    /* ------------------------------------ Object Serializer and Property Serializer ------------------------------------ */
};