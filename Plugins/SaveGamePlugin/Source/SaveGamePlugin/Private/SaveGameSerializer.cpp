// Copyright Alex Stevens (@MilkyEngineer). All Rights Reserved.

#pragma once

#include "SaveGameSerializer.h"

#include "SaveGameFunctionLibrary.h"
#include "SaveGameObject.h"
#include "SaveGameVersion.h"
#include "SaveGameProxyArchive.h"
#include "TaskHelpers.inl"
#include "Formatters/NullArchiveFormatter.h"

constexpr bool bForceSingleThreaded = false;
#define USE_TEXT_FORMATTER WITH_TEXT_ARCHIVE_SUPPORT

#if USE_TEXT_FORMATTER
#include "Formatters/JsonOutputArchiveFormatter.h"
#include "Formatters/ProxyArchiveFormatter.h"
#endif

#include "SaveGameSystem.h"
#include "PlatformFeatures.h"
#include "SaveGameSubsystem.h"
#include "SaveGameThreading.h"
#include "Tasks/TaskConcurrencyLimiter.h"

#define LEVEL_SUBPATH_PREFIX TEXT("PersistentLevel.")

using namespace UE::Tasks;

#if USE_TEXT_FORMATTER
class FSaveGameArchiveFormatter : public FProxyArchiveFormatter
{
public:
	FSaveGameArchiveFormatter(FArchive& InnerArchive, bool bUseNull)
		: FProxyArchiveFormatter(BinaryFormatter,
			static_cast<FStructuredArchiveFormatter&>(bUseNull ? FNullArchiveFormatter::Get() : JsonFormatter))
		, BinaryFormatter(InnerArchive)
	{}

	FBinaryArchiveFormatter BinaryFormatter;
	FJsonOutputArchiveFormatter JsonFormatter;
};
#endif

template<bool bIsLoading>
class TSaveGameArchive
{
	using FSaveGameFormatter =
		typename TChooseClass<USE_TEXT_FORMATTER, class FSaveGameArchiveFormatter, FBinaryArchiveFormatter>::Result;

public:
	TSaveGameArchive(FArchive& InArchive, TMap<FSoftObjectPath, FSoftObjectPath>& InRedirects)
		: ProxyArchive(InArchive, InRedirects)
		, Formatter(ProxyArchive, bIsLoading)
		, ArchiveData(nullptr)
	{}

	~TSaveGameArchive()
	{
		checkf(ArchiveData == nullptr, TEXT("Be sure to manually close archive!"));
	}

	FStructuredArchive::FRecord& GetRecord()
	{
		if (ArchiveData == nullptr)
		{
			ArchiveData = new FStructuredArchiveData(Formatter);
		}
		return ArchiveData->RootRecord;
	}

	void Close()
	{
		if (ArchiveData)
		{
			delete ArchiveData;
			ArchiveData = nullptr;
		}
	}

	void ConsolidateVersions(TSaveGameArchive& Other)
	{
		if (bIsLoading)
		{
			ProxyArchive.SetUEVer(Other.ProxyArchive.UEVer());
			ProxyArchive.SetEngineVer(Other.ProxyArchive.EngineVer());
			ProxyArchive.SetCustomVersions(Other.ProxyArchive.GetCustomVersions());
		}
		else
		{
			const FCustomVersionContainer& OtherVersions = Other.ProxyArchive.GetCustomVersions();

			for (const FCustomVersion& Version : ProxyArchive.GetCustomVersions().GetAllVersions())
			{
				const FCustomVersion* OtherVersion = OtherVersions.GetVersion(Version.Key);
				check(OtherVersion == nullptr || Version.Version == OtherVersion->Version);
				Other.ProxyArchive.SetCustomVersion(Version.Key, Version.Version, Version.GetFriendlyName());
			}
		}
	}

