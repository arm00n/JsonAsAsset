// Copyright JAA Contributors 2024-2025

#include "Importers/Types/PhysicsAssetImporter.h"

bool IPhysicsAssetImporter::ImportData() {
	try {
	} catch (const char* Exception) {
		UE_LOG(LogJson, Error, TEXT("%s"), *FString(Exception));
	}

	return false;
}
