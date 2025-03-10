// Copyright JAA Contributors 2024-2025

#include "Modules/UI/StyleModule.h"
#include "Styling/SlateStyleRegistry.h"
#include "Interfaces/IPluginManager.h"

TSharedPtr<FSlateStyleSet> FJsonAsAssetStyle::StyleInstance = nullptr;

void FJsonAsAssetStyle::Initialize() {
	if (!StyleInstance.IsValid()) {
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FJsonAsAssetStyle::Shutdown() {
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FJsonAsAssetStyle::GetStyleSetName() {
	static FName StyleSetName(TEXT("JsonAsAssetStyle"));
	return StyleSetName;
}

#define IMAGE_BRUSH(RelativePath, ...) FSlateImageBrush(Style->RootToContentDir(RelativePath, TEXT(".png")), __VA_ARGS__)

const FVector2D Icon40x40(40, 40);
const FVector2D Icon80x80(40, 40);

TSharedRef<FSlateStyleSet> FJsonAsAssetStyle::Create() {
	TSharedRef<FSlateStyleSet> Style = MakeShareable(new FSlateStyleSet("JsonAsAssetStyle"));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("JsonAsAsset")->GetBaseDir() / TEXT("Resources"));

	Style->Set("JsonAsAsset.Logo", new IMAGE_BRUSH(TEXT("Icon40"), Icon40x40));
	Style->Set("JsonAsAsset.WarningLogo", new IMAGE_BRUSH(TEXT("WarningIcon40"), Icon40x40));
	Style->Set("JsonAsAsset.FModelLogo", new IMAGE_BRUSH(TEXT("FModelButtonIcon"), Icon80x80));
	Style->Set("JsonAsAsset.GithubLogo", new IMAGE_BRUSH(TEXT("GithubLogo"), Icon40x40));

	return Style;
}

const ISlateStyle& FJsonAsAssetStyle::Get() {
	return *StyleInstance;
}

#undef IMAGE_BRUSH

void FJsonAsAssetStyle::ReloadTextures() {
	if (FSlateApplication::IsInitialized()) {
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}