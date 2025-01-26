// Copyright JAA Contributors 2024-2025

#include "Importers/Constructor/MaterialGraph.h"
#include "Utilities/MathUtilities.h"
#include "Styling/SlateIconFinder.h"

// Expressions
#include "Materials/MaterialExpressionComment.h"
#include "Materials/MaterialExpressionFeatureLevelSwitch.h"
#include "Materials/MaterialExpressionShadingPathSwitch.h"
#include "Materials/MaterialExpressionQualitySwitch.h"
#include "Materials/MaterialExpressionReroute.h"

#if ENGINE_MAJOR_VERSION >= 5
#include "Materials/MaterialExpressionTextureBase.h"
#endif

static TWeakPtr<SNotificationItem> MaterialGraphNotification;

TArray<FString> IMaterialGraph::IgnoredExpressions;

IMaterialGraph::IMaterialGraph(const FString& FileName, const FString& FilePath, const TSharedPtr<FJsonObject>& JsonObject, UPackage* Package, UPackage* OutermostPkg, const TArray<TSharedPtr<FJsonValue>>& AllJsonObjects):
	IImporter(FileName, FilePath, JsonObject, Package, OutermostPkg, AllJsonObjects)
{
	/* Handled manually by IMaterialGraph */
	IgnoredExpressions = {
		"MaterialExpressionComposite",
		"MaterialExpressionPinBase",
		"MaterialExpressionComment",
		"MaterialFunction",
		"Material"
	};
}

TSharedPtr<FJsonObject> IMaterialGraph::FindEditorOnlyData(const FString& Type, const FString& Outer, TMap<FName, FExportData>& OutExports, TArray<FName>& ExpressionNames, bool bFilterByOuter) {
	TSharedPtr<FJsonObject> EditorOnlyData;

	for (const TSharedPtr<FJsonValue> Value : bFilterByOuter ? FilterExportsByOuter(Outer) : AllJsonObjects) {
		TSharedPtr<FJsonObject> Object = TSharedPtr<FJsonObject>(Value->AsObject());

		FString ExType = Object->GetStringField("Type");
		FString Name = Object->GetStringField("Name");

		if (ExType == Type + "EditorOnlyData") {
			EditorOnlyData = Object;
			continue;
		}

		// For older versions, the "editor" data is in the main UMaterial/UMaterialFunction export
		if (ExType == Type) {
			EditorOnlyData = Object;
			continue;
		}

		ExpressionNames.Add(FName(Name));
		OutExports.Add(FName(Name), FExportData(ExType, Outer, Object));
	}

	return EditorOnlyData;
}

TMap<FName, UMaterialExpression*> IMaterialGraph::ConstructExpressions(UObject* Parent, const FString& Outer, TArray<FName>& ExpressionNames, TMap<FName, FExportData>& Exports) {
	TMap<FName, UMaterialExpression*> CreatedExpressionMap;

	for (FName Name : ExpressionNames) {
		FName Type;
		FJsonObject* SharedRef = NULL;
		bool bFound = false;

		for (TTuple<FName, FExportData>& Key : Exports) {
			TSharedPtr<FJsonObject>* SharedO = new TSharedPtr<FJsonObject>(Key.Value.Json);

			if (Key.Key == Name && Key.Value.Outer == FName(*Outer)) {
				Type = Key.Value.Type;
				SharedRef = SharedO->Get();

				bFound = true;
				break;
			}
		}

		if (!bFound) continue;
		UMaterialExpression* Ex = CreateEmptyExpression(Parent, Name, Type, SharedRef);
		if (Ex == nullptr)
			continue;

		CreatedExpressionMap.Add(Name, Ex);
	}

	return CreatedExpressionMap;
}

