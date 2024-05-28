// Copyright Alex Stevens (@MilkyEngineer). All Rights Reserved.

#pragma once

#include "Serialization/StructuredArchiveFormatter.h"

class FNullArchiveFormatter final : public FStructuredArchiveFormatter
{
	static FArchive NullArchive;
	static FNullArchiveFormatter NullFormatter;

public:
	static FStructuredArchiveFormatter& Get()
	{
		return NullFormatter;
	}

	virtual FArchive& GetUnderlyingArchive() override
	{
		return NullArchive;
	}

	virtual bool HasDocumentTree() const override { return false; }
	virtual void EnterRecord() override {}
	virtual void LeaveRecord() override {}
	virtual void EnterField(FArchiveFieldName Name) override {}
	virtual void LeaveField() override {}
	virtual bool TryEnterField(FArchiveFieldName Name, bool bEnterWhenWriting) override { return true; }
	virtual void EnterArray(int32& NumElements) override {}
	virtual void LeaveArray() override {}
	virtual void EnterArrayElement() override {}
	virtual void LeaveArrayElement() override {}
	virtual void EnterStream() override {}
	virtual void LeaveStream() override {}
	virtual void EnterStreamElement() override {}
	virtual void LeaveStreamElement() override {}
	virtual void EnterMap(int32& NumElements) override {}
	virtual void LeaveMap() override {}
	virtual void EnterMapElement(FString& Name) override {}
	virtual void LeaveMapElement() override {}
	virtual void EnterAttributedValue() override {}
	virtual void EnterAttribute(FArchiveFieldName AttributeName) override {}
	virtual void LeaveAttribute() override {}
	virtual void EnterAttributedValueValue() override {}
	virtual void LeaveAttributedValue() override {}
	virtual bool TryEnterAttribute(FArchiveFieldName AttributeName, bool bEnterWhenWriting) override { return true; }
	virtual bool TryEnterAttributedValueValue() override { return true; }
	virtual void Serialize(uint8& Value) override {}
	virtual void Serialize(uint16& Value) override {}
	virtual void Serialize(uint32& Value) override {}
	virtual void Serialize(uint64& Value) override {}
	virtual void Serialize(int8& Value) override {}
	virtual void Serialize(int16& Value) override {}
	virtual void Serialize(int32& Value) override {}
	virtual void Serialize(int64& Value) override {}
	virtual void Serialize(float& Value) override {}
	virtual void Serialize(double& Value) override {}
	virtual void Serialize(bool& Value) override {}
	virtual void Serialize(FString& Value) override {}
	virtual void Serialize(FName& Value) override {}
	virtual void Serialize(UObject*& Value) override {}
	virtual void Serialize(FText& Value) override {}
	virtual void Serialize(FWeakObjectPtr& Value) override {}
	virtual void Serialize(FSoftObjectPtr& Value) override {}
	virtual void Serialize(FSoftObjectPath& Value) override {}
	virtual void Serialize(FLazyObjectPtr& Value) override {}
	virtual void Serialize(FObjectPtr& Value) override {}
	virtual void Serialize(TArray<uint8>& Value) override {}
	virtual void Serialize(void* Data, uint64 DataSize) override {}
};

FArchive FNullArchiveFormatter::NullArchive;
FNullArchiveFormatter FNullArchiveFormatter::NullFormatter;
