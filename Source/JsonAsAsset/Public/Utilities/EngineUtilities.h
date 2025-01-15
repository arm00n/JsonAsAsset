// Copyright JAA Contributors 2024-2025

#pragma once

#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "IContentBrowserSingleton.h"
#include "ContentBrowserModule.h"
#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "Interfaces/IMainFrameModule.h"

// Gets all assets in selected folder
inline TArray<FAssetData> GetAssetsInSelectedFolder() {
	TArray<FAssetData> AssetDataList;

	// Get the Content Browser Module
	const FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

	TArray<FString> SelectedFolders;
	ContentBrowserModule.Get().GetSelectedPathViewFolders(SelectedFolders);

	if (SelectedFolders.Num() == 0) {
		return AssetDataList;
	}

	const auto CurrentFolder = SelectedFolders[0];

	// Check if the folder is the root folder
	if (CurrentFolder == "/Game") {
		return AssetDataList;
	}

	// Get the Asset Registry Module
	const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	AssetRegistryModule.Get().SearchAllAssets(true);

	// Get all assets in the folder and its subfolders
	AssetRegistryModule.Get().GetAssetsByPath(FName(*CurrentFolder), AssetDataList, true);

	return AssetDataList;
}

inline TArray<FString> OpenFolderDialog(FString Title) {
	TArray<FString> ReturnValue;

	void* ParentWindowHandle = nullptr;

	IMainFrameModule& MainFrameModule = IMainFrameModule::Get();
	TSharedPtr<SWindow> MainWindow = MainFrameModule.GetParentWindow();

	if (MainWindow.IsValid() && MainWindow->GetNativeWindow().IsValid()) {
		ParentWindowHandle = MainWindow->GetNativeWindow()->GetOSWindowHandle();
	}

	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (DesktopPlatform) {
		FString SelectedFolder;

		// Open Folder Dialog
		if (DesktopPlatform->OpenDirectoryDialog(ParentWindowHandle, Title, FString(""), SelectedFolder)) {
			ReturnValue.Add(SelectedFolder);
		}
	}

	return ReturnValue;
}

// Filter to remove
inline TSharedPtr<FJsonObject> RemovePropertiesShared(TSharedPtr<FJsonObject> Input, TArray<FString> RemovedProperties) {
	const TSharedPtr<FJsonObject> RawSharedPtrData = TSharedPtr<FJsonObject>(Input);
	
	for (FString Property : RemovedProperties) {
		if (RawSharedPtrData->HasField(Property))
			RawSharedPtrData->RemoveField(Property);
	}

	return RawSharedPtrData;
}

// Filter to whitelist
inline TSharedPtr<FJsonObject> KeepPropertiesShared(const TSharedPtr<FJsonObject>& Input, TArray<FString> WhitelistProperties) {
	const TSharedPtr<FJsonObject> RawSharedPtrData = MakeShared<FJsonObject>();

	for (const FString& Property : WhitelistProperties) {
		if (Input->HasField(Property)) {
			RawSharedPtrData->SetField(Property, Input->TryGetField(Property));
		}
	}

	return RawSharedPtrData;
}

// Simple handler for JsonArray
inline auto ProcessJsonArrayField(const TSharedPtr<FJsonObject>& ObjectField, const FString& ArrayFieldName,
                                  const TFunction<void(const TSharedPtr<FJsonObject>&)>& ProcessObjectFunction) -> void
{
	const TArray<TSharedPtr<FJsonValue>>* JsonArray;
	
	if (ObjectField->TryGetArrayField(ArrayFieldName, JsonArray)) {
		for (const auto& JsonValue : *JsonArray) {
			if (const TSharedPtr<FJsonObject> JsonItem = JsonValue->AsObject()) {
				ProcessObjectFunction(JsonItem);
			}
		}
	}
}

// Show the user a Notification
inline auto AppendNotification(const FText& Text, const FText& SubText, const float ExpireDuration,
                               const SNotificationItem::ECompletionState CompletionState, const bool bUseSuccessFailIcons,
                               const float WidthOverride) -> void
{
	FNotificationInfo Info = FNotificationInfo(Text);
	Info.ExpireDuration = ExpireDuration;
	Info.bUseLargeFont = true;
	Info.bUseSuccessFailIcons = bUseSuccessFailIcons;
	Info.WidthOverride = FOptionalSize(WidthOverride);

#if ENGINE_MAJOR_VERSION >= 5
	Info.SubText = SubText;
#endif

	const TSharedPtr<SNotificationItem> NotificationPtr = FSlateNotificationManager::Get().AddNotification(Info);
	NotificationPtr->SetCompletionState(CompletionState);
}

// Show the user a Notification with Subtext
inline auto AppendNotification(const FText& Text, const FText& SubText, float ExpireDuration,
                               const FSlateBrush* SlateBrush, SNotificationItem::ECompletionState CompletionState,
                               const bool bUseSuccessFailIcons, const float WidthOverride) -> void
{
	FNotificationInfo Info = FNotificationInfo(Text);
	Info.ExpireDuration = ExpireDuration;
	Info.bUseLargeFont = true;
	Info.bUseSuccessFailIcons = bUseSuccessFailIcons;
	Info.WidthOverride = FOptionalSize(WidthOverride);
#if ENGINE_MAJOR_VERSION >= 5
	Info.SubText = SubText;
#endif
	Info.Image = SlateBrush;

	const TSharedPtr<SNotificationItem> NotificationPtr = FSlateNotificationManager::Get().AddNotification(Info);
	NotificationPtr->SetCompletionState(CompletionState);
}