FExpressionInput IMaterialGraph::CreateExpressionInput(TSharedPtr<FJsonObject> JsonProperties, TMap<FName, UMaterialExpression*>& CreatedExpressionMap, FString PropertyName) {
	// Find Expression Input by PropertyName
	const TSharedPtr<FJsonObject>* Ptr;
	
	if (JsonProperties->TryGetObjectField(PropertyName, Ptr)) {
		FJsonObject* AsObject = Ptr->Get();
		FName ExpressionName = GetExpressionName(AsObject);

		// If Expression Found
		if (CreatedExpressionMap.Contains(ExpressionName)) {
			FExpressionInput Input = PopulateExpressionInput(AsObject, *CreatedExpressionMap.Find(ExpressionName));

			return Input;
		}
	}

	return FExpressionInput();
}

FMaterialAttributesInput IMaterialGraph::CreateMaterialAttributesInput(TSharedPtr<FJsonObject> JsonProperties, TMap<FName, UMaterialExpression*>& CreatedExpressionMap, FString PropertyName) {
	FExpressionInput Input = CreateExpressionInput(JsonProperties, CreatedExpressionMap, PropertyName);

	return *reinterpret_cast<FMaterialAttributesInput*>(&Input);
}

void IMaterialGraph::PropagateExpressions(UObject* Parent, TArray<FName>& ExpressionNames, TMap<FName, FExportData>& Exports, TMap<FName, UMaterialExpression*>& CreatedExpressionMap, bool bCheckOuter, bool bSubgraph) {
	for (FName Name : ExpressionNames) {
		FExportData* Type = Exports.Find(Name);
		TSharedPtr<FJsonObject> Properties = Type->Json->GetObjectField("Properties");

		// Find the expression from FName
		if (!CreatedExpressionMap.Contains(Name)) continue;
		UMaterialExpression* Expression = *CreatedExpressionMap.Find(Name);

		//	Used for Subgraphs:
		//  | Checks if the outer is the same as the parent
		//  | to determine if it's in a subgraph or not.
		if (bCheckOuter) {
			FString Outer;
			if (Type->Json->TryGetStringField("Outer", Outer) && Outer != Parent->GetName()) // Not the same as parent
				continue;
		}

		// ------------ Manually check for Material Function Calls ------------ 
		if (Type->Type == "MaterialExpressionMaterialFunctionCall") {
			UMaterialExpressionMaterialFunctionCall* MaterialFunctionCall = Cast<UMaterialExpressionMaterialFunctionCall>(Expression);

			const TSharedPtr<FJsonObject>* MaterialFunctionPtr;
			
			if (Properties->TryGetObjectField("MaterialFunction", MaterialFunctionPtr)) {
				// For UE4, we fallback to TWeakObjectPtr
#if ENGINE_MAJOR_VERSION == 4
				TObjectPtr<UMaterialFunctionInterface> MaterialFunctionObjectPtr;
				MaterialFunctionObjectPtr = MaterialFunctionCall->MaterialFunction;
				
				LoadObject(MaterialFunctionPtr, MaterialFunctionObjectPtr);
#else
				LoadObject(MaterialFunctionPtr, MaterialFunctionCall->MaterialFunction);
#endif

				// Notify material function is missing
				if (MaterialFunctionCall->MaterialFunction == nullptr) {
					FString ObjectPath;
					MaterialFunctionPtr->Get()->GetStringField("ObjectPath").Split(".", &ObjectPath, nullptr);
					if (!ImportAssetReference(ObjectPath)) {} // AppendNotification(FText::FromString("Material Function Missing: " + ObjectPath), FText::FromString("Material Graph"), 2.0f, SNotificationItem::CS_Fail, true);
					else {
#if ENGINE_MAJOR_VERSION >= 5
						LoadObject(MaterialFunctionPtr, MaterialFunctionCall->MaterialFunction);
#else
						LoadObject(MaterialFunctionPtr, MaterialFunctionObjectPtr);
#endif
					}
				}
			}
		}

		// Sets 99% of properties for nodes
		GetObjectSerializer()->DeserializeObjectProperties(Properties, Expression);

		// Material Nodes with edited properties (ex: 9 objects with the same name ---> array of objects)
		if (Type->Type == "MaterialExpressionQualitySwitch") {
			UMaterialExpressionQualitySwitch* QualitySwitch = Cast<UMaterialExpressionQualitySwitch>(Expression);

			const TArray<TSharedPtr<FJsonValue>>* InputsPtr;
			
			if (Type->Json->TryGetArrayField("Inputs", InputsPtr)) {
				int i = 0;
				for (const TSharedPtr<FJsonValue> InputValue : *InputsPtr) {
					FJsonObject* InputObject = InputValue->AsObject().Get();
					FName InputExpressionName = GetExpressionName(InputObject);
					if (CreatedExpressionMap.Contains(InputExpressionName)) {
						FExpressionInput Input = PopulateExpressionInput(InputObject, *CreatedExpressionMap.Find(InputExpressionName));
						QualitySwitch->Inputs[i] = Input;
					}
					i++;
				}
			}
		} else if (Type->Type == "MaterialExpressionShadingPathSwitch") {
			UMaterialExpressionShadingPathSwitch* ShadingPathSwitch = Cast<UMaterialExpressionShadingPathSwitch>(Expression);

			const TArray<TSharedPtr<FJsonValue>>* InputsPtr;
			
			if (Type->Json->TryGetArrayField("Inputs", InputsPtr)) {
				int i = 0;
				for (const TSharedPtr<FJsonValue> InputValue : *InputsPtr) {
					FJsonObject* InputObject = InputValue->AsObject().Get();
					FName InputExpressionName = GetExpressionName(InputObject);
					if (CreatedExpressionMap.Contains(InputExpressionName)) {
						FExpressionInput Input = PopulateExpressionInput(InputObject, *CreatedExpressionMap.Find(InputExpressionName));
						ShadingPathSwitch->Inputs[i] = Input;
					}
					i++;
				}
			}
		} else if (Type->Type == "MaterialExpressionFeatureLevelSwitch") {
			UMaterialExpressionFeatureLevelSwitch* FeatureLevelSwitch = Cast<UMaterialExpressionFeatureLevelSwitch>(Expression);

			const TArray<TSharedPtr<FJsonValue>>* InputsPtr;
			
			if (Type->Json->TryGetArrayField("Inputs", InputsPtr)) {
				int i = 0;
				for (const TSharedPtr<FJsonValue> InputValue : *InputsPtr) {
					FJsonObject* InputObject = InputValue->AsObject().Get();
					FName InputExpressionName = GetExpressionName(InputObject);
					if (CreatedExpressionMap.Contains(InputExpressionName)) {
						FExpressionInput Input = PopulateExpressionInput(InputObject, *CreatedExpressionMap.Find(InputExpressionName));
						FeatureLevelSwitch->Inputs[i] = Input;
					}
					i++;
				}
			}
		}

		MaterialGraphNode_ExpressionWrapper(Parent, Expression, Properties);

		if (!bSubgraph) {
			// Adding expressions is different between UE4 and UE5
#if ENGINE_MAJOR_VERSION >= 5
			if (UMaterialFunction* FuncCasted = Cast<UMaterialFunction>(Parent)) {
				FuncCasted->GetExpressionCollection().AddExpression(Expression);
			}

			if (UMaterial* MatCasted = Cast<UMaterial>(Parent)) {
				MatCasted->GetEditorOnlyData()->ExpressionCollection.Expressions.Add(Expression);
				Expression->UpdateMaterialExpressionGuid(true, false);
				MatCasted->AddExpressionParameter(Expression, MatCasted->EditorParameters);
			}
#else
			if (UMaterialFunction* FuncCasted = Cast<UMaterialFunction>(Parent))
				FuncCasted->FunctionExpressions.Add(Expression);

			if (UMaterial* MatCasted = Cast<UMaterial>(Parent)) {
				MatCasted->Expressions.Add(Expression);
				Expression->UpdateMaterialExpressionGuid(true, false);
				MatCasted->AddExpressionParameter(Expression, MatCasted->EditorParameters);
			}
#endif
		}
	}
}

