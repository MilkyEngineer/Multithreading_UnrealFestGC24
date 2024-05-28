// Copyright Alex Stevens (@MilkyEngineer). All Rights Reserved.

#include "ProxyArchiveFormatter.h"

DEFINE_LOG_CATEGORY_STATIC(LogProxyArchiveFormatter, Log, All);

FString Tabber(const int32 NumTabs)
{
	FString Tabs;
	Tabs.Reserve(NumTabs * 4);

	for (int32 i = 0; i < NumTabs; ++i)
	{
		Tabs += TEXT("\t");
	}

	return Tabs;
}

FProxyArchiveFormatter::FProxyArchiveFormatter(FStructuredArchiveFormatter& InPrimary, FStructuredArchiveFormatter& InSecondary)
	: StackDepth(INDEX_NONE)
	, Primary(InPrimary)
	, Secondary(&InSecondary)
{}

FArchive& FProxyArchiveFormatter::GetUnderlyingArchive()
{
	return Primary.GetUnderlyingArchive();
}

bool FProxyArchiveFormatter::HasDocumentTree() const
{
	return true;
}

void FProxyArchiveFormatter::EnterRecord()
{
	UE_LOG(LogProxyArchiveFormatter, Verbose, TEXT("%llu: %s%hs"), GetUnderlyingArchive().Tell(), *Tabber(StackDepth), __FUNCTION__);

	++StackDepth;
	Primary.EnterRecord();
	Secondary->EnterRecord();
}

void FProxyArchiveFormatter::LeaveRecord()
{
	--StackDepth;
	UE_LOG(LogProxyArchiveFormatter, Verbose, TEXT("%llu: %s%hs"), GetUnderlyingArchive().Tell(), *Tabber(StackDepth), __FUNCTION__);

	Primary.LeaveRecord();
	Secondary->LeaveRecord();
}

void FProxyArchiveFormatter::EnterField(FArchiveFieldName Name)
{
	UE_LOG(LogProxyArchiveFormatter, Verbose, TEXT("%llu: %s%hs: %s"), GetUnderlyingArchive().Tell(), *Tabber(StackDepth), __FUNCTION__, Name.Name);

	Primary.EnterField(Name);
	Secondary->EnterField(Name);
}

void FProxyArchiveFormatter::LeaveField()
{
	UE_LOG(LogProxyArchiveFormatter, Verbose, TEXT("%llu: %s%hs"), GetUnderlyingArchive().Tell(), *Tabber(StackDepth), __FUNCTION__);

	Primary.LeaveField();
	Secondary->LeaveField();
}

bool FProxyArchiveFormatter::TryEnterField(FArchiveFieldName Name, bool bEnterWhenWriting)
{
	UE_LOG(LogProxyArchiveFormatter, Verbose, TEXT("%llu: %s%hs: %s %i"), GetUnderlyingArchive().Tell(), *Tabber(StackDepth), __FUNCTION__, Name.Name, bEnterWhenWriting);

	return Primary.TryEnterField(Name, bEnterWhenWriting) && Secondary->TryEnterField(Name, bEnterWhenWriting);
}

void FProxyArchiveFormatter::EnterArray(int32& NumElements)
{
	UE_LOG(LogProxyArchiveFormatter, Verbose, TEXT("%llu: %s%hs: %i"), GetUnderlyingArchive().Tell(), *Tabber(StackDepth), __FUNCTION__, NumElements);

	Primary.EnterArray(NumElements);
	Secondary->EnterArray(NumElements);
}

void FProxyArchiveFormatter::LeaveArray()
{
	UE_LOG(LogProxyArchiveFormatter, Verbose, TEXT("%llu: %s%hs"), GetUnderlyingArchive().Tell(), *Tabber(StackDepth), __FUNCTION__);

	Primary.LeaveArray();
	Secondary->LeaveArray();
}

void FProxyArchiveFormatter::EnterArrayElement()
{
	UE_LOG(LogProxyArchiveFormatter, Verbose, TEXT("%llu: %s%hs"), GetUnderlyingArchive().Tell(), *Tabber(StackDepth), __FUNCTION__);

	Primary.EnterArrayElement();
	Secondary->EnterArrayElement();
}

