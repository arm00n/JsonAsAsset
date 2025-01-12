// Copyright JAA Contributors 2024-2025

#include "Importers/Types/CurveLinearColorImporter.h"
#include "Utilities/MathUtilities.h"
#include "Curves/CurveLinearColor.h"
#include "Factories/CurveFactory.h"
#include "UObject/SavePackage.h"

bool ICurveLinearColorImporter::ImportData() {
	// Array of containers
	TArray<TSharedPtr<FJsonValue>> FloatCurves = JsonObject->GetArrayField("FloatCurves");

	UCurveLinearColorFactory* CurveFactory = NewObject<UCurveLinearColorFactory>();
	UCurveLinearColor* LinearCurveAsset = Cast<UCurveLinearColor>(CurveFactory->FactoryCreateNew(UCurveLinearColor::StaticClass(), OutermostPkg, *FileName, RF_Standalone | RF_Public, nullptr, GWarn));

	// for each container, get keys
	for (int i = 0; i < FloatCurves.Num(); i++) {
		TArray<TSharedPtr<FJsonValue>> Keys = FloatCurves[i]->AsObject()->GetArrayField("Keys");
		LinearCurveAsset->FloatCurves[i].Keys.Empty();

		// add keys to array
		for (int j = 0; j < Keys.Num(); j++) {
			LinearCurveAsset->FloatCurves[i].Keys.Add(FMathUtilities::ObjectToRichCurveKey(Keys[j]->AsObject()));
		}
	}

	return OnAssetCreation(LinearCurveAsset);
}