void IMaterialGraph::MaterialGraphNode_AddComment(UObject* Parent, UMaterialExpressionComment* Comment) {
#if ENGINE_MAJOR_VERSION >= 5
	if (UMaterialFunction* FuncCasted = Cast<UMaterialFunction>(Parent)) FuncCasted->GetExpressionCollection().AddComment(Comment);
	if (UMaterial* MatCasted = Cast<UMaterial>(Parent)) MatCasted->GetExpressionCollection().AddComment(Comment);
#else
	if (UMaterialFunction* FuncCasted = Cast<UMaterialFunction>(Parent)) FuncCasted->FunctionEditorComments.Add(Comment);
	if (UMaterial* MatCasted = Cast<UMaterial>(Parent)) MatCasted->EditorComments.Add(Comment);
#endif
}

void IMaterialGraph::MaterialGraphNode_ConstructComments(UObject* Parent, const TSharedPtr<FJsonObject>& Json, TMap<FName, FExportData>& Exports) {
	const TArray<TSharedPtr<FJsonValue>>* StringExpressionComments;
	
	if (Json->TryGetArrayField("EditorComments", StringExpressionComments))
		// Iterate through comments
		for (const TSharedPtr<FJsonValue> ExpressionComment : *StringExpressionComments) {
			if (ExpressionComment->IsNull()) continue; // just in-case

			FName ExportName = GetExportNameOfSubobject(ExpressionComment.Get()->AsObject()->GetStringField("ObjectName"));

			// Get properties of comment, and create it relative to parent
			const TSharedPtr<FJsonObject> Properties = Exports.Find(ExportName)->Json->GetObjectField("Properties");
			UMaterialExpressionComment* Comment = NewObject<UMaterialExpressionComment>(Parent, UMaterialExpressionComment::StaticClass(), ExportName, RF_Transactional);

			// Deserialize and send it off to the material
			MaterialGraphNode_ExpressionWrapper(Parent, Comment, Properties);
			GetObjectSerializer()->DeserializeObjectProperties(Properties, Comment);

			MaterialGraphNode_AddComment(Parent, Comment);
		}

	for (TTuple<FString, FJsonObject*>& Key : MissingNodeClasses) {
		TSharedPtr<FJsonObject>* SharedObject = new TSharedPtr<FJsonObject>(Key.Value);

		const TSharedPtr<FJsonObject> Properties = SharedObject->Get()->GetObjectField("Properties");
		UMaterialExpressionComment* Comment = NewObject<UMaterialExpressionComment>(Parent, UMaterialExpressionComment::StaticClass(), *("UMaterialExpressionComment_" + Key.Key), RF_Transactional);

		Comment->Text = *("Missing Node Class " + Key.Key);
		Comment->CommentColor = FLinearColor(1.0, 0.0, 0.0);
		Comment->bCommentBubbleVisible = true;
		Comment->SizeX = 415;
		Comment->SizeY = 40;

		Comment->Desc = "A node is missing in your Unreal Engine build, this may be for many reasons, primarily due to your version of Unreal being younger than the one your porting from.";

		GetObjectSerializer()->DeserializeObjectProperties(Properties, Comment);
		MaterialGraphNode_AddComment(Parent, Comment);
	}
}

