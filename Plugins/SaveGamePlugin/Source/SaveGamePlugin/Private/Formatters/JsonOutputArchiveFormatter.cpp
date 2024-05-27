// Copyright Alex Stevens (@MilkyEngineer). All Rights Reserved.

#include "Formatters/JsonOutputArchiveFormatter.h"

#if WITH_TEXT_ARCHIVE_SUPPORT
#include "DOM/JsonObject.h"

DEFINE_LOG_CATEGORY_STATIC(LogJsonOutputArchiveFormatter, Log, All);

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

FArchive& FJsonOutputArchiveFormatter::GetUnderlyingArchive()
{
	static FArchive NullArchive;
	return NullArchive;
}

bool FJsonOutputArchiveFormatter::HasDocumentTree() const
{
	return true;
}

void FJsonOutputArchiveFormatter::EnterRecord()
{
	UE_LOG(LogJsonOutputArchiveFormatter, Verbose, TEXT("%s%hs"), *Tabber(Stack.Num()), __FUNCTION__);
	TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();

	if (Stack.Num() > 0)
	{
		FStackObject& Current = GetCurrent();

		check(!Current.Field.IsEmpty());
		Current.Object->SetObjectField(Current.Field, Object);
	}
	else
	{
		check(!RootObject.IsValid());
		RootObject = Object;
	}

	Stack.Push(Object);
}

void FJsonOutputArchiveFormatter::LeaveRecord()
{
	Stack.Pop(false);
	UE_LOG(LogJsonOutputArchiveFormatter, Verbose, TEXT("%s%hs"), *Tabber(Stack.Num()), __FUNCTION__);
}

void FJsonOutputArchiveFormatter::EnterField(FArchiveFieldName Name)
{
	FStackObject& Current = GetCurrent();
	check(!Current.bInStream);
	Current.Field = FString(Name.Name);

	UE_LOG(LogJsonOutputArchiveFormatter, Verbose, TEXT("%s%hs: %s"), *Tabber(Stack.Num()), __FUNCTION__, Name.Name);
}

void FJsonOutputArchiveFormatter::LeaveField()
{
	FStackObject& Current = GetCurrent();
	check(!Current.Field.IsEmpty());

	UE_LOG(LogJsonOutputArchiveFormatter, Verbose, TEXT("%s%hs: %s"), *Tabber(Stack.Num()), __FUNCTION__, *Current.Field);
	Current.bInAttributedValueValue = false;
	Current.Field.Reset();
}

bool FJsonOutputArchiveFormatter::TryEnterField(FArchiveFieldName Name, bool bEnterWhenWriting)
{
	UE_LOG(LogJsonOutputArchiveFormatter, Verbose, TEXT("%s%hs: %s"), *Tabber(Stack.Num()), __FUNCTION__, Name.Name);
	EnterField(Name);
	return true;
}

void FJsonOutputArchiveFormatter::EnterArray(int32& NumElements)
{
	UE_LOG(LogJsonOutputArchiveFormatter, Verbose, TEXT("%s%hs: %d"), *Tabber(Stack.Num()), __FUNCTION__, NumElements);
	GetCurrent().StreamValues.Reserve(NumElements);
	EnterStream();
}

void FJsonOutputArchiveFormatter::LeaveArray()
{
	UE_LOG(LogJsonOutputArchiveFormatter, Verbose, TEXT("%s%hs"), *Tabber(Stack.Num()), __FUNCTION__);
	LeaveStream();
}

void FJsonOutputArchiveFormatter::EnterArrayElement()
{
	UE_LOG(LogJsonOutputArchiveFormatter, Verbose, TEXT("%s%hs"), *Tabber(Stack.Num()), __FUNCTION__);
	EnterStreamElement();
}

void FJsonOutputArchiveFormatter::LeaveArrayElement()
{
	UE_LOG(LogJsonOutputArchiveFormatter, Verbose, TEXT("%s%hs"), *Tabber(Stack.Num()), __FUNCTION__);
	LeaveStreamElement();
}

void FJsonOutputArchiveFormatter::EnterStream()
{
	UE_LOG(LogJsonOutputArchiveFormatter, Verbose, TEXT("%s%hs"), *Tabber(Stack.Num()), __FUNCTION__);
	FStackObject& Current = GetCurrent();
	Current.bInStream = true;
}