	TSaveGameProxyArchive<bIsLoading>& GetArchive() { return ProxyArchive; }

private:
	TSaveGameProxyArchive<bIsLoading> ProxyArchive;

public:
	FSaveGameFormatter Formatter;

private:
	struct FStructuredArchiveData
	{
		FStructuredArchiveData(FStructuredArchiveFormatter& InFormatter)
			: StructuredArchive(InFormatter)
			, RootSlot(StructuredArchive.Open())
			, RootRecord(RootSlot.EnterRecord())
		{
		}

		FStructuredArchive StructuredArchive;
		FStructuredArchive::FSlot RootSlot;
		FStructuredArchive::FRecord RootRecord;
	};

	FStructuredArchiveData* ArchiveData;
};

template<bool bLoading>
FORCEINLINE_DEBUGGABLE void SerializeCompressedData(FArchive& Ar, TArray<uint8>& Data)
{
	check(Ar.IsLoading() == bLoading);

	int64 UncompressedSize;

	if (!bLoading)
	{
		UncompressedSize = Data.Num();
	}

	Ar << UncompressedSize;

	if (bLoading)
	{
		Data.SetNumUninitialized(UncompressedSize);
	}

	Ar.SerializeCompressed(Data.GetData(), UncompressedSize, NAME_Zlib);
}

template <bool bIsLoading>
struct TSaveGameSerializer<bIsLoading>::FActorInfo
{
	~FActorInfo()
	{
		if (Archive)
		{
			delete Archive;
			Archive = nullptr;
		}

		if (MemoryArchive)
		{
			delete MemoryArchive;
			MemoryArchive = nullptr;
		}
	}

	void CreateArchive(TArray<uint8>& InData, TMap<FSoftObjectPath, FSoftObjectPath>& InRedirects)
	{
		MemoryArchive = new TSaveGameMemoryArchive(InData);
		Archive = new TSaveGameArchive<bIsLoading>(*MemoryArchive, InRedirects);
	}

	TWeakObjectPtr<AActor> Actor;
	FString Name;

	TArray<uint8> Data;
	TSaveGameArchive<bIsLoading>* Archive = nullptr;

private:
	FArchive* MemoryArchive = nullptr;
};

template <bool bIsLoading>
TSaveGameSerializer<bIsLoading>::TSaveGameSerializer(USaveGameSubsystem* InSubsystem)
	: Subsystem(InSubsystem)
	, Archive(Data)
	, SaveArchive(new TSaveGameArchive<bIsLoading>(Archive, Redirects))
	, ActorOffsetsOffset(0)
	, VersionOffset(0)
	, ActorsOffset(0)
{
	// Ensure that we're using the latest save game version
	Archive.UsingCustomVersion(FSaveGameVersion::GUID);
}

template <bool bIsLoading>
TSaveGameSerializer<bIsLoading>::~TSaveGameSerializer()
{
	delete SaveArchive;
}

