// Copyright JAA Contributors 2024-2025

#include "Importers/Types/MaterialFunctionImporter.h"
#include "Factories/MaterialFunctionFactoryNew.h"

bool IMaterialFunctionImporter::ImportData() {
	try {
		// Create Material Function Factory (factory automatically creates the MF)
		UMaterialFunctionFactoryNew* MaterialFunctionFactory = NewObject<UMaterialFunctionFactoryNew>();
		UMaterialFunction* MaterialFunction = Cast<UMaterialFunction>(MaterialFunctionFactory->FactoryCreateNew(UMaterialFunction::StaticClass(), OutermostPkg, *FileName, RF_Standalone | RF_Public, nullptr, GWarn));
#if ENGINE_MAJOR_VERSION >= 5
		MaterialFunction->GetExpressionCollection().Empty();
#else
		MaterialFunction->FunctionExpressions.Empty();
#endif

		// Handle edit changes, and add it to the content browser
		if (!HandleAssetCreation(MaterialFunction)) return false;

		MaterialFunction->StateId = FGuid(JsonObject->GetObjectField("Properties")->GetStringField("StateId"));
		
		// Misc properties
		bool bPrefixParameterNames;
		FString Description;
		bool bExposeToLibrary;
		
		if (JsonObject->GetObjectField("Properties")->TryGetStringField("Description", Description)) MaterialFunction->Description = Description;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("bExposeToLibrary", bExposeToLibrary)) MaterialFunction->bExposeToLibrary = bExposeToLibrary;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("bPrefixParameterNames", bPrefixParameterNames)) MaterialFunction->bPrefixParameterNames = bPrefixParameterNames;

		// Define editor only data from the JSON
		TMap<FName, FExportData> Exports;
		TArray<FName> ExpressionNames;
		const TSharedPtr<FJsonObject> EdProps = FindEditorOnlyData(JsonObject->GetStringField("Type"), MaterialFunction->GetName(), Exports, ExpressionNames, false)->GetObjectField("Properties");
		const TSharedPtr<FJsonObject> StringExpressionCollection = EdProps->GetObjectField("ExpressionCollection");

		// Map out each expression for easier access
		TMap<FName, UMaterialExpression*> CreatedExpressionMap = ConstructExpressions(MaterialFunction, MaterialFunction->GetName(), ExpressionNames, Exports);

		// Missing Material Data
		if (Exports.Num() == 0) {
			FNotificationInfo Info = FNotificationInfo(FText::FromString("Material Data Missing (" + FileName + ")"));
			Info.ExpireDuration = 7.0f;
			Info.bUseLargeFont = true;
			Info.bUseSuccessFailIcons = true;
			Info.WidthOverride = FOptionalSize(350);
#if ENGINE_MAJOR_VERSION >= 5
			Info.SubText = FText::FromString(FString("Please use the correct FModel provided in the JsonAsAsset server."));
#endif
			
			TSharedPtr<SNotificationItem> NotificationPtr = FSlateNotificationManager::Get().AddNotification(Info);
			NotificationPtr->SetCompletionState(SNotificationItem::CS_Fail);

			return false;
		}

		// Iterate through all the expression names
		PropagateExpressions(MaterialFunction, ExpressionNames, Exports, CreatedExpressionMap);
		MaterialGraphNode_ConstructComments(MaterialFunction, StringExpressionCollection, Exports);

		MaterialFunction->PreEditChange(NULL);
		MaterialFunction->PostEditChange();

		SavePackage();
	} catch (const char* Exception) {
		UE_LOG(LogJson, Error, TEXT("%s"), *FString(Exception))
		return false;
	}

	return true;
}