void IMaterialGraph::MaterialGraphNode_ExpressionWrapper(UObject* Parent, UMaterialExpression* Expression, const TSharedPtr<FJsonObject>& Json) {
	if (UMaterialFunction* FuncCasted = Cast<UMaterialFunction>(Parent)) Expression->Function = FuncCasted;
	else if (UMaterial* MatCasted = Cast<UMaterial>(Parent)) Expression->Material = MatCasted;

	if (UMaterialExpressionTextureBase* TextureBase = Cast<UMaterialExpressionTextureBase>(Expression)) {
		const TSharedPtr<FJsonObject>* TexturePtr;
		
		if (Json->TryGetObjectField("Texture", TexturePtr)) {
#if ENGINE_MAJOR_VERSION >= 5
			LoadObject(TexturePtr, TextureBase->Texture);
#else
				// For UE4: use a different method of TObjectPtr for the texture
				TObjectPtr<UTexture> TextureObjectPtr;
				LoadObject(TexturePtr, TextureObjectPtr);
				TextureBase->Texture = TextureObjectPtr.Get();
#endif

			Expression->UpdateParameterGuid(true, false);
		}
	}
}

UMaterialExpression* IMaterialGraph::CreateEmptyExpression(UObject* Parent, FName Name, FName Type, FJsonObject* LocalizedObject) {
	if (IgnoredExpressions.Contains(Type.ToString())) // Unhandled expressions
		return nullptr;

	const UClass* Class = FindObject<UClass>(ANY_PACKAGE, *Type.ToString());

	if (!Class) {
#if ENGINE_MAJOR_VERSION >= 5
		TArray<FString> Redirects = TArray{
			FLinkerLoad::FindNewPathNameForClass("/Script/InterchangeImport." + Type.ToString(), false),
			FLinkerLoad::FindNewPathNameForClass("/Script/Landscape." + Type.ToString(), false)
		};
		
		for (FString RedirectedPath : Redirects) {
			if (!RedirectedPath.IsEmpty() && !Class)
				Class = FindObject<UClass>(nullptr, *RedirectedPath);
		}
#endif

		if (!Class) 
			Class = FindObject<UClass>(ANY_PACKAGE, *Type.ToString().Replace(TEXT("MaterialExpressionPhysicalMaterialOutput"), TEXT("MaterialExpressionLandscapePhysicalMaterialOutput")));
	}

	// Show missing nodes in graph
	if (!Class) {
		TSharedPtr<FJsonObject>* ShareObject = new TSharedPtr<FJsonObject>(LocalizedObject);
		MissingNodeClasses.Add(Type.ToString(), ShareObject->Get());

		GLog->Log(*("JsonAsAsset: Missing Node " + Type.ToString() + " in Parent " + Parent->GetName()));
		FNotificationInfo Info = FNotificationInfo(FText::FromString("Missing Node (" + Parent->GetName() + ")"));

		Info.bUseLargeFont = false;
		Info.FadeOutDuration = 2.5f;
		Info.ExpireDuration = 8.0f;
		Info.WidthOverride = FOptionalSize(456);
		Info.bUseThrobber = false;
#if ENGINE_MAJOR_VERSION >= 5
		Info.SubText = FText::FromString(Type.ToString());
#endif

#pragma warning(disable: 4800)
		UClass* MaterialClass = FindObject<UClass>(nullptr, TEXT("/Script/Engine.Material"));
		Info.Image = FSlateIconFinder::FindCustomIconBrushForClass(MaterialClass, TEXT("ClassThumbnail"));

		MaterialGraphNotification = FSlateNotificationManager::Get().AddNotification(Info);
		MaterialGraphNotification.Pin()->SetCompletionState(SNotificationItem::CS_Pending);

		return NewObject<UMaterialExpression>(
			Parent,
			UMaterialExpressionReroute::StaticClass(),
			Name
		);
	}

	return NewObject<UMaterialExpression>
	(
		Parent,
		Class, // Find class using ANY_PACKAGE (may error in the future)
		Name,
		RF_Transactional
	);
}

