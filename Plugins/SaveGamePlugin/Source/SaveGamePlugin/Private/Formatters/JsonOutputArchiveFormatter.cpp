// Copyright Alex Stevens (@MilkyEngineer). All Rights Reserved.

#include "Formatters/JsonOutputArchiveFormatter.h"

#if WITH_TEXT_ARCHIVE_SUPPORT
#include "DOM/JsonObject.h"

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
}

void FJsonOutputArchiveFormatter::EnterField(FArchiveFieldName Name)
{
	FStackObject& Current = GetCurrent();
	check(!Current.bInStream);
	Current.Field = FString(Name.Name);
}

void FJsonOutputArchiveFormatter::LeaveField()
{
	FStackObject& Current = GetCurrent();
	check(!Current.Field.IsEmpty());

	Current.bInAttributedValueValue = false;
	Current.Field.Reset();
}

bool FJsonOutputArchiveFormatter::TryEnterField(FArchiveFieldName Name, bool bEnterWhenWriting)
{
	EnterField(Name);
	return true;
}

void FJsonOutputArchiveFormatter::EnterArray(int32& NumElements)
{
	GetCurrent().StreamValues.Reserve(NumElements);
	EnterStream();
}

void FJsonOutputArchiveFormatter::LeaveArray()
{
	LeaveStream();
}

void FJsonOutputArchiveFormatter::EnterArrayElement()
{
	EnterStreamElement();
}

void FJsonOutputArchiveFormatter::LeaveArrayElement()
{
	LeaveStreamElement();
}

void FJsonOutputArchiveFormatter::EnterStream()
{
	FStackObject& Current = GetCurrent();
	Current.bInStream = true;
}

void FJsonOutputArchiveFormatter::LeaveStream()
{
	FStackObject& Current = GetCurrent();
	Current.bInStream = false;
	Current.Object->SetArrayField(Current.Field, Current.StreamValues);
	Current.StreamValues.Reset();
}

void FJsonOutputArchiveFormatter::EnterStreamElement()
{
}

void FJsonOutputArchiveFormatter::LeaveStreamElement()
{
}

void FJsonOutputArchiveFormatter::EnterMap(int32& NumElements)
{
	EnterRecord();
}

void FJsonOutputArchiveFormatter::LeaveMap()
{
	LeaveRecord();
}

void FJsonOutputArchiveFormatter::EnterMapElement(FString& Name)
{
	EnterField(*Name);
}

void FJsonOutputArchiveFormatter::LeaveMapElement()
{
	LeaveField();
}

void FJsonOutputArchiveFormatter::EnterAttributedValue()
{
	EnterRecord();
}

void FJsonOutputArchiveFormatter::LeaveAttributedValue()
{
	LeaveRecord();
}

void FJsonOutputArchiveFormatter::EnterAttribute(FArchiveFieldName AttributeName)
{
	EnterField(AttributeName);
}

void FJsonOutputArchiveFormatter::LeaveAttribute()
{
	LeaveField();
}

void FJsonOutputArchiveFormatter::EnterAttributedValueValue()
{
	GetCurrent().bInAttributedValueValue = true;
	EnterField(FArchiveFieldName(TEXT("_Value")));
}

bool FJsonOutputArchiveFormatter::TryEnterAttribute(FArchiveFieldName AttributeName, bool bEnterWhenWriting)
{
	EnterField(AttributeName);
	return true;
}

bool FJsonOutputArchiveFormatter::TryEnterAttributedValueValue()
{
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
	SetValue(MakeShared<FJsonValueObject>(Value));
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