template <bool bIsLoading>
FTask TSaveGameSerializer<bIsLoading>::DoOperation()
{
	if (ISaveGameSystem* SaveSystem = IPlatformFeaturesModule::Get().GetSaveGameSystem())
	{
		FTask PreviousTask;

		if (bIsLoading)
		{
			PreviousTask = Launch(UE_SOURCE_LOCATION, [this, SaveSystem]
			{
				TArray<uint8> CompressedData;
				const bool bLoaded = SaveSystem->LoadGame(false, *GetSaveName(), 0, CompressedData);
				check(bLoaded);

				// Decompress the loaded save game data
				TSaveGameMemoryArchive CompressorArchive(CompressedData);
				SerializeCompressedData<true>(CompressorArchive, Data);
			}, PreviousTask);
		}

		PreviousTask = Launch(UE_SOURCE_LOCATION, [this]
		{
			SerializeVersionOffset();
			SerializeHeader();
		}, PreviousTask);

		if (bIsLoading)
		{
			PreviousTask = Launch(UE_SOURCE_LOCATION, [this] { SerializeVersions(); }, PreviousTask);

			FTaskEvent MapLoadEvent(TEXT("MapLoaded"));
			LaunchGameThread(UE_SOURCE_LOCATION, [this, MapLoadEvent]() mutable
			{
				UWorld* World = Subsystem->GetWorld();

				check(!MapName.IsEmpty());
				check(!World->IsInSeamlessTravel());

				// When our map has loaded, continue the serialization process
				FCoreUObjectDelegates::PostLoadMapWithWorld.AddSPLambda(this, [this, MapLoadEvent](UWorld*) mutable
				{
					MapLoadEvent.Trigger();

					const signed int RemovedCount = FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);
                    check(RemovedCount == 1);
				});

				World->SeamlessTravel(MapName, true);
			}, PreviousTask);

			// Our next task should wait for the map to be loaded
			PreviousTask = MapLoadEvent;
		}

		PreviousTask = LaunchGameThread(UE_SOURCE_LOCATION, [this]
		{
			SerializeDestroyedActors();
			SerializeActors();
		}, PreviousTask);

		if (!bIsLoading)
		{
			PreviousTask = Launch(UE_SOURCE_LOCATION, [this]
			{
				MergeSaveData();
				SerializeVersions();

				// Go back to the start to override the original version offset
				Archive.Seek(0);
				SerializeVersionOffset();
			}, PreviousTask);
		}

		PreviousTask = Launch(UE_SOURCE_LOCATION, [this]
		{
			SaveArchive->Close();
		}, PreviousTask);

		if (!bIsLoading)
		{
			TArray<FTask, TFixedAllocator<1 + USE_TEXT_FORMATTER>> FinishEvents;

#if USE_TEXT_FORMATTER
  			FinishEvents.Add(Launch(UE_SOURCE_LOCATION, [this, SaveSystem]
			{
				TArray<uint8> JsonData;
  				FMemoryWriter WriterArchive(JsonData);
				TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> Writer = TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&WriterArchive);
				FJsonSerializer::Serialize(reinterpret_cast<FSaveGameArchiveFormatter&>(SaveArchive->Formatter).JsonFormatter.GetRoot(), Writer);

				SaveSystem->SaveGame(false, *(GetSaveName() + TEXT(".json")), 0, JsonData);
			}, PreviousTask));
#endif

			FinishEvents.Add(Launch(UE_SOURCE_LOCATION, [this, SaveSystem]
			{
				// Compress the save game data
				TArray<uint8> CompressedData;
				TSaveGameMemoryArchive CompressorArchive(CompressedData);
				SerializeCompressedData<false>(CompressorArchive, Data);

				const bool bSaved = SaveSystem->SaveGame(false, *GetSaveName(), 0, CompressedData);
				check(bSaved);
			}, PreviousTask));

			PreviousTask = Launch(UE_SOURCE_LOCATION, []{}, Prerequisites(FinishEvents), ETaskPriority::Default, EExtendedTaskPriority::Inline);
		}

		return PreviousTask;
	}

	return MakeCompletedTask<void>();
}

template <bool bIsLoading>
FString TSaveGameSerializer<bIsLoading>::GetSaveName()
{
	return TEXT("SaveGame");
}

template <bool bIsLoading>
void TSaveGameSerializer<bIsLoading>::SerializeVersionOffset()
{
	// We're a binary archive, so let's serialize where the version is
	// so that we can read it before loading anything
	SaveArchive->GetRecord() << SA_VALUE(TEXT("VersionsOffset"), VersionOffset);
}

