// Copyright Alex Stevens (@MilkyEngineer). All Rights Reserved.

#include "ProxyArchiveFormatter.h"

FProxyArchiveFormatter::FProxyArchiveFormatter(FStructuredArchiveFormatter& InPrimary, FStructuredArchiveFormatter& InSecondary)
	: Primary(InPrimary)
	, Secondary(&InSecondary)
{}

FArchive& FProxyArchiveFormatter::GetUnderlyingArchive()
{
	return Primary.GetUnderlyingArchive();
}

bool FProxyArchiveFormatter::HasDocumentTree() const
{
	return Primary.HasDocumentTree() || Secondary->HasDocumentTree();
}

void FProxyArchiveFormatter::EnterRecord()
{
	Primary.EnterRecord();
	Secondary->EnterRecord();
}

void FProxyArchiveFormatter::LeaveRecord()
{
	Primary.LeaveRecord();
	Secondary->LeaveRecord();
}

void FProxyArchiveFormatter::EnterField(FArchiveFieldName Name)
{
	Primary.EnterField(Name);
	Secondary->EnterField(Name);
}

void FProxyArchiveFormatter::LeaveField()
{
	Primary.LeaveField();
	Secondary->LeaveField();
}

bool FProxyArchiveFormatter::TryEnterField(FArchiveFieldName Name, bool bEnterWhenWriting)
{
	return Primary.TryEnterField(Name, bEnterWhenWriting) && Secondary->TryEnterField(Name, bEnterWhenWriting);
}

void FProxyArchiveFormatter::EnterArray(int32& NumElements)
{
	Primary.EnterArray(NumElements);
	Secondary->EnterArray(NumElements);
}

void FProxyArchiveFormatter::LeaveArray()
{
	Primary.LeaveArray();
	Secondary->LeaveArray();
}

void FProxyArchiveFormatter::EnterArrayElement()
{
	Primary.EnterArrayElement();
	Secondary->EnterArrayElement();
}

void FProxyArchiveFormatter::LeaveArrayElement()
{
	Primary.LeaveArrayElement();
	Secondary->LeaveArrayElement();
}

void FProxyArchiveFormatter::EnterStream()
{
	Primary.EnterStream();
	Secondary->EnterStream();
}

void FProxyArchiveFormatter::LeaveStream()
{
	Primary.LeaveStream();
	Secondary->LeaveStream();
}

void FProxyArchiveFormatter::EnterStreamElement()
{
	Primary.EnterStreamElement();
	Secondary->EnterStreamElement();
}

void FProxyArchiveFormatter::LeaveStreamElement()
{
	Primary.LeaveStreamElement();
	Secondary->LeaveStreamElement();
}

void FProxyArchiveFormatter::EnterMap(int32& NumElements)
{
	Primary.EnterMap(NumElements);
	Secondary->EnterMap(NumElements);
}

void FProxyArchiveFormatter::LeaveMap()
{
	Primary.LeaveMap();
	Secondary->LeaveMap();
}

void FProxyArchiveFormatter::EnterMapElement(FString& Name)
{
	Primary.EnterMapElement(Name);
	Secondary->EnterMapElement(Name);
}

void FProxyArchiveFormatter::LeaveMapElement()
{
	Primary.LeaveMapElement();
	Secondary->LeaveMapElement();
}

void FProxyArchiveFormatter::EnterAttributedValue()
{
	Primary.EnterAttributedValue();
	Secondary->EnterAttributedValue();
}

void FProxyArchiveFormatter::EnterAttribute(FArchiveFieldName AttributeName)
{
	Primary.EnterAttribute(AttributeName);
	Secondary->EnterAttribute(AttributeName);
}

void FProxyArchiveFormatter::LeaveAttribute()
{
	Primary.LeaveAttribute();
	Secondary->LeaveAttribute();
}

void FProxyArchiveFormatter::EnterAttributedValueValue()
{
	Primary.EnterAttributedValueValue();
	Secondary->EnterAttributedValueValue();
}