void FJsonOutputArchiveFormatter::LeaveStream()
{
	FStackObject& Current = GetCurrent();
	Current.bInStream = false;
	Current.Object->SetArrayField(Current.Field, Current.StreamValues);
	Current.StreamValues.Reset();
	UE_LOG(LogJsonOutputArchiveFormatter, Verbose, TEXT("%s%hs"), *Tabber(Stack.Num()), __FUNCTION__);
}

void FJsonOutputArchiveFormatter::EnterStreamElement()
{
	UE_LOG(LogJsonOutputArchiveFormatter, Verbose, TEXT("%s%hs"), *Tabber(Stack.Num()), __FUNCTION__);
}

void FJsonOutputArchiveFormatter::LeaveStreamElement()
{
	UE_LOG(LogJsonOutputArchiveFormatter, Verbose, TEXT("%s%hs"), *Tabber(Stack.Num()), __FUNCTION__);
}

void FJsonOutputArchiveFormatter::EnterMap(int32& NumElements)
{
	UE_LOG(LogJsonOutputArchiveFormatter, Verbose, TEXT("%s%hs: %i"), *Tabber(Stack.Num()), __FUNCTION__, NumElements);
	EnterRecord();
}

void FJsonOutputArchiveFormatter::LeaveMap()
{
	LeaveRecord();
	UE_LOG(LogJsonOutputArchiveFormatter, Verbose, TEXT("%s%hs"), *Tabber(Stack.Num()), __FUNCTION__);
}

void FJsonOutputArchiveFormatter::EnterMapElement(FString& Name)
{
	UE_LOG(LogJsonOutputArchiveFormatter, Verbose, TEXT("%s%hs"), *Tabber(Stack.Num()), __FUNCTION__);
	EnterField(*Name);
}

void FJsonOutputArchiveFormatter::LeaveMapElement()
{
	LeaveField();
	UE_LOG(LogJsonOutputArchiveFormatter, Verbose, TEXT("%s%hs"), *Tabber(Stack.Num()), __FUNCTION__);
}

void FJsonOutputArchiveFormatter::EnterAttributedValue()
{
	UE_LOG(LogJsonOutputArchiveFormatter, Verbose, TEXT("%s%hs"), *Tabber(Stack.Num()), __FUNCTION__);
	EnterRecord();
}

void FJsonOutputArchiveFormatter::LeaveAttributedValue()
{
	LeaveRecord();
	UE_LOG(LogJsonOutputArchiveFormatter, Verbose, TEXT("%s%hs"), *Tabber(Stack.Num()), __FUNCTION__);
}

void FJsonOutputArchiveFormatter::EnterAttribute(FArchiveFieldName AttributeName)
{
	UE_LOG(LogJsonOutputArchiveFormatter, Verbose, TEXT("%s%hs"), *Tabber(Stack.Num()), __FUNCTION__);
	EnterField(AttributeName);
}

void FJsonOutputArchiveFormatter::LeaveAttribute()
{
	LeaveField();
	UE_LOG(LogJsonOutputArchiveFormatter, Verbose, TEXT("%s%hs"), *Tabber(Stack.Num()), __FUNCTION__);
}

void FJsonOutputArchiveFormatter::EnterAttributedValueValue()
{
	UE_LOG(LogJsonOutputArchiveFormatter, Verbose, TEXT("%s%hs"), *Tabber(Stack.Num()), __FUNCTION__);
	GetCurrent().bInAttributedValueValue = true;
	EnterField(FArchiveFieldName(TEXT("_Value")));
}

bool FJsonOutputArchiveFormatter::TryEnterAttribute(FArchiveFieldName AttributeName, bool bEnterWhenWriting)
{
	UE_LOG(LogJsonOutputArchiveFormatter, Verbose, TEXT("%s%hs: %s"), *Tabber(Stack.Num()), __FUNCTION__, AttributeName.Name);
	EnterField(AttributeName);
	return true;
}

bool FJsonOutputArchiveFormatter::TryEnterAttributedValueValue()
{
	UE_LOG(LogJsonOutputArchiveFormatter, Verbose, TEXT("%s%hs"), *Tabber(Stack.Num()), __FUNCTION__);
	EnterAttributedValueValue();
	return true;
}

void FJsonOutputArchiveFormatter::Serialize(uint8& Value)
{
	SetNumberValue(Value);
}

void FJsonOutputArchiveFormatter::Serialize(uint16& Value)
{
	SetNumberValue(Value);
}

void FJsonOutputArchiveFormatter::Serialize(uint32& Value)
{
	SetNumberValue(Value);
}

