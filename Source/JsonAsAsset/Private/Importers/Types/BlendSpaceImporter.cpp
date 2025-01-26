﻿// Copyright JAA Contributors 2024-2025

#include "Importers/Types/BlendSpaceImporter.h"
#include "Utilities/MathUtilities.h"

bool IBlendSpaceImporter::ImportData() {
	TSharedPtr<FJsonObject> AssetData = JsonObject->GetObjectField("Properties");
	UBlendSpace* BlendSpace = NewObject<UBlendSpace>(Package, UBlendSpace::StaticClass(), *FileName, RF_Public | RF_Standalone);
	
	auto SampleData = AssetData->GetArrayField("SampleData");
	BlendSpace->Modify();

	/**
	 * I do want to evenutally make this read by the property seralizer, but at the moment
	 * CUE4Parse does not export them as a array, meaning it won't work.
	 */
	if (AssetData->HasField("BlendParameters")) {
		const TSharedPtr<FJsonObject> BlendParamsObject = AssetData->GetObjectField("BlendParameters");

		FBlendParameter PrimaryBlendParam;
		PrimaryBlendParam.DisplayName = BlendParamsObject->GetStringField("DisplayName");
		PrimaryBlendParam.Min = BlendParamsObject->GetNumberField("Min");
		PrimaryBlendParam.Max = BlendParamsObject->GetNumberField("Max");
		if (BlendParamsObject->HasField("GridNum")) {
			PrimaryBlendParam.GridNum = BlendParamsObject->GetNumberField("GridNum");
		} else {
			PrimaryBlendParam.GridNum = 4;
		}

		Cast<CBlendSpaceDerived>(BlendSpace)->SetBlendParameterPrimary(PrimaryBlendParam);
	}

	if (AssetData->HasField("BlendParameters[1]"))
	{
		const TSharedPtr<FJsonObject> BlendParamsObjectSecondary = AssetData->GetObjectField("BlendParameters[1]");

		FBlendParameter SecondaryBlendParam;
		SecondaryBlendParam.DisplayName = BlendParamsObjectSecondary->GetStringField("DisplayName");
		SecondaryBlendParam.Min = BlendParamsObjectSecondary->GetNumberField("Min");
		SecondaryBlendParam.Max = BlendParamsObjectSecondary->GetNumberField("Max");

		if (BlendParamsObjectSecondary->HasField("GridNum")) {
			SecondaryBlendParam.GridNum = BlendParamsObjectSecondary->GetNumberField("GridNum");
		} else {
			SecondaryBlendParam.GridNum = 4;
		}

		Cast<CBlendSpaceDerived>(BlendSpace)->SetBlendParameterSecondary(SecondaryBlendParam);
	}

	if (AssetData->HasField("InterpolationParam")) {
		const TSharedPtr<FJsonObject> InterpolationParamObject = AssetData->GetObjectField("InterpolationParam");

		FInterpolationParameter PrimaryInterpolationParam;
		PrimaryInterpolationParam.InterpolationTime = InterpolationParamObject->GetNumberField("InterpolationTime");

		Cast<CBlendSpaceDerived>(BlendSpace)->SetInterpolationParamPrimary(PrimaryInterpolationParam);
	}

	if (AssetData->HasField("InterpolationParam[1]")) {
		const TSharedPtr<FJsonObject> InterpolationParamObjectSecondary = AssetData->GetObjectField("InterpolationParam[1]");

		FInterpolationParameter SecondaryInterpolationParam;
		SecondaryInterpolationParam.InterpolationTime = InterpolationParamObjectSecondary->GetNumberField("InterpolationTime");

		Cast<CBlendSpaceDerived>(BlendSpace)->SetInterpolationParamSecondary(SecondaryInterpolationParam);
	}

	// Add samples
	for (const TSharedPtr<FJsonValue>& JsonObjectValue : SampleData) {
		const TSharedPtr<FJsonObject> JsonObjectVal = JsonObjectValue->AsObject();

		if (JsonObjectVal.IsValid()) {
			auto AnimationJsonObject = JsonObjectVal->GetObjectField("Animation");

			FString AnimationPath = AnimationJsonObject->GetStringField("ObjectPath").Replace(TEXT("FortniteGame/Content"), TEXT("/Game"));
			AnimationPath.Split(".", &AnimationPath, nullptr);

			UObject* Object = StaticLoadObject(UObject::StaticClass(), nullptr, *AnimationPath);

			BlendSpace->Modify();
			Cast<CBlendSpaceDerived>(BlendSpace)->AddSampleOnly(Cast<UAnimSequence>(Object), FMathUtilities::ObjectToVector(JsonObjectVal->GetObjectField("SampleValue").Get()));
			BlendSpace->PostEditChange();
		}
	}

	// Ensure internal state is refreshed after adding all samples
	BlendSpace->ValidateSampleData();
	FPropertyChangedEvent PropertyChangedEvent(nullptr);
	BlendSpace->PostEditChangeProperty(PropertyChangedEvent);
	BlendSpace->MarkPackageDirty();
	BlendSpace->PostEditChange();
	
	GetObjectSerializer()->DeserializeObjectProperties(RemovePropertiesShared(AssetData,
	{
		"SampleData",
		"GridSamples",
		"InterpolationParam",
		"InterpolationParam[1]",
		"BlendParameters",
		"BlendParameters[1]",
	}), BlendSpace);

	return OnAssetCreation(BlendSpace);
}

// BlendSpace Derived class functionality
void CBlendSpaceDerived::AddSampleOnly(UAnimSequence* AnimationSequence, const FVector& SampleValue) {
	SampleData.Add(FBlendSample(AnimationSequence, SampleValue, true, true));
	PreviewBasePose = nullptr;
}

void CBlendSpaceDerived::SetAxisToScaleAnimationInput(const EBlendSpaceAxis AxisToScaleAnimationInput) {
	this->AxisToScaleAnimation = AxisToScaleAnimationInput;
}

void CBlendSpaceDerived::SetBlendParameterPrimary(const FBlendParameter& BlendParametersInput) {
	this->BlendParameters[0] = BlendParametersInput;
}

void CBlendSpaceDerived::SetBlendParameterSecondary(const FBlendParameter& BlendParametersInput) {
	this->BlendParameters[1] = BlendParametersInput;
}

void CBlendSpaceDerived::SetInterpolationParamPrimary(const FInterpolationParameter InterpolationParamInput) {
	this->InterpolationParam[0] = InterpolationParamInput;
}

void CBlendSpaceDerived::SetInterpolationParamSecondary(const FInterpolationParameter InterpolationParamInput) {
	this->InterpolationParam[1] = InterpolationParamInput;
}

void CBlendSpaceDerived::SetNotifyTriggerMode(const ENotifyTriggerMode::Type NotifyTriggerModeInput) {
	this->NotifyTriggerMode = NotifyTriggerModeInput;
}