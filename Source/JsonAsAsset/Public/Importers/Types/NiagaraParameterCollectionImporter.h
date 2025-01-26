// Copyright JAA Contributors 2024-2025

#pragma once

#include "../Constructor/Importer.h"
#include "NiagaraParameterCollectionImporter.h"
#include "NiagaraParameterCollection.h"

class CNiagaraParameterCollectionDerived : public UNiagaraParameterCollection {
public:
    void SetSourceMaterialCollection(TObjectPtr<UMaterialParameterCollection> MaterialParameterCollection);
    void SetCompileId(FGuid Guid);
    void SetNamespace(FName InNamespace);
    void AddAParameter(FNiagaraVariable Parameter);
};

class INiagaraParameterCollectionImporter : public IImporter {
public:
    INiagaraParameterCollectionImporter(const FString& FileName, const FString& FilePath, const TSharedPtr<FJsonObject>& JsonObject, UPackage* Package, UPackage* OutermostPkg) :
        IImporter(FileName, FilePath, JsonObject, Package, OutermostPkg) {
    }

    virtual bool ImportData() override;
};