﻿// Copyright JAA Contributors 2024-2025

#include "Importers/Types/AnimationBaseImporter.h"

#include "Dom/JsonObject.h"
#include "Utilities/AssetUtilities.h"
#include "Utilities/MathUtilities.h"
#include "Animation/AnimSequence.h"

#if ENGINE_MAJOR_VERSION == 5
#include "Animation/AnimData/IAnimationDataController.h"
#if ENGINE_MINOR_VERSION >= 4
#include "Animation/AnimData/IAnimationDataModel.h"
#endif
#include "AnimDataController.h"
#endif

bool IAnimationBaseImporter::ImportData() {
	// Properties of the object
	TSharedPtr<FJsonObject> Properties = JsonObject->GetObjectField("Properties");
	FString AssetName = JsonObject->GetStringField("Name");

	TArray<TSharedPtr<FJsonValue>> FloatCurves;
	TArray<TSharedPtr<FJsonValue>> Notifies;

	UAnimSequenceBase* AnimSequenceBase = nullptr;

	// ----------------------------------------------------------------------------
	// Find the asset in the current content browser selection
	TArray<FAssetData> AssetDataList = GetAssetsInSelectedFolder();

	for (const FAssetData& AssetData : AssetDataList) {
		if (UAnimSequenceBase* SequenceBase = Cast<UAnimSequenceBase>(AssetData.GetAsset())) {
			FString AnimationName = SequenceBase->GetName();

			if (AnimationName == AssetName) {
				AnimSequenceBase = SequenceBase;
			}
		}
	}

	if (!AnimSequenceBase) {
		AnimSequenceBase = Cast<UAnimSequenceBase>(FAssetUtilities::GetSelectedAsset());
	}

	ensure(AnimSequenceBase);
	if (!AnimSequenceBase)
	{
		UE_LOG(LogJson, Error, TEXT("Could not get valid AnimSequenceBase"));
		return false;
	}

	USkeleton* Skeleton = AnimSequenceBase->GetSkeleton();
	ensure(Skeleton);
	if (!Skeleton)
	{
		UE_LOG(LogJson, Error, TEXT("Could not get valid Skeleton"));
		return false;
	}
	
	/* In Unreal Engine 5, a new data model has been added to edit animation curves */
	// Unreal Engine 5.2 changed handling getting a data model
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 2
	IAnimationDataController& Controller = AnimSequenceBase->GetController();
#if ENGINE_MINOR_VERSION >= 3
	IAnimationDataModel* DataModel = AnimSequenceBase->GetDataModel();
#endif
#endif

	// Some CUE4Parse versions have different named objects for curves
	const TSharedPtr<FJsonObject>* RawCurveData;
	
	if (Properties->TryGetObjectField("RawCurveData", RawCurveData)) FloatCurves = Properties->GetObjectField("RawCurveData")->GetArrayField("FloatCurves");
	else if (JsonObject->TryGetObjectField("CompressedCurveData", RawCurveData)) FloatCurves = JsonObject->GetObjectField("CompressedCurveData")->GetArrayField("FloatCurves");

	for (TSharedPtr<FJsonValue> FloatCurveObject : FloatCurves)
	{
		// Display Name (for example: jaw_open_pose)
		FString DisplayName = "";
		if (FloatCurveObject->AsObject()->HasField("Name")) {
			DisplayName = FloatCurveObject->AsObject()->GetObjectField("Name")->GetStringField("DisplayName");
		} else {
			DisplayName = FloatCurveObject->AsObject()->GetStringField("CurveName");
		}

		// Curve Type Flags:
		// Used to define if a curve is a curve is metadata or not.
		int CurveTypeFlags = FloatCurveObject->AsObject()->GetIntegerField("CurveTypeFlags");

#if ENGINE_MAJOR_VERSION == 4
		FSmartName NewTrackName;

		// Included to add the curve's name to the skeleton's data
		Skeleton->AddSmartNameAndModify(USkeleton::AnimCurveMappingName, FName(*DisplayName), NewTrackName);
		ensureAlways(Skeleton->GetSmartNameByUID(USkeleton::AnimCurveMappingName, NewTrackName.UID, NewTrackName));
#endif
		
#if ENGINE_MAJOR_VERSION == 5
#if ENGINE_MINOR_VERSION <= 3
		FSmartName NewTrackName;

		// Included to add the curve's name to the skeleton's data
		Skeleton->AddSmartNameAndModify(USkeleton::AnimCurveMappingName, FName(*DisplayName), NewTrackName);
		
		ensureAlways(Skeleton->GetSmartNameByUID(USkeleton::AnimCurveMappingName, NewTrackName.UID, NewTrackName));
		FAnimationCurveIdentifier CurveId = FAnimationCurveIdentifier(NewTrackName, ERawCurveTrackTypes::RCT_Float);
#endif
#if ENGINE_MINOR_VERSION >= 4
		// Create Curve Identifier
		FName CurveName = FName(*DisplayName);
		FAnimationCurveIdentifier CurveId(CurveName, ERawCurveTrackTypes::RCT_Float);

		// Add curve metadata to skeleton
		Skeleton->AddCurveMetaData(CurveName);
		// Add or update the curve
		const FFloatCurve* ExistingCurve = DataModel->FindFloatCurve(CurveId);
		if (ExistingCurve == nullptr)
		{
			Controller.AddCurve(CurveId, CurveTypeFlags, true);
		}
		else 
		{
			// Update existing curve flags if needed
			Controller.SetCurveFlags(CurveId, CurveTypeFlags, true);
		}
#endif
		// For Unreal Engine 5.3 and above, the smart name's display name is required
#if ENGINE_MINOR_VERSION == 3 && ENGINE_PATCH_VERSION < 2
		Controller->AddCurve(CurveId, CurveTypeFlags);
#elif (ENGINE_MINOR_VERSION == 3 && ENGINE_PATCH_VERSION == 2)
		Controller.AddCurve(CurveId, CurveTypeFlags);
#endif
		// For Unreal Engine 5.2 and below, just the smart name is required
#if ENGINE_MINOR_VERSION < 3
		AnimSequenceBase->Modify(true);

		IAnimationDataController& LocalOneController = AnimSequenceBase->GetController();
		LocalOneController.AddCurve(FAnimationCurveIdentifier(NewTrackName, ERawCurveTrackTypes::RCT_Float), CurveTypeFlags);

		TArray<FLinearColor> RandomizedColorArray = {
			FLinearColor(.904, .323, .539),
			FLinearColor(.552, .737, .328),
			FLinearColor(.947, .418, .219),
			FLinearColor(.156, .624, .921),
			FLinearColor(.921, .314, .337),
			FLinearColor(.361, .651, .332),
			FLinearColor(.982, .565, .254),
			FLinearColor(.246, .223, .514),
			FLinearColor(.208, .386, .687),
			FLinearColor(.223, .590, .337),
			FLinearColor(.230, .291, .591)
		}; { // Random Color
			auto index = rand() % RandomizedColorArray.Num();

			if (RandomizedColorArray.IsValidIndex(index)) // Safe
				LocalOneController.SetCurveColor(FAnimationCurveIdentifier(NewTrackName, ERawCurveTrackTypes::RCT_Float), RandomizedColorArray[index]);
		}

		AnimSequenceBase->PostEditChange();
#endif
#endif

		// Each key of the curve
		TArray<TSharedPtr<FJsonValue>> Keys = FloatCurveObject->AsObject()->GetObjectField("FloatCurve")->GetArrayField("Keys");

		for (TSharedPtr<FJsonValue> JsonKey : Keys) {
			TSharedPtr<FJsonObject> Key = JsonKey->AsObject();

			FRichCurveKey RichKey = FMathUtilities::ObjectToRichCurveKey(Key);

			// Unreal Engine 5 and Unreal Engine 4
			// have different ways of adding curves
			//
			// Unreal Engine 4: Simply adding curves to RawCurveData
			// Unreal Engine 5: Using a AnimDataController to handle adding curves
#if ENGINE_MAJOR_VERSION == 5
			Controller.SetCurveKey(CurveId, RichKey);
#endif
#if ENGINE_MAJOR_VERSION == 4
			AnimSequenceBase->RawCurveData.AddFloatCurveKey(NewTrackName, CurveTypeFlags, RichKey.Time, RichKey.Value);
			AnimSequenceBase->RawCurveData.FloatCurves.Last().FloatCurve.Keys.Last().ArriveTangent = RichKey.ArriveTangent;
			AnimSequenceBase->RawCurveData.FloatCurves.Last().FloatCurve.Keys.Last().LeaveTangent = RichKey.LeaveTangent;
			AnimSequenceBase->RawCurveData.FloatCurves.Last().FloatCurve.Keys.Last().InterpMode = RichKey.InterpMode;
#endif
		}
	}

	UAnimSequence* CastedAnimSequence = Cast<UAnimSequence>(AnimSequenceBase);

	const TArray<TSharedPtr<FJsonValue>>* AuthoredSyncMarkers1;
	
	if (Properties->TryGetArrayField("AuthoredSyncMarkers", AuthoredSyncMarkers1) && CastedAnimSequence)
	{
		TArray<TSharedPtr<FJsonValue>> AuthoredSyncMarkers = Properties->GetArrayField("AuthoredSyncMarkers");

		for (TSharedPtr<FJsonValue> SyncMarker : AuthoredSyncMarkers)
		{
			FAnimSyncMarker AuthoredSyncMarker = FAnimSyncMarker();
			AuthoredSyncMarker.MarkerName = FName(*SyncMarker.Get()->AsObject().Get()->GetStringField("MarkerName"));
			AuthoredSyncMarker.Time = SyncMarker.Get()->AsObject().Get()->GetNumberField("Time");
			CastedAnimSequence->AuthoredSyncMarkers.Add(AuthoredSyncMarker);
		}
	}

	// Whitelist
	GetObjectSerializer()->DeserializeObjectProperties(KeepPropertiesShared(Properties,
	{
		"RetargetSource",
		
		"AdditiveAnimType",
		"RefPoseType",
		"RefPoseSeq",
		"Notifies",

		// Montages
		"BlendIn",
		"BlendOut",
		"SlotAnimTracks",
		"CompositeSections"
	}), AnimSequenceBase);

#if ENGINE_MAJOR_VERSION == 5
	if (ITargetPlatform* RunningPlatform = GetTargetPlatformManagerRef().GetRunningTargetPlatform())
	{
		CastedAnimSequence->CacheDerivedData(RunningPlatform);
	}
#else
	if (CastedAnimSequence)
	{
		CastedAnimSequence->RequestSyncAnimRecompression();
	}
#endif

#if ENGINE_MAJOR_VERSION == 4
	AnimSequenceBase->MarkRawDataAsModified();
#endif
	AnimSequenceBase->Modify();
	AnimSequenceBase->PostEditChange();

	return true;
}