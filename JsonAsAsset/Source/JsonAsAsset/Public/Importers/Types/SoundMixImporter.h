// Copyright JAA Contributors 2024-2025

#pragma once

#include "../Constructor/Importer.h"

class USoundMixImporter : public IImporter {
public:
	USoundMixImporter(const FString& FileName, const FString& FilePath, const TSharedPtr<FJsonObject>& JsonObject, UPackage* Package, UPackage* OutermostPkg, const TArray<TSharedPtr<FJsonValue>>& AllJsonObjects):
		IImporter(FileName, FilePath, JsonObject, Package, OutermostPkg, AllJsonObjects) {
	}

	virtual bool ImportData() override;
};