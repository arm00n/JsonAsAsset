// Copyright JAA Contributors 2024-2025

#include "Importers/Types/PhysicsAssetImporter.h"

bool IPhysicsAssetImporter::ImportData() {
	UPhysicsAsset* PhysicsAsset = NewObject<UPhysicsAsset>(Package, UPhysicsAsset::StaticClass(), *FileName, RF_Public | RF_Standalone);

	return OnAssetCreation(PhysicsAsset);
}
