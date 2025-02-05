// Copyright JAA Contributors 2024-2025

#include "Importers/Types/PhysicsAssetImporter.h"
#include "PhysicsEngine/PhysicsConstraintTemplate.h"
#include "Utilities/EngineUtilities.h"

USkeletalBodySetup* CreateNewBody(UPhysicsAsset* PhysAsset, FName ExportName, FName BoneName)
{
	USkeletalBodySetup* NewBodySetup = NewObject<USkeletalBodySetup>(PhysAsset, ExportName, RF_Transactional);
	NewBodySetup->BoneName = BoneName;

	PhysAsset->SkeletalBodySetups.Add(NewBodySetup);
	PhysAsset->UpdateBodySetupIndexMap();

	return NewBodySetup;
}

UPhysicsConstraintTemplate* CreateNewConstraint(UPhysicsAsset* PhysAsset, FName ExportName)
{
	UPhysicsConstraintTemplate* NewConstraintSetup = NewObject<UPhysicsConstraintTemplate>(PhysAsset, ExportName, RF_Transactional);
	PhysAsset->ConstraintSetup.Add(NewConstraintSetup);

	return NewConstraintSetup;
}

bool IPhysicsAssetImporter::ImportData() {
	UPhysicsAsset* PhysicsAsset = NewObject<UPhysicsAsset>(Package, UPhysicsAsset::StaticClass(), *FileName, RF_Public | RF_Standalone);

	/*
	 * January 4th 2025
	 * I'm planning this beforehand so I can have a clear mind:
	 * 
	 * 1. Import main physics asset's properties
	 * 2. Read SkeletalBodySetups and create them into an array
	 * 3. Read ConstraintSetup and create them intro an array
	 * 
	 * What I'm baffled about is if this will work in the end, because
	 * physics assets have a lot of data basically baked into the uasset itself.
	 * 
	 * I made a full recreation of a physics asset a long time ago with 1:1 JSON data,
	 * yet it was completely broken on simulation.
	 * 
	 * We'll see.
	*/
	
	/*
	 * January 12th 2025
	 * Physics asset import properly, but do not simulate properly.
	 *
	 * I'm not quite sure how to fix it, but maybe an idea pops up later on.
	*/

	TSharedPtr<FJsonObject> Properties = JsonObject->GetObjectField(TEXT("Properties"));
	TMap<FName, FExportData> Exports = CreateExports();

	// SkeletalBodySetups
	ProcessJsonArrayField(Properties, TEXT("SkeletalBodySetups"), [&](const TSharedPtr<FJsonObject>& ObjectField) {
		FName ExportName = GetExportNameOfSubobject(ObjectField->GetStringField(TEXT("ObjectName")));
		FJsonObject* ExportJson = Exports.Find(ExportName)->Json;

		TSharedPtr<FJsonObject> ExportProperties = ExportJson->GetObjectField(TEXT("Properties"));
		FName BoneName = FName(*ExportProperties->GetStringField(TEXT("BoneName")));
		
		UE_LOG(LogTemp, Log, TEXT("Processing Skeletal Body Setup: %s"), *ExportName.ToString());

		USkeletalBodySetup* BodySetup = CreateNewBody(PhysicsAsset, ExportName, BoneName);
		
		GetObjectSerializer()->DeserializeObjectProperties(ExportProperties, BodySetup);
	});

	// ConstraintSetup
	ProcessJsonArrayField(Properties, TEXT("ConstraintSetup"), [&](const TSharedPtr<FJsonObject>& ObjectField) {
		FName ExportName = GetExportNameOfSubobject(ObjectField->GetStringField(TEXT("ObjectName")));
		FJsonObject* ExportJson = Exports.Find(ExportName)->Json;

		TSharedPtr<FJsonObject> ExportProperties = ExportJson->GetObjectField(TEXT("Properties"));
		UE_LOG(LogTemp, Log, TEXT("Processing Constraint Setup: %s"), *ExportName.ToString());

		UPhysicsConstraintTemplate* PhysicsConstraintTemplate = CreateNewConstraint(PhysicsAsset, ExportName);
		
		GetObjectSerializer()->DeserializeObjectProperties(ExportProperties, PhysicsConstraintTemplate);
	});

	// ---------------------------------------------------------
	// Simple data at end
	GetObjectSerializer()->DeserializeObjectProperties(RemovePropertiesShared(Properties,
	{
		"SkeletalBodySetups",
		"ConstraintSetup",
		"BoundsBodies"
	}), PhysicsAsset);

	PhysicsAsset->RefreshPhysicsAssetChange();
	PhysicsAsset->PostEditChange();
	PhysicsAsset->UpdateBoundsBodiesArray();
	PhysicsAsset->UpdateBodySetupIndexMap();
	
	return OnAssetCreation(PhysicsAsset);
}