FExpressionInput IMaterialGraph::PopulateExpressionInput(const FJsonObject* JsonProperties, UMaterialExpression* Expression, const FString& Type) {
	FExpressionInput Input;
	Input.Expression = Expression;

	// Each Mask input/output
	int OutputIndex;
	if (JsonProperties->TryGetNumberField("OutputIndex", OutputIndex)) Input.OutputIndex = OutputIndex;
	FString InputName;
	if (JsonProperties->TryGetStringField("InputName", InputName)) Input.InputName = FName(InputName);
	int Mask;
	if (JsonProperties->TryGetNumberField("Mask", Mask)) Input.Mask = Mask;
	int MaskR;
	if (JsonProperties->TryGetNumberField("MaskR", MaskR)) Input.MaskR = MaskR;
	int MaskG;
	if (JsonProperties->TryGetNumberField("MaskG", MaskG)) Input.MaskG = MaskG;
	int MaskB;
	if (JsonProperties->TryGetNumberField("MaskB", MaskB)) Input.MaskB = MaskB;
	int MaskA;
	if (JsonProperties->TryGetNumberField("MaskA", MaskA)) Input.MaskA = MaskA;

	if (Type == "Color") {
		if (FColorMaterialInput* ColorInput = reinterpret_cast<FColorMaterialInput*>(&Input)) {
			bool UseConstant;
			if (JsonProperties->TryGetBoolField("UseConstant", UseConstant)) ColorInput->UseConstant = UseConstant;
			const TSharedPtr<FJsonObject>* Constant;
			if (JsonProperties->TryGetObjectField("Constant", Constant)) ColorInput->Constant = FMathUtilities::ObjectToLinearColor(Constant->Get()).ToFColor(true);
			Input = FExpressionInput(*ColorInput);
		}
	} else if (Type == "Scalar") {
		if (FScalarMaterialInput* ScalarInput = reinterpret_cast<FScalarMaterialInput*>(&Input)) {
			bool UseConstant;
			if (JsonProperties->TryGetBoolField("UseConstant", UseConstant)) ScalarInput->UseConstant = UseConstant;
#if ENGINE_MAJOR_VERSION >= 5
			float Constant;
#else
			double Constant;
#endif
			if (JsonProperties->TryGetNumberField("Constant", Constant)) ScalarInput->Constant = Constant;
			Input = FExpressionInput(*ScalarInput);
		}
	} else if (Type == "Vector") {
		if (FVectorMaterialInput* VectorInput = reinterpret_cast<FVectorMaterialInput*>(&Input)) {
			bool UseConstant;
			if (JsonProperties->TryGetBoolField("UseConstant", UseConstant)) VectorInput->UseConstant = UseConstant;
			const TSharedPtr<FJsonObject>* Constant;
			if (JsonProperties->TryGetObjectField("Constant", Constant)) VectorInput->Constant = FMathUtilities::ObjectToVector3f(Constant->Get());
			Input = FExpressionInput(*VectorInput);
		}
	}

	return Input;
}

