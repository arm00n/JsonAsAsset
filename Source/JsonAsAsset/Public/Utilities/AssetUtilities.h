// Copyright JAA Contributors 2024-2025

#pragma once

#include "AppStyleCompatibility.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"

class FAssetUtilities {
public:
	/*
	* Creates a UPackage to create assets in the Content Browser.
	* 
	* @return Package
	*/
	static UPackage* CreateAssetPackage(const FString& FullPath);
	static UPackage* CreateAssetPackage(const FString& Name, const FString& OutputPath);
	static UPackage* CreateAssetPackage(const FString& Name, const FString& OutputPath, UPackage*& OutOutermostPkg);
	
	/**
	 * Get the asset currently selected in the Content Browser.
	 * 
	 * @return Selected Asset
	 */
	template <typename T>
	static T* GetSelectedAsset() {
		const FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
		TArray<FAssetData> SelectedAssets;
		ContentBrowserModule.Get().GetSelectedAssets(SelectedAssets);

		if (SelectedAssets.Num() == 0) {
			GLog->Log("JsonAsAsset: [GetSelectedAsset] None selected, returning nullptr.");

			const FText DialogText = FText::Format(
				FText::FromString(TEXT("Importing an asset of type '{0}' requires a base asset selected. Select one in your content browser.")),
				FText::FromString(T::StaticClass()->GetName())
			);

			FMessageDialog::Open(EAppMsgType::Ok, DialogText);
			return nullptr;
		}

		UObject* SelectedAsset = SelectedAssets[0].GetAsset();
		T* CastedAsset = Cast<T>(SelectedAsset);

		if (!CastedAsset) {
			GLog->Log("JsonAsAsset: [GetSelectedAsset] Selected asset is not of the required class, returning nullptr.");

			const FText DialogText = FText::Format(
				FText::FromString(TEXT("The selected asset is not of type '{0}'. Please select a valid asset.")),
				FText::FromString(T::StaticClass()->GetName())
			);

			FMessageDialog::Open(EAppMsgType::Ok, DialogText);
			return nullptr;
		}

		return CastedAsset;
	}
	
public:
	template <class T = UObject>
	static bool ConstructAsset(const FString& Path, const FString& Type, TObjectPtr<T>& OutObject, bool& bSuccess);
	
	static bool Construct_TypeTexture(const FString& Path, const FString& RealPath, UTexture*& OutTexture);

	// Creates a plugin in the name (may result in bugs if inputted wrong)
	static void CreatePlugin(FString PluginName);

	static const TSharedPtr<FJsonObject> API_RequestExports(const FString& Path);
};
