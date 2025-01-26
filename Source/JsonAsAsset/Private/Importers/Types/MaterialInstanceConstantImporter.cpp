// Copyright JAA Contributors 2024-2025

#include "Importers/Types/MaterialInstanceConstantImporter.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Utilities/MathUtilities.h"
#include "Dom/JsonObject.h"
#include "RHIDefinitions.h"
#include "MaterialShared.h"

bool IMaterialInstanceConstantImporter::ImportData() {
	TSharedPtr<FJsonObject> Properties = JsonObject->GetObjectField("Properties");

	UMaterialInstanceConstant* MaterialInstanceConstant = NewObject<UMaterialInstanceConstant>(Package, UMaterialInstanceConstant::StaticClass(), *FileName, RF_Public | RF_Standalone);
	HandleAssetCreation(MaterialInstanceConstant);

	TArray<TSharedPtr<FJsonObject>> EditorOnlyData;
	GetObjectSerializer()->DeserializeObjectProperties(Properties, MaterialInstanceConstant);

	for (const TSharedPtr<FJsonValue> Value : AllJsonObjects) {
		TSharedPtr<FJsonObject> Object = TSharedPtr<FJsonObject>(Value->AsObject());

		if (Object->GetStringField("Type") == "MaterialInstanceEditorOnlyData") {
			EditorOnlyData.Add(Object);
		}
	}

	const TSharedPtr<FJsonObject>* ParentPtr;
	if (Properties->TryGetObjectField("Parent", ParentPtr)) {
#if ENGINE_MAJOR_VERSION >= 5
		LoadObject(ParentPtr, MaterialInstanceConstant->Parent);
#else
		TObjectPtr<UMaterialInterface> ParentObjectPtr;
		LoadObject(ParentPtr, ParentObjectPtr);
		MaterialInstanceConstant->Parent = ParentObjectPtr.Get();
#endif
	}

	const TSharedPtr<FJsonObject>* SubsurfaceProfilePtr;
	if (Properties->TryGetObjectField("SubsurfaceProfile", SubsurfaceProfilePtr)) {
#if ENGINE_MAJOR_VERSION >= 5
		LoadObject(SubsurfaceProfilePtr, MaterialInstanceConstant->SubsurfaceProfile);
#else
		TObjectPtr<USubsurfaceProfile> SubsurfaceProfilObjectPtr;
		LoadObject(SubsurfaceProfilePtr, SubsurfaceProfilObjectPtr);
		MaterialInstanceConstant->SubsurfaceProfile = SubsurfaceProfilObjectPtr.Get();
#endif
	}

	bool bOverrideSubsurfaceProfile;
	if (Properties->TryGetBoolField("bOverrideSubsurfaceProfile", bOverrideSubsurfaceProfile))
		MaterialInstanceConstant->bOverrideSubsurfaceProfile = bOverrideSubsurfaceProfile;

	TArray<FScalarParameterValue> ScalarParameterValues;
	TArray<TSharedPtr<FJsonValue>> Scalars = Properties->GetArrayField("ScalarParameterValues");

	for (int32 i = 0; i < Scalars.Num(); i++) {
		TSharedPtr<FJsonObject> Scalar = Scalars[i]->AsObject();

		FScalarParameterValue Parameter;
		Parameter.ParameterValue = Scalar->GetNumberField("ParameterValue");
		Parameter.ExpressionGUID = FGuid(Scalar->GetStringField("ExpressionGUID"));

		const TSharedPtr<FJsonObject>* ParameterInfoPtr;
		if (Scalar->TryGetObjectField("ParameterInfo", ParameterInfoPtr)) {
			TSharedPtr<FJsonObject> ParameterInfoJson = Scalar->GetObjectField("ParameterInfo");
			FMaterialParameterInfo ParameterInfo;
			ParameterInfo.Index = ParameterInfoJson->GetIntegerField("Index");
			ParameterInfo.Name = FName(ParameterInfoJson->GetStringField("Name"));
			ParameterInfo.Association = GlobalParameter;

			Parameter.ParameterInfo = ParameterInfo;
		} else {
			FMaterialParameterInfo ParameterInfo;
			ParameterInfo.Index = -1;
			ParameterInfo.Name = FName(Scalar->GetStringField("ParameterName"));
			ParameterInfo.Association = GlobalParameter;

			Parameter.ParameterInfo = ParameterInfo;
		}

		ScalarParameterValues.Add(Parameter);
	}

	MaterialInstanceConstant->ScalarParameterValues = ScalarParameterValues;
	TArray<FVectorParameterValue> VectorParameterValues;

	TArray<TSharedPtr<FJsonValue>> Vectors = Properties->GetArrayField("VectorParameterValues");
	for (int32 i = 0; i < Vectors.Num(); i++) {
		TSharedPtr<FJsonObject> Vector = Vectors[i]->AsObject();

		FVectorParameterValue Parameter;
		Parameter.ExpressionGUID = FGuid(Vector->GetStringField("ExpressionGUID"));

		Parameter.ParameterValue = FMathUtilities::ObjectToLinearColor(Vector->GetObjectField("ParameterValue").Get());

		const TSharedPtr<FJsonObject>* ParameterInfoPtr;
		
		if (Vector->TryGetObjectField("ParameterInfo", ParameterInfoPtr)) {
			TSharedPtr<FJsonObject> ParameterInfoJson = Vector->GetObjectField("ParameterInfo");
			FMaterialParameterInfo ParameterInfo;
			ParameterInfo.Index = ParameterInfoJson->GetIntegerField("Index");
			ParameterInfo.Name = FName(ParameterInfoJson->GetStringField("Name"));
			ParameterInfo.Association = GlobalParameter;

			Parameter.ParameterInfo = ParameterInfo;
		} else {
			FMaterialParameterInfo ParameterInfo;
			ParameterInfo.Index = -1;
			ParameterInfo.Name = FName(Vector->GetStringField("ParameterName"));
			ParameterInfo.Association = GlobalParameter;

			Parameter.ParameterInfo = ParameterInfo;
		}

		VectorParameterValues.Add(Parameter);
	}

	MaterialInstanceConstant->VectorParameterValues = VectorParameterValues;
	TArray<FTextureParameterValue> TextureParameterValues;

	TArray<TSharedPtr<FJsonValue>> Textures = Properties->GetArrayField("TextureParameterValues");
	for (int32 i = 0; i < Textures.Num(); i++) {
		TSharedPtr<FJsonObject> Texture = Textures[i]->AsObject();

		FTextureParameterValue Parameter;
		Parameter.ExpressionGUID = FGuid(Texture->GetStringField("ExpressionGUID"));

		const TSharedPtr<FJsonObject>* TexturePtr = nullptr;
		if (Texture->TryGetObjectField("ParameterValue", TexturePtr) && TexturePtr != nullptr) {
#if ENGINE_MAJOR_VERSION >= 5
			LoadObject(TexturePtr, Parameter.ParameterValue);
#else
			TObjectPtr<UTexture> TextureObjectPtr;
			LoadObject(TexturePtr, TextureObjectPtr);
			Parameter.ParameterValue = TextureObjectPtr.Get();
#endif
		}

		const TSharedPtr<FJsonObject>* ParameterInfoPtr;
		
		if (Texture->TryGetObjectField("ParameterInfo", ParameterInfoPtr)) {
			TSharedPtr<FJsonObject> ParameterInfoJson = Texture->GetObjectField("ParameterInfo");
			FMaterialParameterInfo ParameterInfo;
			ParameterInfo.Index = ParameterInfoJson->GetIntegerField("Index");
			ParameterInfo.Name = FName(ParameterInfoJson->GetStringField("Name"));
			ParameterInfo.Association = GlobalParameter;

			Parameter.ParameterInfo = ParameterInfo;
		} else {
			FMaterialParameterInfo ParameterInfo;
			ParameterInfo.Index = -1;
			ParameterInfo.Name = FName(Texture->GetStringField("ParameterName"));
			ParameterInfo.Association = GlobalParameter;

			Parameter.ParameterInfo = ParameterInfo;
		}

		TextureParameterValues.Add(Parameter);
	}

	MaterialInstanceConstant->TextureParameterValues = TextureParameterValues;

	TArray<TSharedPtr<FJsonValue>> Local_StaticParameterObjects;
	TArray<TSharedPtr<FJsonValue>> Local_StaticComponentMaskParametersObjects;
	const TSharedPtr<FJsonObject>* StaticParams;

	if (Properties->TryGetObjectField("StaticParametersRuntime", StaticParams)) {
		Local_StaticParameterObjects = StaticParams->Get()->GetArrayField("StaticSwitchParameters");
	} else if (EditorOnlyData.Num() > 0) {
		for (TSharedPtr<FJsonObject> Ed : EditorOnlyData) {
			const TSharedPtr<FJsonObject> Props = Ed->GetObjectField("Properties");

			if (Props->TryGetObjectField("StaticParameters", StaticParams)) {
				for (TSharedPtr<FJsonValue> Parameter : StaticParams->Get()->GetArrayField("StaticSwitchParameters")) {
					Local_StaticParameterObjects.Add(TSharedPtr<FJsonValue>(Parameter));
				}

				for (TSharedPtr<FJsonValue> Parameter : StaticParams->Get()->GetArrayField("StaticComponentMaskParameters")) {
					Local_StaticComponentMaskParametersObjects.Add(TSharedPtr<FJsonValue>(Parameter));
				}
			}
		}
	} else if (Properties->TryGetObjectField("StaticParameters", StaticParams)) {
		Local_StaticParameterObjects = StaticParams->Get()->GetArrayField("StaticSwitchParameters");
	}

	// --------- STATIC PARAMETERS -----------
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 2
	FStaticParameterSet NewStaticParameterSet; // Unreal Engine 5.2 and beyond have a different method
#endif

	TArray<FStaticSwitchParameter> StaticSwitchParameters;
	for (const TSharedPtr<FJsonValue> StaticParameter_Value : Local_StaticParameterObjects) {
		TSharedPtr<FJsonObject> ParameterObject = StaticParameter_Value->AsObject();
		TSharedPtr<FJsonObject> Local_MaterialParameterInfo = ParameterObject->GetObjectField("ParameterInfo");

		// Create Material Parameter Info
		FMaterialParameterInfo MaterialParameterParameterInfo = FMaterialParameterInfo(
			FName(Local_MaterialParameterInfo->GetStringField("Name")),
			static_cast<EMaterialParameterAssociation>(StaticEnum<EMaterialParameterAssociation>()->GetValueByNameString(Local_MaterialParameterInfo->GetStringField("Association"))),
			Local_MaterialParameterInfo->GetIntegerField("Index")
		);

		// Now, create the actual switch parameter
		FStaticSwitchParameter Parameter = FStaticSwitchParameter(
			MaterialParameterParameterInfo,
			ParameterObject->GetBoolField("Value"),
			ParameterObject->GetBoolField("bOverride"),
			FGuid(ParameterObject->GetStringField("ExpressionGUID"))
		);

		StaticSwitchParameters.Add(Parameter);
		#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 2
			MaterialInstanceConstant->GetEditorOnlyData()->StaticParameters.StaticSwitchParameters.Add(Parameter);
		#endif
		#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 2
			// Unreal Engine 5.2 and beyond have a different method
			NewStaticParameterSet.StaticSwitchParameters.Add(Parameter); 
		#endif
	}

	TArray<FStaticComponentMaskParameter> StaticSwitchMaskParameters;
	for (const TSharedPtr<FJsonValue> StaticParameter_Value : Local_StaticComponentMaskParametersObjects) {
		TSharedPtr<FJsonObject> ParameterObject = StaticParameter_Value->AsObject();
		TSharedPtr<FJsonObject> Local_MaterialParameterInfo = ParameterObject->GetObjectField("ParameterInfo");

		// Create Material Parameter Info
		FMaterialParameterInfo MaterialParameterParameterInfo = FMaterialParameterInfo(
			FName(Local_MaterialParameterInfo->GetStringField("Name")),
			static_cast<EMaterialParameterAssociation>(StaticEnum<EMaterialParameterAssociation>()->GetValueByNameString(Local_MaterialParameterInfo->GetStringField("Association"))),
			Local_MaterialParameterInfo->GetIntegerField("Index")
		);

		FStaticComponentMaskParameter Parameter = FStaticComponentMaskParameter(
			MaterialParameterParameterInfo,
			ParameterObject->GetBoolField("R"),
			ParameterObject->GetBoolField("G"),
			ParameterObject->GetBoolField("B"),
			ParameterObject->GetBoolField("A"),
			ParameterObject->GetBoolField("bOverride"),
			FGuid(ParameterObject->GetStringField("ExpressionGUID"))
		);

		StaticSwitchMaskParameters.Add(Parameter);
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 2
		MaterialInstanceConstant->GetEditorOnlyData()->StaticParameters.StaticComponentMaskParameters.Add(Parameter);
#endif
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 2
		NewStaticParameterSet.EditorOnly.StaticComponentMaskParameters.Add(Parameter);
#endif
	}

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 2
	FMaterialUpdateContext MaterialUpdateContext(FMaterialUpdateContext::EOptions::Default & ~FMaterialUpdateContext::EOptions::RecreateRenderStates);

	MaterialInstanceConstant->UpdateStaticPermutation(NewStaticParameterSet, &MaterialUpdateContext);
	MaterialInstanceConstant->InitStaticPermutation();
#endif

	return OnAssetCreation(MaterialInstanceConstant);
}