FExpressionOutput IMaterialGraph::PopulateExpressionOutput(const FJsonObject* JsonProperties) {
	FExpressionOutput Output;

	FString OutputName;
	if (JsonProperties->TryGetStringField("OutputName", OutputName)) Output.OutputName = FName(OutputName);
	int Mask;
	if (JsonProperties->TryGetNumberField("Mask", Mask)) Output.Mask = Mask;
	int MaskR;
	if (JsonProperties->TryGetNumberField("MaskR", MaskR)) Output.MaskR = MaskR;
	int MaskG;
	if (JsonProperties->TryGetNumberField("MaskG", MaskG)) Output.MaskG = MaskG;
	int MaskB;
	if (JsonProperties->TryGetNumberField("MaskB", MaskB)) Output.MaskB = MaskB;
	int MaskA;
	if (JsonProperties->TryGetNumberField("MaskA", MaskA)) Output.MaskA = MaskA;

	return Output;
}

FName IMaterialGraph::GetExpressionName(const FJsonObject* JsonProperties, FString OverrideParameterName) {
	const TSharedPtr<FJsonValue> ExpressionField = JsonProperties->TryGetField(OverrideParameterName);

	if (ExpressionField == nullptr || ExpressionField->IsNull()) {
		// Must be from < 4.25
		return FName(JsonProperties->GetStringField("ExpressionName"));
	}

	const TSharedPtr<FJsonObject> ExpressionObject = ExpressionField->AsObject();
	FString ObjectName;
	if (ExpressionObject->TryGetStringField("ObjectName", ObjectName)) {
		return GetExportNameOfSubobject(ObjectName);
	}

	return NAME_None;
}
