// Copyright JAA Contributors 2024-2025

#include "Importers/Types/CurveLinearColorAtlasImporter.h"
#include "Importers/Types/TextureImporter.h"
#include "Curves/CurveLinearColor.h"
#include "JsonGlobals.h"
#include "Curves/CurveLinearColorAtlas.h"
#include "Dom/JsonObject.h"

bool UCurveLinearColorAtlasImporter::ImportData() {
	try {
		TSharedPtr<FJsonObject> Properties = JsonObject->GetObjectField("Properties");
		
		float Width = 256;
		float Height = 256;

		UCurveLinearColorAtlas* Object = NewObject<UCurveLinearColorAtlas>(Package, UCurveLinearColorAtlas::StaticClass(), *FileName, RF_Public | RF_Standalone);
		Object->Source.Init(Width, Height, 1, 1, TSF_RGBA16F);
		const int32 TextureDataSize = Object->Source.CalcMipSize(0);
		Object->SrcData.AddUninitialized(TextureDataSize);
		uint32* TextureData = (uint32*)Object->Source.LockMip(0);
		FFloat16Color InitColor(FLinearColor::White);
		for (uint32 y = 0; y < Object->TextureSize; y++) {
			for (uint32 x = 0; x < Object->TextureSize; x++) {
				Object->SrcData[x * Object->TextureSize + y] = InitColor;
			}
		}
		FMemory::Memcpy(TextureData, Object->SrcData.GetData(), TextureDataSize);
		Object->Source.UnlockMip(0);

		Object->UpdateResource();

		bool bHasAnyDirtyTextures = false;
		if (Properties->TryGetBoolField("bHasAnyDirtyTextures", bHasAnyDirtyTextures))
		{
			Object->bHasAnyDirtyTextures = bHasAnyDirtyTextures;
		}

		bool bIsDirty = false;
		if (Properties->TryGetBoolField("bIsDirty", bIsDirty))
		{
			Object->bIsDirty = bIsDirty;
		}

		bool bShowDebugColorsForNullGradients = false;
		if (Properties->TryGetBoolField("bShowDebugColorsForNullGradients", bShowDebugColorsForNullGradients))
		{
			Object->bShowDebugColorsForNullGradients = bShowDebugColorsForNullGradients;
		}

		bool bSquareResolution = false;
		if (Properties->TryGetBoolField("bSquareResolution", bSquareResolution))
		{
			Object->bSquareResolution = bSquareResolution;
		}

#if ENGINE_MAJOR_VERSION == 5
		float TextureSize = 0.0f;
		float TextureHeight = 0.0f;
#else
		double TextureSize = 0.0f;
		double TextureHeight = 0.0f;
#endif
		
		if (Properties->TryGetNumberField("TextureSize", TextureSize))
		{
			Object->TextureSize = TextureSize;
		}

		if (Properties->TryGetNumberField("TextureHeight", TextureHeight))
		{
			Object->TextureHeight = TextureHeight;
		}

		FProperty* TextureSizeProperty = FindFProperty<FProperty>(Object->GetClass(), "TextureSize");
		FPropertyChangedEvent TextureSizePropertyPropertyChangedEvent(TextureSizeProperty, EPropertyChangeType::ValueSet);
		Object->PostEditChangeProperty(TextureSizePropertyPropertyChangedEvent);

		// Add gradient curves
		FProperty* GradientCurvesProperty = FindFProperty<FProperty>(Object->GetClass(), "GradientCurves");
		FPropertyChangedEvent PropertyChangedEvent(GradientCurvesProperty, EPropertyChangeType::ArrayAdd);

		const TArray<TSharedPtr<FJsonValue>> GradientCurves = Properties->GetArrayField("GradientCurves");
		TArray<TObjectPtr<UCurveLinearColor>> CurvesLocal;

		CurvesLocal = LoadObject(GradientCurves, CurvesLocal);
		Object->GradientCurves = CurvesLocal;
		Object->PostEditChangeProperty(PropertyChangedEvent);

		// Handle edit changes, and add it to the content browser
		return OnAssetCreation(Object);
	} catch (const char* Exception) {
		UE_LOG(LogJson, Error, TEXT("%s"), *FString(Exception));
	}

	return false;
}