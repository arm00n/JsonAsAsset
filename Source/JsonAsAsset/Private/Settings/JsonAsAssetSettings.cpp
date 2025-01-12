// Copyright JAA Contributors 2024-2025

#include "JsonAsAssetSettings.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SBoxPanel.h"
#include "Modules/LocalFetch.h"

#define LOCTEXT_NAMESPACE "JsonAsAsset"

UJsonAsAssetSettings::UJsonAsAssetSettings():
	// Default Initializers
	bEnableLocalFetch(false), UnrealVersion(),
	bDownloadExistingTextures(false), bChangeURL(false)
{
	CategoryName = TEXT("Plugins");
	SectionName = TEXT("JsonAsAsset");
}

FText UJsonAsAssetSettings::GetSectionText() const
{
	return LOCTEXT("SettingsDisplayName", "JsonAsAsset");
}

#if WITH_EDITOR
// This displays the Unreal Versions in settings
TArray<FString> UJsonAsAssetSettings::GetUnrealVersions()
{
	TArray<FString> EnumNames;

	for (int Version = GAME_UE4_0; Version <= GAME_UE5_LATEST; ++Version)
	{
		EnumNames.Add(StaticEnum<ECUE4ParseVersion>()->GetNameStringByIndex(Version));
	}

	return EnumNames;
}

#endif
#undef LOCTEXT_NAMESPACE