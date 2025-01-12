// Copyright JAA Contributors 2024-2025

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Engine/DeveloperSettings.h"
#include "Fetch/LocalFetch.h"
#include "JsonAsAssetSettings.generated.h"

// Settings for materials
USTRUCT()
struct FMaterialImportSettings
{
	GENERATED_BODY()
public:
	// Constructor to initialize default values
	FMaterialImportSettings()
		: bSkipResultNodeConnection(false)
	{}

	/**
	* When importing/downloading the asset type Material, a error may occur
	* | Material expression called Compiler->TextureParameter() without implementing UMaterialExpression::GetReferencedTexture properly
	*
	* To counter that error, we skip connecting the inputs to the main result
	* node in the material. If you do use this, import a material, save everything,
	* restart, and re-import the material without any problems with this turned off/on.
	*
	* (or you could just connect them yourself)
	*/
	UPROPERTY(EditAnywhere, Config, Category = "Material Import Settings")
	bool bSkipResultNodeConnection;
};

// Settings for sounds
USTRUCT()
struct FSoundImportSettings
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, Config, Category = "Sound Import Settings", meta = (DisplayName = "Audio File Name Extension"))
	FString AudioFileExtension = "ogg";
};

USTRUCT()
struct FAssetSettings
{
	GENERATED_BODY()
public:
	// Constructor to initialize default values
	FAssetSettings()
		: bSavePackagesOnImport(false)
	{
		MaterialImportSettings = FMaterialImportSettings();
		SoundImportSettings = FSoundImportSettings();
	}
	
	UPROPERTY(EditAnywhere, Config, Category = "Asset Settings")
	FMaterialImportSettings MaterialImportSettings;

	UPROPERTY(EditAnywhere, Config, Category = "Asset Settings")
	FSoundImportSettings SoundImportSettings;

	UPROPERTY(EditAnywhere, Config, Category = "Asset Settings", meta = (DisplayName = "Save Assets On Import"))
	bool bSavePackagesOnImport;

	/**
	* Not needed for normal operations, needed for older versions of game builds.
	*/
	UPROPERTY(EditAnywhere, Config, Category = "Asset Settings")
	FString GameName;
};

// A user-friendly Unreal Engine plugin designed to import assets from packaged games through JSON files
UCLASS(Config = EditorPerProjectUserSettings, DefaultConfig)
class UJsonAsAssetSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UJsonAsAssetSettings();

#if WITH_EDITOR
	virtual FText GetSectionText() const override;
#endif

	/**
	 * Directory path for exporting FModel assets.
	 * (Output/Exports)
	 *
	 * NOTE: Use the file selector to choose a directory. 
	 *       Avoid manually entering the path or replacing "\" with "/". 
	 *       The file selector ensures proper formatting.
	*/
	UPROPERTY(EditAnywhere, Config, Category = "Configuration")
	FDirectoryPath ExportDirectory;

	UPROPERTY(EditAnywhere, Config, Category = "Configuration")
	FAssetSettings AssetSettings;

	/**
	 * Fetches assets from a local hosted API and imports them directly into your project.
	 * 
	 * NOTE: Ensure all settings are correctly configured before starting Local Fetch. 
	 *       Please refer to the README.md file.
	 */
	UPROPERTY(EditAnywhere, Config, Category = "Local Fetch")
	bool bEnableLocalFetch;

	/**
	 * Location of the Paks folder containing all the assets.
	 * (Content/Paks)
	 * 
	 * NOTE: Use the file selector to choose the folder location. 
	 *       Avoid manually pasting the path or replacing "\" with "/".
	 */
	UPROPERTY(EditAnywhere, Config, Category = "Local Fetch - Configuration", meta=(EditCondition="bEnableLocalFetch"))
	FDirectoryPath ArchiveDirectory;

	// UE Version for the Unreal Engine Game (same as FModel's UE Verisons property)
	UPROPERTY(EditAnywhere, Config, Category = "Local Fetch - Configuration", meta=(EditCondition="bEnableLocalFetch"))
	TEnumAsByte<ECUE4ParseVersion> UnrealVersion;

	UFUNCTION(CallInEditor)
	static TArray<FString> GetUnrealVersions();

	/*
	 * Path to the mappings file.
	 *
	 * NOTE: Use the file selector to choose the file location. 
	 *        Avoid manually pasting the path.
	 */
	UPROPERTY(EditAnywhere, Config, Category = "Local Fetch - Configuration", meta=(EditCondition="bEnableLocalFetch", FilePathFilter="usmap", RelativeToGameDir))
	FFilePath MappingFilePath;

	UPROPERTY(EditAnywhere, Config, Category = "Local Fetch - Encryption", meta=(EditCondition="bEnableLocalFetch"), AdvancedDisplay)
	bool bDownloadExistingTextures;

	// Main key for archives
	UPROPERTY(EditAnywhere, Config, Category = "Local Fetch - Encryption", meta=(EditCondition="bEnableLocalFetch", DisplayName="Archive Key"))
	FString ArchiveKey = "0x00000000000000000000000000000000000000000000000000000000000";

	// AES Keys
	UPROPERTY(EditAnywhere, Config, Category = "Local Fetch - Encryption", meta=(EditCondition="bEnableLocalFetch", DisplayName="Dynamic Keys"))
	TArray<FAesKey> DynamicKeys;

	// Enables the option to change the api's URL
	UPROPERTY(EditAnywhere, Config, Category = "Local Fetch", meta = (EditCondition = "bEnableLocalFetch"), AdvancedDisplay)
	bool bChangeURL;

	// "http://localhost:1500" is default
	UPROPERTY(EditAnywhere, Config, Category = "Local Fetch", meta=(EditCondition="bChangeURL && bEnableLocalFetch", DisplayName = "Local URL"), AdvancedDisplay)
	FString Url = "http://localhost:1500";
};
