// Copyright JAA Contributors 2024-2025

#include "Importers/Types/CurveFloatImporter.h"
#include "Utilities/MathUtilities.h"
#include "Factories/CurveFactory.h"
#include "Dom/JsonObject.h"

bool ICurveFloatImporter::ImportData() {
	// Quick way to access the curve keys
	TArray<TSharedPtr<FJsonValue>> Keys = JsonObject->GetObjectField("Properties")->GetObjectField("FloatCurve")->GetArrayField("Keys");

	UCurveFloatFactory* CurveFactory = NewObject<UCurveFloatFactory>();
	UCurveFloat* CurveAsset = Cast<UCurveFloat>(CurveFactory->FactoryCreateNew(UCurveFloat::StaticClass(), OutermostPkg, *FileName, RF_Standalone | RF_Public, nullptr, GWarn));

	// Add Rich Keys
	for (TSharedPtr<FJsonValue>& Key : Keys) {
		CurveAsset->FloatCurve.Keys.Add(FMathUtilities::ObjectToRichCurveKey(Key->AsObject()));
	}

	// Handle edit changes, and add it to the content browser
	return OnAssetCreation(CurveAsset);
}