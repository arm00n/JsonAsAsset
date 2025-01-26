﻿// Copyright JAA Contributors 2024-2025

#pragma once

#include "./Importer.h"

// Basic template importer using Asset Class.
template <typename AssetType>
class ITemplatedImporter : public IImporter {
public:
	const UClass* AssetClass;

	// We mash these properties back into the properties object if needed
	// [basically when CUE4Parse doesn't format it properly]
	TArray<FString> PropertyMash = {
		"ChildClasses"
	};

	ITemplatedImporter(UClass* AssetClass, const FString& FileName, const FString& FilePath, const TSharedPtr<FJsonObject>& JsonObject, UPackage* Package, UPackage* OutermostPkg, const TArray<TSharedPtr<FJsonValue>>& AllJsonObjects) :
		IImporter(FileName, FilePath, JsonObject, Package, OutermostPkg, AllJsonObjects),
		AssetClass(AssetClass) {}

	virtual bool ImportData() override;
};