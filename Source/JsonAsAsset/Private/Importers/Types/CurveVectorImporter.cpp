﻿// Copyright JAA Contributors 2024-2025

#include "Importers/Types/CurveVectorImporter.h"
#include "Utilities/MathUtilities.h"
#include "Factories/CurveFactory.h"
#include "Curves/CurveVector.h"

bool ICurveVectorImporter::ImportData() {
	// Array of containers
	TArray<TSharedPtr<FJsonValue>> FloatCurves = JsonObject->GetArrayField("FloatCurves");

	UCurveVectorFactory* CurveVectorFactory = NewObject<UCurveVectorFactory>();
	UCurveVector* CurveVectorAsset = Cast<UCurveVector>(CurveVectorFactory->FactoryCreateNew(UCurveVector::StaticClass(), OutermostPkg, *FileName, RF_Standalone | RF_Public, nullptr, GWarn));

	// for each container, get keys
	for (int i = 0; i < FloatCurves.Num(); i++) {
		TArray<TSharedPtr<FJsonValue>> Keys = FloatCurves[i]->AsObject()->GetArrayField("Keys");
		CurveVectorAsset->FloatCurves[i].Keys.Empty();

		// add keys to array
		for (int j = 0; j < Keys.Num(); j++) {
			CurveVectorAsset->FloatCurves[i].Keys.Add(FMathUtilities::ObjectToRichCurveKey(Keys[j]->AsObject()));
		}
	}

	// Handle edit changes, and add it to the content browser
	return OnAssetCreation(CurveVectorAsset);
}