void FJsonOutputArchiveFormatter::Serialize(uint64& Value)
{
	SetNumberValue(Value);
}

void FJsonOutputArchiveFormatter::Serialize(int8& Value)
{
	SetNumberValue(Value);
}

void FJsonOutputArchiveFormatter::Serialize(int16& Value)
{
	SetNumberValue(Value);
}

void FJsonOutputArchiveFormatter::Serialize(int32& Value)
{
	SetNumberValue(Value);
}

void FJsonOutputArchiveFormatter::Serialize(int64& Value)
{
	SetNumberValue(Value);
}

void FJsonOutputArchiveFormatter::Serialize(float& Value)
{
	SetNumberValue(Value);
}

void FJsonOutputArchiveFormatter::Serialize(double& Value)
{
	SetNumberValue(Value);
}

void FJsonOutputArchiveFormatter::Serialize(bool& Value)
{
	SetNumberValue(Value);
}

void FJsonOutputArchiveFormatter::Serialize(FString& Value)
{
	SetValue<FJsonValueString>(Value);
}

void FJsonOutputArchiveFormatter::Serialize(FName& Value)
{
	SetValue<FJsonValueString>(Value.ToString());
}

void FJsonOutputArchiveFormatter::Serialize(UObject*& Value)
{
	Serialize(MoveTemp(Value));
}

void FJsonOutputArchiveFormatter::Serialize(FText& Value)
{
	SetValue<FJsonValueString>(Value.ToString());
}

void FJsonOutputArchiveFormatter::Serialize(FWeakObjectPtr& Value)
{
	Serialize(Value.Get());
}

void FJsonOutputArchiveFormatter::Serialize(FSoftObjectPtr& Value)
{
	SetValue<FJsonValueString>(Value.ToString());
}

void FJsonOutputArchiveFormatter::Serialize(FSoftObjectPath& Value)
{
	SetValue<FJsonValueString>(Value.ToString());
}

void FJsonOutputArchiveFormatter::Serialize(FLazyObjectPtr& Value)
{
	Serialize(Value.Get());
}

void FJsonOutputArchiveFormatter::Serialize(FObjectPtr& Value)
{
	Serialize(Value.Get());
}

void FJsonOutputArchiveFormatter::Serialize(TArray<uint8>& Value)
{
	SetValue<FJsonValueString>(FBase64::Encode(Value));
}

void FJsonOutputArchiveFormatter::Serialize(void* Data, uint64 DataSize)
{
	SetValue<FJsonValueString>(FBase64::Encode(static_cast<const uint8*>(Data), DataSize));
}

void FJsonOutputArchiveFormatter::Serialize(const TSharedRef<FJsonObject>& Value)
{
	FStackObject& Current = GetCurrent();

	// Replace the current record
	check(Current.Object->Values.Num() == 0);
	Current.Object = Value;

	if (Stack.Num() > 1)
	{
		// We've got a parent, ensure its field is replaced with the new object
		FStackObject& Parent = Stack.Last(1);

		check(!Parent.Field.IsEmpty());
		Parent.Object->SetObjectField(Parent.Field, Value);
	}
}

void FJsonOutputArchiveFormatter::Serialize(const UObject* Value)
{
	if (Value)
	{
		SetValue<FJsonValueString>(Value->GetPathName());
	}
	else
	{
		SetValue(MakeShared<FJsonValueNull>());
	}
}

template <typename ValueType>
void FJsonOutputArchiveFormatter::SetNumberValue(const ValueType& Value)
{
	SetValue<FJsonValueNumber>(Value);
}

template <typename JsonValueType, typename ValueType>
void FJsonOutputArchiveFormatter::SetValue(const ValueType& Value)
{
	SetValue(MakeShared<JsonValueType>(Value));
}

void FJsonOutputArchiveFormatter::SetValue(const TSharedRef<FJsonValue>& Value)
{
	FStackObject& Current = GetCurrent();
	check(!Current.Field.IsEmpty());

	UE_LOG(LogJsonOutputArchiveFormatter, Verbose, TEXT("%s%hs: %s = %s"), *Tabber(Stack.Num()), __FUNCTION__, *Current.Field, *Value->AsString());

	if (Current.bInStream)
	{
		Current.StreamValues.Add(Value);
	}
	else
	{
		Current.Object->RemoveField(Current.Field);
		Current.Object->SetField(Current.Field, Value);
	}
}
#endif
