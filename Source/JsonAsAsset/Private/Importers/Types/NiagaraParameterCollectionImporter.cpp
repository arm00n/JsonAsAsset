// Copyright JAA Contributors 2024-2025

#include "Importers/Types/NiagaraParameterCollectionImporter.h"

#include "Dom/JsonObject.h"
#include "Materials/MaterialParameterCollection.h"

void CNiagaraParameterCollectionDerived::SetSourceMaterialCollection(TObjectPtr<UMaterialParameterCollection> MaterialParameterCollection) {
    this->SourceMaterialCollection = MaterialParameterCollection;
}

void CNiagaraParameterCollectionDerived::SetCompileId(FGuid Guid) {
    this->CompileId = Guid;
}

void CNiagaraParameterCollectionDerived::SetNamespace(FName InNamespace) {
    this->Namespace = InNamespace;
}

void CNiagaraParameterCollectionDerived::AddAParameter(FNiagaraVariable Parameter) {
    this->Parameters.Add(Parameter);
}

bool INiagaraParameterCollectionImporter::ImportData() {
    TSharedPtr<FJsonObject> Properties = JsonObject->GetObjectField("Properties");

    CNiagaraParameterCollectionDerived* NiagaraParameterCollection = Cast<CNiagaraParameterCollectionDerived>(
        NewObject<UNiagaraParameterCollection>(Package, UNiagaraParameterCollection::StaticClass(), *FileName, RF_Public | RF_Standalone));

    NiagaraParameterCollection->SetCompileId(FGuid(Properties->GetStringField("CompileId")));

    TObjectPtr<UMaterialParameterCollection> MaterialParameterCollection;
    const TSharedPtr<FJsonObject>* SourceMaterialCollection;
    
    if (Properties->TryGetObjectField("SourceMaterialCollection", SourceMaterialCollection))
        LoadObject(SourceMaterialCollection, MaterialParameterCollection);

    NiagaraParameterCollection->SetSourceMaterialCollection(MaterialParameterCollection);

    const TArray<TSharedPtr<FJsonValue>>* ParametersPtr;
    if (Properties->TryGetArrayField("Parameters", ParametersPtr)) {
        for (const TSharedPtr<FJsonValue> ParameterPtr : *ParametersPtr) {
            TSharedPtr<FJsonObject> ParameterObj = ParameterPtr->AsObject();

            FName Name = FName(*ParameterObj->GetStringField("Name"));
        }
    }

    // Handle edit changes, and add it to the content browser
    return OnAssetCreation(NiagaraParameterCollection);
}