template <bool bIsLoading>
void TSaveGameSerializer<bIsLoading>::SerializeHeader()
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_SaveGame_SerializeHeader);

	FEngineVersion EngineVersion;
	FPackageFileVersion PackageVersion;

	if (!bIsLoading)
	{
		EngineVersion = FEngineVersion::Current();
		PackageVersion = GPackageFileUEVersion;
	}

	FStructuredArchive::FRecord& Record = SaveArchive->GetRecord();

	Record << SA_VALUE(TEXT("EngineVersion"), EngineVersion);
	Record << SA_VALUE(TEXT("PackageVersion"), PackageVersion);

	if (bIsLoading)
	{
		Archive.SetEngineVer(EngineVersion);
		Archive.SetUEVer(PackageVersion);
	}

	// If we already have a map name, don't change it
	if (!bIsLoading && MapName.IsEmpty())
	{
		MapName = Subsystem->GetWorld()->GetOutermost()->GetLoadedPath().GetPackageName();
	}

	Record << SA_VALUE(TEXT("Map"), MapName);
}

void ExecuteJobs(const int32 NumJobs, TFunction<void(int32)> Job)
{
	FTaskConcurrencyLimiter ConcurrencyLimiter(FTaskGraphInterface::Get().GetNumWorkerThreads(), ETaskPriority::BackgroundNormal);
	FSaveGameTheadScope GameThreadScope;
	TAtomic<uint32> CompletedJobs = 0;

	for (int32 ActorIdx = 0; ActorIdx < NumJobs; ++ActorIdx)
	{
		auto DoJob = [ActorIdx, &CompletedJobs, &Job](uint32)
		{
			Job(ActorIdx);
			++CompletedJobs;
		};

		if (bForceSingleThreaded)
		{
			DoJob(ActorIdx);
		}
		else
		{
			ConcurrencyLimiter.Push(UE_SOURCE_LOCATION, DoJob);
		}
	}

	if (!bForceSingleThreaded)
	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_SaveGame_PumpGameThread);

		// Pump the Work Queue on the game thread
		while (GameThreadScope.ProcessThread() || CompletedJobs.Load() != NumJobs)
		{
			FPlatformProcess::YieldCycles(10000);
		}
	}
}

template<bool bIsLoading>
void TSaveGameSerializer<bIsLoading>::SerializeActors()
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_SaveGame_SerializeActors);

	// We start in the game thread, as we want to ensure we have control over what accesses UObjects
	check(IsInGameThread());

	// This serialize method assumes that we don't have any streamed/sub levels
	const UWorld* World = Subsystem->GetWorld();
	LevelAssetPath = FTopLevelAssetPath(World->GetCurrentLevel()->GetPackage()->GetFName(), World->GetCurrentLevel()->GetOuter()->GetFName());

	SaveGameActors = Subsystem->SaveGameActors.Array();
	int32 NumActors = SaveGameActors.Num();

	if (bIsLoading)
	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_SaveGame_CollectSpawnIDs);

		// Iterate through our live actors so that we can map their SpawnIDs
		for (const TWeakObjectPtr<AActor>& ActorPtr : SaveGameActors)
		{
			AActor* Actor = ActorPtr.Get();
			if (IsValid(Actor) && Actor->Implements<USaveGameSpawnActor>())
			{
				const FGuid SpawnID = ISaveGameSpawnActor::Execute_GetSpawnID(Actor);

				if (SpawnID.IsValid())
				{
					SpawnIDs.Add(SpawnID, Actor);
				}
			}
		}
	}

	ActorOffsets.SetNumZeroed(NumActors);
	ActorOffsetsOffset = Archive.Tell();
	Archive << ActorOffsets;

	// We do this as in a load game, we will have the number of actors from the actor offets
	NumActors = ActorOffsets.Num();
	SaveGameActors.SetNumZeroed(NumActors);
	ActorData.SetNumZeroed(NumActors);

	ActorsOffset = Archive.Tell();
	FStructuredArchive::FStream ActorStream = SaveArchive->GetRecord().EnterStream(TEXT("Actors"));

	// Need to init actors first for the sake of populating redirects before serialization
	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_SaveGame_InitializeActors);

		ExecuteJobs(NumActors, [this] (int32 ActorIdx) { InitializeActor(ActorIdx); });
	}

	// Actually do the serialization of each actor (now that we've updated redirects)
	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_SaveGame_Serialize);

		ExecuteJobs(NumActors, [this] (int32 ActorIdx) { SerializeActor(ActorIdx); });
	}

	if (bIsLoading)
	{
		for (FActorInfo& ActorInfo : ActorData)
		{
			ActorInfo.Archive->Close();
		}
	}
}