void FProxyArchiveFormatter::LeaveArrayElement()
{
	UE_LOG(LogProxyArchiveFormatter, Verbose, TEXT("%llu: %s%hs"), GetUnderlyingArchive().Tell(), *Tabber(StackDepth), __FUNCTION__);

	Primary.LeaveArrayElement();
	Secondary->LeaveArrayElement();
}

void FProxyArchiveFormatter::EnterStream()
{
	UE_LOG(LogProxyArchiveFormatter, Verbose, TEXT("%llu: %s%hs"), GetUnderlyingArchive().Tell(), *Tabber(StackDepth), __FUNCTION__);

	Primary.EnterStream();
	Secondary->EnterStream();
}

void FProxyArchiveFormatter::LeaveStream()
{
	UE_LOG(LogProxyArchiveFormatter, Verbose, TEXT("%llu: %s%hs"), GetUnderlyingArchive().Tell(), *Tabber(StackDepth), __FUNCTION__);

	Primary.LeaveStream();
	Secondary->LeaveStream();
}

void FProxyArchiveFormatter::EnterStreamElement()
{
	UE_LOG(LogProxyArchiveFormatter, Verbose, TEXT("%llu: %s%hs"), GetUnderlyingArchive().Tell(), *Tabber(StackDepth), __FUNCTION__);

	Primary.EnterStreamElement();
	Secondary->EnterStreamElement();
}

void FProxyArchiveFormatter::LeaveStreamElement()
{
	UE_LOG(LogProxyArchiveFormatter, Verbose, TEXT("%llu: %s%hs"), GetUnderlyingArchive().Tell(), *Tabber(StackDepth), __FUNCTION__);

	Primary.LeaveStreamElement();
	Secondary->LeaveStreamElement();
}

void FProxyArchiveFormatter::EnterMap(int32& NumElements)
{
	UE_LOG(LogProxyArchiveFormatter, Verbose, TEXT("%llu: %s%hs: %i"), GetUnderlyingArchive().Tell(), *Tabber(StackDepth), __FUNCTION__, NumElements);

	++StackDepth;
	Primary.EnterMap(NumElements);
	Secondary->EnterMap(NumElements);
}

void FProxyArchiveFormatter::LeaveMap()
{
	--StackDepth;
	UE_LOG(LogProxyArchiveFormatter, Verbose, TEXT("%llu: %s%hs"), GetUnderlyingArchive().Tell(), *Tabber(StackDepth), __FUNCTION__);

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
	UE_LOG(LogProxyArchiveFormatter, Verbose, TEXT("%llu: %s%hs"), GetUnderlyingArchive().Tell(), *Tabber(StackDepth), __FUNCTION__);

	Primary.LeaveMapElement();
	Secondary->LeaveMapElement();
}

void FProxyArchiveFormatter::EnterAttributedValue()
{
	UE_LOG(LogProxyArchiveFormatter, Verbose, TEXT("%llu: %s%hs"), GetUnderlyingArchive().Tell(), *Tabber(StackDepth), __FUNCTION__);

	++StackDepth;
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
	UE_LOG(LogProxyArchiveFormatter, Verbose, TEXT("%llu: %s%hs"), GetUnderlyingArchive().Tell(), *Tabber(StackDepth), __FUNCTION__);

	Primary.LeaveAttribute();
	Secondary->LeaveAttribute();
}

void FProxyArchiveFormatter::EnterAttributedValueValue()
{
	UE_LOG(LogProxyArchiveFormatter, Verbose, TEXT("%llu: %s%hs"), GetUnderlyingArchive().Tell(), *Tabber(StackDepth), __FUNCTION__);

	Primary.EnterAttributedValueValue();
	Secondary->EnterAttributedValueValue();
}

void FProxyArchiveFormatter::LeaveAttributedValue()
{
	--StackDepth;
	UE_LOG(LogProxyArchiveFormatter, Verbose, TEXT("%llu: %s%hs"), GetUnderlyingArchive().Tell(), *Tabber(StackDepth), __FUNCTION__);

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

	UE_LOG(LogProxyArchiveFormatter, Verbose, TEXT("%llu: %s%hs %u"), GetUnderlyingArchive().Tell(), *Tabber(StackDepth), __FUNCSIG__, Value);
}