void FProxyArchiveFormatter::LeaveAttributedValue()
{
	Primary.LeaveAttributedValue();
	Secondary->LeaveAttributedValue();
}

bool FProxyArchiveFormatter::TryEnterAttribute(FArchiveFieldName AttributeName, bool bEnterWhenWriting)
{
	return Primary.TryEnterAttribute(AttributeName, bEnterWhenWriting) && Secondary->TryEnterAttribute(AttributeName, bEnterWhenWriting);
}

bool FProxyArchiveFormatter::TryEnterAttributedValueValue()
{
	return Primary.TryEnterAttributedValueValue() && Secondary->TryEnterAttributedValueValue();
}

void FProxyArchiveFormatter::Serialize(uint8& Value)
{
	Primary.Serialize(Value);
	Secondary->Serialize(Value);
}

void FProxyArchiveFormatter::Serialize(uint16& Value)
{
	Primary.Serialize(Value);
	Secondary->Serialize(Value);
}

void FProxyArchiveFormatter::Serialize(uint32& Value)
{
	Primary.Serialize(Value);
	Secondary->Serialize(Value);
}

void FProxyArchiveFormatter::Serialize(uint64& Value)
{
	Primary.Serialize(Value);
	Secondary->Serialize(Value);
}

void FProxyArchiveFormatter::Serialize(int8& Value)
{
	Primary.Serialize(Value);
	Secondary->Serialize(Value);
}

void FProxyArchiveFormatter::Serialize(int16& Value)
{
	Primary.Serialize(Value);
	Secondary->Serialize(Value);
}

void FProxyArchiveFormatter::Serialize(int32& Value)
{
	Primary.Serialize(Value);
	Secondary->Serialize(Value);
}

void FProxyArchiveFormatter::Serialize(int64& Value)
{
	Primary.Serialize(Value);
	Secondary->Serialize(Value);
}

void FProxyArchiveFormatter::Serialize(float& Value)
{
	Primary.Serialize(Value);
	Secondary->Serialize(Value);
}

void FProxyArchiveFormatter::Serialize(double& Value)
{
	Primary.Serialize(Value);
	Secondary->Serialize(Value);
}

void FProxyArchiveFormatter::Serialize(bool& Value)
{
	Primary.Serialize(Value);
	Secondary->Serialize(Value);
}

void FProxyArchiveFormatter::Serialize(FString& Value)
{
	Primary.Serialize(Value);
	Secondary->Serialize(Value);
}

void FProxyArchiveFormatter::Serialize(FName& Value)
{
	Primary.Serialize(Value);
	Secondary->Serialize(Value);
}

void FProxyArchiveFormatter::Serialize(UObject*& Value)
{
	Primary.Serialize(Value);
	Secondary->Serialize(Value);
}

void FProxyArchiveFormatter::Serialize(FText& Value)
{
	Primary.Serialize(Value);
	Secondary->Serialize(Value);
}

void FProxyArchiveFormatter::Serialize(FWeakObjectPtr& Value)
{
	Primary.Serialize(Value);
	Secondary->Serialize(Value);
}

void FProxyArchiveFormatter::Serialize(FSoftObjectPtr& Value)
{
	Primary.Serialize(Value);
	Secondary->Serialize(Value);
}

void FProxyArchiveFormatter::Serialize(FSoftObjectPath& Value)
{
	Primary.Serialize(Value);
	Secondary->Serialize(Value);
}

void FProxyArchiveFormatter::Serialize(FLazyObjectPtr& Value)
{
	Primary.Serialize(Value);
	Secondary->Serialize(Value);
}

void FProxyArchiveFormatter::Serialize(FObjectPtr& Value)
{
	Primary.Serialize(Value);
	Secondary->Serialize(Value);
}

void FProxyArchiveFormatter::Serialize(TArray<uint8>& Value)
{
	Primary.Serialize(Value);
	Secondary->Serialize(Value);
}

void FProxyArchiveFormatter::Serialize(void* Data, uint64 DataSize)
{
	Primary.Serialize(Data, DataSize);
	Secondary->Serialize(Data, DataSize);
}
