// Copyright JAA Contributors 2024-2025

#pragma once
#include "../Constructor/Importer.h"

class IDataAssetImporter : public IImporter {
public:
	const UClass* DataAssetClass;

	IDataAssetImporter(UClass* DataClass, const FString& FileName, const FString& FilePath, const TSharedPtr<FJsonObject>& JsonObject, UPackage* Package, UPackage* OutermostPkg, const TArray<TSharedPtr<FJsonValue>>& AllJsonObjects) :
		IImporter(FileName, FilePath, JsonObject, Package, OutermostPkg, AllJsonObjects) {
		this->DataAssetClass = DataClass;
	}

	virtual bool ImportData() override;
};
