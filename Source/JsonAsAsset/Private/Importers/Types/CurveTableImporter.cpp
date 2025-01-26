// Copyright JAA Contributors 2024-2025

#include "Importers/Types/CurveTableImporter.h"
#include "Dom/JsonObject.h"

// Unfortunately these variables are privated, so we had to make a "bypass" by making
// an asset then casting to subclass that has these functions to modify them.
void CCurveTableDerived::AddRow(FName Name, FRealCurve* Curve) {
	RowMap.Add(Name, Curve);
}

void CCurveTableDerived::ChangeTableMode(ECurveTableMode Mode) {
	CurveTableMode = Mode;
}

bool ICurveTableImporter::ImportData() {
	TSharedPtr<FJsonObject> RowData = JsonObject->GetObjectField("Rows");
	UCurveTable* CurveTable = NewObject<UCurveTable>(Package, UCurveTable::StaticClass(), *FileName, RF_Public | RF_Standalone);
	CCurveTableDerived* DerivedCurveTable = Cast<CCurveTableDerived>(CurveTable);

	// Used to determine curve type
	ECurveTableMode CurveTableMode = ECurveTableMode::RichCurves; {
		FString CurveMode;
		
		if (JsonObject->TryGetStringField("CurveTableMode", CurveMode))
			CurveTableMode = static_cast<ECurveTableMode>(StaticEnum<ECurveTableMode>()->GetValueByNameString(CurveMode));

		DerivedCurveTable->ChangeTableMode(CurveTableMode);
	}

	// Loop throughout row data, and deserialize
	for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : RowData->Values) {
		const TSharedPtr<FJsonObject> CurveData = Pair.Value->AsObject();

		// Curve structure (either simple or rich)
		FRealCurve RealCurve;

		if (CurveTableMode == ECurveTableMode::RichCurves) {
			FRichCurve& NewRichCurve = CurveTable->AddRichCurve(FName(*Pair.Key)); {
				RealCurve = NewRichCurve;
			}

			const TArray<TSharedPtr<FJsonValue>>* KeysPtr;
			if (CurveData->TryGetArrayField("Keys", KeysPtr))
				for (const TSharedPtr<FJsonValue> KeyPtr : *KeysPtr) {
					TSharedPtr<FJsonObject> Key = KeyPtr->AsObject(); {
						NewRichCurve.AddKey(Key->GetNumberField("Time"), Key->GetNumberField("Value"));
						FRichCurveKey RichKey = NewRichCurve.Keys.Last();

						RichKey.InterpMode =
							static_cast<ERichCurveInterpMode>(
								StaticEnum<ERichCurveInterpMode>()->GetValueByNameString(Key->GetStringField("InterpMode"))
							);
						RichKey.TangentMode =
							static_cast<ERichCurveTangentMode>(
								StaticEnum<ERichCurveTangentMode>()->GetValueByNameString(Key->GetStringField("TangentMode"))
							);
						RichKey.TangentWeightMode =
							static_cast<ERichCurveTangentWeightMode>(
								StaticEnum<ERichCurveTangentWeightMode>()->GetValueByNameString(Key->GetStringField("TangentWeightMode"))
							);

						RichKey.ArriveTangent = Key->GetNumberField("ArriveTangent");
						RichKey.ArriveTangentWeight = Key->GetNumberField("ArriveTangentWeight");
						RichKey.LeaveTangent = Key->GetNumberField("LeaveTangent");
						RichKey.LeaveTangentWeight = Key->GetNumberField("LeaveTangentWeight");
					}
				}
		} else {
			FSimpleCurve& NewSimpleCurve = CurveTable->AddSimpleCurve(FName(*Pair.Key)); {
				RealCurve = NewSimpleCurve;
			}

			// Method of Interpolation
			NewSimpleCurve.InterpMode =
				static_cast<ERichCurveInterpMode>(
					StaticEnum<ERichCurveInterpMode>()->GetValueByNameString(CurveData->GetStringField("InterpMode"))
				);

			const TArray<TSharedPtr<FJsonValue>>* KeysPtr;
			
			if (CurveData->TryGetArrayField("Keys", KeysPtr))
				for (const TSharedPtr<FJsonValue> KeyPtr : *KeysPtr) {
					TSharedPtr<FJsonObject> Key = KeyPtr->AsObject(); {
						NewSimpleCurve.AddKey(Key->GetNumberField("Time"), Key->GetNumberField("Value"));
					}
				}
		}

		// Inherited data from FRealCurve
		RealCurve.SetDefaultValue(CurveData->GetNumberField("DefaultValue"));
		RealCurve.PreInfinityExtrap = 
			static_cast<ERichCurveExtrapolation>(
				StaticEnum<ERichCurveExtrapolation>()->GetValueByNameString(CurveData->GetStringField("PreInfinityExtrap"))
			);
		RealCurve.PostInfinityExtrap =
			static_cast<ERichCurveExtrapolation>(
				StaticEnum<ERichCurveExtrapolation>()->GetValueByNameString(CurveData->GetStringField("PostInfinityExtrap"))
			);

		// Update Curve Table
		CurveTable->OnCurveTableChanged().Broadcast();
		CurveTable->Modify(true);
	}

	// Handle edit changes, and add it to the content browser
	return OnAssetCreation(CurveTable);
}