void FProxyArchiveFormatter::Serialize(uint16& Value)
{
	Primary.Serialize(Value);
	Secondary->Serialize(Value);

	UE_LOG(LogProxyArchiveFormatter, Verbose, TEXT("%llu: %s%hs %u"), GetUnderlyingArchive().Tell(), *Tabber(StackDepth), __FUNCSIG__, Value);
}

void FProxyArchiveFormatter::Serialize(uint32& Value)
{
	Primary.Serialize(Value);
	Secondary->Serialize(Value);

	UE_LOG(LogProxyArchiveFormatter, Verbose, TEXT("%llu: %s%hs %u"), GetUnderlyingArchive().Tell(), *Tabber(StackDepth), __FUNCSIG__, Value);
}

void FProxyArchiveFormatter::Serialize(uint64& Value)
{
	Primary.Serialize(Value);
	Secondary->Serialize(Value);

	UE_LOG(LogProxyArchiveFormatter, Verbose, TEXT("%llu: %s%hs %llu"), GetUnderlyingArchive().Tell(), *Tabber(StackDepth), __FUNCSIG__, Value);
}

void FProxyArchiveFormatter::Serialize(int8& Value)
{
	Primary.Serialize(Value);
	Secondary->Serialize(Value);

	UE_LOG(LogProxyArchiveFormatter, Verbose, TEXT("%llu: %s%hs %i"), GetUnderlyingArchive().Tell(), *Tabber(StackDepth), __FUNCSIG__, Value);
}

void FProxyArchiveFormatter::Serialize(int16& Value)
{
	Primary.Serialize(Value);
	Secondary->Serialize(Value);

	UE_LOG(LogProxyArchiveFormatter, Verbose, TEXT("%llu: %s%hs %i"), GetUnderlyingArchive().Tell(), *Tabber(StackDepth), __FUNCSIG__, Value);
}

void FProxyArchiveFormatter::Serialize(int32& Value)
{
	Primary.Serialize(Value);
	Secondary->Serialize(Value);

	UE_LOG(LogProxyArchiveFormatter, Verbose, TEXT("%llu: %s%hs %i"), GetUnderlyingArchive().Tell(), *Tabber(StackDepth), __FUNCSIG__, Value);
}

void FProxyArchiveFormatter::Serialize(int64& Value)
{
	Primary.Serialize(Value);
	Secondary->Serialize(Value);

	UE_LOG(LogProxyArchiveFormatter, Verbose, TEXT("%llu: %s%hs %lld"), GetUnderlyingArchive().Tell(), *Tabber(StackDepth), __FUNCSIG__, Value);
}

void FProxyArchiveFormatter::Serialize(float& Value)
{
	Primary.Serialize(Value);
	Secondary->Serialize(Value);

	UE_LOG(LogProxyArchiveFormatter, Verbose, TEXT("%llu: %s%hs %f"), GetUnderlyingArchive().Tell(), *Tabber(StackDepth), __FUNCSIG__, Value);
}

void FProxyArchiveFormatter::Serialize(double& Value)
{
	Primary.Serialize(Value);
	Secondary->Serialize(Value);

	UE_LOG(LogProxyArchiveFormatter, Verbose, TEXT("%llu: %s%hs %f"), GetUnderlyingArchive().Tell(), *Tabber(StackDepth), __FUNCSIG__, Value);
}

void FProxyArchiveFormatter::Serialize(bool& Value)
{
	Primary.Serialize(Value);
	Secondary->Serialize(Value);

	UE_LOG(LogProxyArchiveFormatter, Verbose, TEXT("%llu: %s%hs Bool %i"), GetUnderlyingArchive().Tell(), *Tabber(StackDepth), __FUNCSIG__, Value);
}

void FProxyArchiveFormatter::Serialize(FString& Value)
{
	Primary.Serialize(Value);
	Secondary->Serialize(Value);

	UE_LOG(LogProxyArchiveFormatter, Verbose, TEXT("%llu: %s%hs %s"), GetUnderlyingArchive().Tell(), *Tabber(StackDepth), __FUNCSIG__, *Value);
}