template <bool bIsLoading>
void TSaveGameSerializer<bIsLoading>::InitializeActor(int32 ActorIdx)
{
	const FString EventStr = FString::Printf(TEXT("InitializeActor: %i"), ActorIdx);
	SCOPED_NAMED_EVENT_FSTRING(EventStr, FColor::Red);

	FSoftClassPath Class;
	FGuid SpawnID;
	FActorInfo& ActorInfo = ActorData[ActorIdx];

	if (bIsLoading)
	{
		// When loading, we already have the data, so reuse our current data
		ActorInfo.CreateArchive(Data, Redirects);
		ActorInfo.Archive->GetArchive().Seek(ActorOffsets[ActorIdx]);
		ActorInfo.Archive->ConsolidateVersions(*SaveArchive);
	}
	else
	{
		AActor* Actor = SaveGameActors[ActorIdx].Get();
		ActorInfo.Actor = Actor;
		ActorInfo.Name = Actor->GetName();

		// When saving, we need to dump the data into
		ActorInfo.CreateArchive(ActorInfo.Data, Redirects);

		if (!USaveGameFunctionLibrary::WasObjectLoaded(ActorInfo.Actor.Get()))
		{
			// We're a spawned actor, stash the class
			Class = Actor->GetClass();
		}

		if (Actor->Implements<USaveGameSpawnActor>())
		{
			SpawnID = ISaveGameSpawnActor::Execute_GetSpawnID(Actor);
		}
	}

	FStructuredArchive::FRecord& Record = ActorInfo.Archive->GetRecord();

	Record.EnterField(TEXT("Name")) << ActorInfo.Name;

	ensureAlways(!ActorInfo.Name.IsEmpty());

	// If we have a class, we're a spawned actor
	if (TOptional<FStructuredArchive::FSlot> ClassSlot = Record.TryEnterField(TEXT("Class"), !Class.IsNull()))
	{
		ClassSlot.GetValue() << Class;
	}

	// If we have a GUID, we're a spawn actor that needs to be mapped by GUID
	if (TOptional<FStructuredArchive::FSlot> GuidSlot = Record.TryEnterField(TEXT("GUID"), SpawnID.IsValid()))
	{
		GuidSlot.GetValue() << SpawnID;
	}

	if (bIsLoading)
	{
		ISaveGameThreadQueue::FTaskFunction SpawnOrGetActor = [this, ActorIdx, Class, SpawnID]
		{
			UWorld* World = Subsystem->GetWorld();
			FActorInfo& ActorInfo = ActorData[ActorIdx];
			TWeakObjectPtr<AActor>& Actor = ActorInfo.Actor;

			if (Class.IsNull())
			{
				ensureAlways(!ActorInfo.Name.IsEmpty());

				// This is a loaded actor (is a level actor), let's find it
				Actor = FindObjectFast<AActor>(World->GetCurrentLevel(), *ActorInfo.Name);
			}
			else if (SpawnID.IsValid() && SpawnIDs.Contains(SpawnID))
			{
				Actor = SpawnIDs[SpawnID];
			}
			else
			{
				UClass* ActorClass = Class.TryLoadClass<AActor>();

				ensureAlways(!ActorInfo.Name.IsEmpty());
				ensureAlways(ActorClass);

				// This is a spawned actor, let's spawn it
				FActorSpawnParameters SpawnParameters;

				// If we were handling levels, specify it here
				SpawnParameters.OverrideLevel = World->GetCurrentLevel();
				SpawnParameters.Name = *ActorInfo.Name;
				SpawnParameters.bNoFail = true;

				Actor = World->SpawnActor(ActorClass, nullptr, nullptr, SpawnParameters);

				if (SpawnID.IsValid() && Actor->Implements<USaveGameSpawnActor>())
				{
					ISaveGameSpawnActor::Execute_SetSpawnID(Actor.Get(), SpawnID);
				}
			}

			check(Actor.IsValid());
			SaveGameActors[ActorIdx] = Actor;

			if (SpawnID.IsValid())
			{
				const FString ActorSubPath = LEVEL_SUBPATH_PREFIX + ActorInfo.Name;

				// We potentially have a spawned actor that other actors reference
				// If the name has changed, be sure to redirect the old actor path to the new one
				ActorInfo.Archive->GetArchive().AddRedirect(FSoftObjectPath(LevelAssetPath, ActorSubPath), FSoftObjectPath(Actor.Get()));
			}
		};

		if (bForceSingleThreaded)
		{
			SpawnOrGetActor();
		}
		else
		{
			ISaveGameThreadQueue::Get().AddTask(Forward<ISaveGameThreadQueue::FTaskFunction>(SpawnOrGetActor));
		}
	}
}

