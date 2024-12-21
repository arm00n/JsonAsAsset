// Copyright JAA Contributors 2024-2025

#pragma once

#if ENGINE_MAJOR_VERSION >= 5
#include "Styling/AppStyle.h"
using FAppStyle = FAppStyle;
#else
#include "EditorStyleSet.h"
class FAppStyle {
public:
	static const ISlateStyle& Get() {
		return FEditorStyle::Get();
	}

	static FName GetAppStyleSetName() {
		return FEditorStyle::GetStyleSetName();
	}
};

template <typename TObjectType>
class TObjectPtr
{
private:
	TWeakObjectPtr<TObjectType> WeakPtr;

public:
	TObjectPtr() {}
	TObjectPtr(TObjectType* InObject) : WeakPtr(InObject) {}

	TObjectType* Get() const { return WeakPtr.Get(); }

	bool IsValid() const { return WeakPtr.IsValid(); }

	void Reset() { WeakPtr.Reset(); }

	void Set(TObjectType* InObject) { WeakPtr = InObject; }

	// Additional constructor to allow raw pointer conversion
	TObjectPtr(TObjectType* InObject, bool bRawPointer) : WeakPtr(InObject) {}
};
#endif