void FProxyArchiveFormatter::Serialize(FName& Value)
{
	Primary.Serialize(Value);
	Secondary->Serialize(Value);

	UE_LOG(LogProxyArchiveFormatter, Verbose, TEXT("%llu: %s%hs %s"), GetUnderlyingArchive().Tell(), *Tabber(StackDepth), __FUNCSIG__, *Value.ToString());
}

void FProxyArchiveFormatter::Serialize(UObject*& Value)
{
	Primary.Serialize(Value);
	Secondary->Serialize(Value);

	UE_LOG(LogProxyArchiveFormatter, Verbose, TEXT("%llu: %s%hs %s"), GetUnderlyingArchive().Tell(), *Tabber(StackDepth), __FUNCSIG__, *GetNameSafe(Value));
}

void FProxyArchiveFormatter::Serialize(FText& Value)
{
	Primary.Serialize(Value);
	Secondary->Serialize(Value);

	UE_LOG(LogProxyArchiveFormatter, Verbose, TEXT("%llu: %s%hs %s"), GetUnderlyingArchive().Tell(), *Tabber(StackDepth), __FUNCSIG__, *Value.ToString());
}

void FProxyArchiveFormatter::Serialize(FWeakObjectPtr& Value)
{
	Primary.Serialize(Value);
	Secondary->Serialize(Value);

	UE_LOG(LogProxyArchiveFormatter, Verbose, TEXT("%llu: %s%hs %s"), GetUnderlyingArchive().Tell(), *Tabber(StackDepth), __FUNCSIG__, *GetNameSafe(Value.Get()));
}

void FProxyArchiveFormatter::Serialize(FSoftObjectPtr& Value)
{
	Primary.Serialize(Value);
	Secondary->Serialize(Value);

	UE_LOG(LogProxyArchiveFormatter, Verbose, TEXT("%llu: %s%hs %s"), GetUnderlyingArchive().Tell(), *Tabber(StackDepth), __FUNCSIG__, *Value.ToString());
}

void FProxyArchiveFormatter::Serialize(FSoftObjectPath& Value)
{
	Primary.Serialize(Value);
	Secondary->Serialize(Value);

	UE_LOG(LogProxyArchiveFormatter, Verbose, TEXT("%llu: %s%hs %s"), GetUnderlyingArchive().Tell(), *Tabber(StackDepth), __FUNCSIG__, *Value.ToString());
}

void FProxyArchiveFormatter::Serialize(FLazyObjectPtr& Value)
{
	Primary.Serialize(Value);
	Secondary->Serialize(Value);

	UE_LOG(LogProxyArchiveFormatter, Verbose, TEXT("%llu: %s%hs %s"), GetUnderlyingArchive().Tell(), *Tabber(StackDepth), __FUNCSIG__, *GetNameSafe(Value.Get()));
}

void FProxyArchiveFormatter::Serialize(FObjectPtr& Value)
{
	Primary.Serialize(Value);
	Secondary->Serialize(Value);

	UE_LOG(LogProxyArchiveFormatter, Verbose, TEXT("%llu: %s%hs %s"), GetUnderlyingArchive().Tell(), *Tabber(StackDepth), __FUNCSIG__, *GetNameSafe(Value.Get()));
}

void FProxyArchiveFormatter::Serialize(TArray<uint8>& Value)
{
	Primary.Serialize(Value);
	Secondary->Serialize(Value);

	UE_LOG(LogProxyArchiveFormatter, Verbose, TEXT("%llu: %s%hs Data %i bytes"), GetUnderlyingArchive().Tell(), *Tabber(StackDepth), __FUNCSIG__, Value.Num());
}

void FProxyArchiveFormatter::Serialize(void* Data, uint64 DataSize)
{
	Primary.Serialize(Data, DataSize);
	Secondary->Serialize(Data, DataSize);

	UE_LOG(LogProxyArchiveFormatter, Verbose, TEXT("%llu: %s%hs Data %llu bytes"), GetUnderlyingArchive().Tell(), *Tabber(StackDepth), __FUNCSIG__, DataSize);
}