template <bool bIsLoading>
void TSaveGameSerializer<bIsLoading>::SerializeActor(int32 ActorIdx)
{
	check(bForceSingleThreaded || !IsInGameThread());

	QUICK_SCOPE_CYCLE_COUNTER(STAT_SaveGame_SerializeScriptProperties);

	FActorInfo& ActorInfo = ActorData[ActorIdx];
	const AActor* Actor = ActorInfo.Actor.Get();
	FStructuredArchive::FRecord& Record = ActorInfo.Archive->GetRecord();

	// Since we have control of the game thread, we should be pretty safe to serialize our properties
	Actor->SerializeScriptProperties(Record.EnterField(TEXT("Properties")));

	ISaveGameThreadQueue::FTaskFunction CallOnSerialize = [this, ActorIdx]
	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_SaveGame_OnSerialize);

		FActorInfo& ActorInfo = ActorData[ActorIdx];
		AActor* Actor = ActorInfo.Actor.Get();
		FStructuredArchive::FRecord& Record = ActorInfo.Archive->GetRecord();
		FStructuredArchive::FSlot CustomDataSlot = Record.EnterField(TEXT("Data"));
		FStructuredArchive::FRecord CustomDataRecord = CustomDataSlot.EnterRecord();

		// Encapsulate the record in something a Blueprint can access
		FSaveGameArchive SaveGameArchive(CustomDataRecord, Actor);

		ISaveGameObject::Execute_OnSerialize(Actor, SaveGameArchive, bIsLoading);
	};

	if (bForceSingleThreaded || ISaveGameObject::Execute_IsThreadSafe(Actor))
	{
		CallOnSerialize();
	}
	else
	{
		// We're not threadsafe, queue up this actor to the game thread
		ISaveGameThreadQueue::Get().AddTask(Forward<ISaveGameThreadQueue::FTaskFunction>(CallOnSerialize));
	}
}

template <bool bIsLoading>
void TSaveGameSerializer<bIsLoading>::MergeSaveData()
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_SaveGame_MergeThreadData);

	Archive.Seek(ActorsOffset);
	FStructuredArchive::FStream ActorStream = SaveArchive->GetRecord().EnterStream(TEXT("Actors"));

	// Merge each actor's save data
	for (int32 ActorIdx = 0; ActorIdx < ActorData.Num(); ++ActorIdx)
	{
		FActorInfo& ActorInfo = ActorData[ActorIdx];

		ActorInfo.Archive->Close();
		SaveArchive->ConsolidateVersions(*ActorInfo.Archive);

		FStructuredArchive::FSlot StreamElement = ActorStream.EnterElement();

#if USE_TEXT_FORMATTER
		// Merge our JSON structure into the main Save Game archive's
		FSaveGameArchiveFormatter& Formatter = reinterpret_cast<FSaveGameArchiveFormatter&>(SaveArchive->Formatter);
		Formatter.JsonFormatter.Serialize(reinterpret_cast<FSaveGameArchiveFormatter&>(ActorInfo.Archive->Formatter).JsonFormatter.GetRoot());
#endif

		Archive.Seek(Data.Num());

		uint64 DataSize = ActorInfo.Data.Num();
		StreamElement.EnterAttribute(TEXT("DataSize")) << DataSize;

		ActorOffsets[ActorIdx] = Data.Num();

		// We are appending the data, as serialising will prepend data on the length of the array
		Data.Append(ActorInfo.Data);
	}

	ActorData.Empty();

	Archive.Seek(ActorOffsetsOffset);
	Archive << ActorOffsets;
	Archive.Seek(Data.Num());
}

template <bool bIsLoading>
void TSaveGameSerializer<bIsLoading>::SerializeDestroyedActors()
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_SaveGame_SerializeDestroyedActors);

	check(IsInGameThread());
	const UWorld* World = Subsystem->GetWorld();

	int32 NumDestroyedActors;

	if (!bIsLoading)
	{
		NumDestroyedActors = Subsystem->DestroyedLevelActors.Num();
	}

	FStructuredArchive::FArray DestroyedActorsArray = SaveArchive->GetRecord().EnterArray(TEXT("DestroyedActors"), NumDestroyedActors);

	if (bIsLoading)
	{
		// Allocate our expected number of actors
		Subsystem->DestroyedLevelActors.Reset();
		Subsystem->DestroyedLevelActors.Reserve(NumDestroyedActors);
	}

	auto DestroyedActorsIt = Subsystem->DestroyedLevelActors.CreateConstIterator();
	for (int32 ActorIdx = 0; ActorIdx < NumDestroyedActors; ++ActorIdx)
	{
		FName ActorName;

		if (!bIsLoading)
		{
			// Only store the object name without the prefix and full path
			FString ActorSubPath = (*DestroyedActorsIt).GetSubPathString();
			ActorSubPath.RemoveFromStart(LEVEL_SUBPATH_PREFIX);
			ActorName = *ActorSubPath;

			++DestroyedActorsIt;
		}

		DestroyedActorsArray.EnterElement() << ActorName;

		if (bIsLoading)
		{
			// Find the live actor in the level
			if (AActor* DestroyedActor = FindObjectFast<AActor>(World->GetCurrentLevel(), ActorName))
			{
				// Be sure to add any valid destroyed actors back into the array for saving later!
				Subsystem->DestroyedLevelActors.Add(DestroyedActor);
				DestroyedActor->Destroy();
			}
		}
	}
}

template <bool bIsLoading>
void TSaveGameSerializer<bIsLoading>::SerializeVersions()
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_SaveGame_SerializeVersions);

	const uint64 InitialPosition = Archive.Tell();
	FCustomVersionContainer VersionContainer;

	if (bIsLoading)
	{
		Archive.Seek(VersionOffset);
	}
	else
	{
		// Grab a copy of our archive's current versions
		VersionContainer = Archive.GetCustomVersions();
		VersionOffset = Archive.Tell();
	}

	VersionContainer.Serialize(SaveArchive->GetRecord().EnterField(TEXT("Versions")));

	if (bIsLoading)
	{
		// Assign our serialized versions
		Archive.SetCustomVersions(VersionContainer);

		// After serializing versions, go back to initial position
		Archive.Seek(InitialPosition);
	}
}

// Instantiate the permutations of TSaveGameSerializer
template TSaveGameSerializer<false>;
template TSaveGameSerializer<true>;
