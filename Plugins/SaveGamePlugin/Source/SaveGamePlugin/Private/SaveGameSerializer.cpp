// Copyright Alex Stevens (@MilkyEngineer). All Rights Reserved.

#pragma once

#include "SaveGameSerializer.h"

#include "SaveGameFunctionLibrary.h"
#include "SaveGameObject.h"
#include "SaveGameVersion.h"
#include "SaveGameProxyArchive.h"
#include "TaskHelpers.inl"

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
	FSaveGameArchiveFormatter(FArchive& InnerArchive)
		: FProxyArchiveFormatter(BinaryFormatter, JsonFormatter)
		, BinaryFormatter(InnerArchive)
	{}

	FBinaryArchiveFormatter BinaryFormatter;
	FJsonOutputArchiveFormatter JsonFormatter;
};
#endif

template<bool bIsLoading>
class TSaveGameArchive
{
	typedef typename TChooseClass<!bIsLoading && USE_TEXT_FORMATTER,
		class FSaveGameArchiveFormatter, FBinaryArchiveFormatter>::Result FSaveGameFormatter;

public:
	TSaveGameArchive(FArchive& InArchive, TMap<FSoftObjectPath, FSoftObjectPath>& InRedirects)
		: ProxyArchive(InArchive, InRedirects)
		, Formatter(ProxyArchive)
		, StructuredArchive(Formatter)
		, RootSlot(StructuredArchive.Open())
		, RootRecord(RootSlot.EnterRecord())
	{}

	FStructuredArchive::FRecord& GetRecord() { return RootRecord; }

	void Close()
	{
		StructuredArchive.Close();
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
	FStructuredArchive StructuredArchive;
	FStructuredArchive::FSlot RootSlot;
	FStructuredArchive::FRecord RootRecord;
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
TSaveGameSerializer<bIsLoading>::TSaveGameSerializer(USaveGameSubsystem* InSubsystem)
	: Subsystem(InSubsystem)
	, Archive(Data)
	, SaveArchive(new TSaveGameArchive<bIsLoading>(Archive, Redirects))
	, VersionOffset(0)
{
	// Ensure that we're using the latest save game version
	SaveArchive->GetArchive().UsingCustomVersion(FSaveGameVersion::GUID);
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
				SaveSystem->LoadGame(false, *GetSaveName(), 0, CompressedData);

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
			PreviousTask = Launch(UE_SOURCE_LOCATION, [this]
			{
				const uint64 InitialPosition = Archive.Tell();

				// After serializing versions, go back to initial position
				ON_SCOPE_EXIT
				{
					Archive.Seek(InitialPosition);
				};

				Archive.Seek(VersionOffset);
				SerializeVersions();
			}, PreviousTask);

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
			SerializeActors();
			SerializeDestroyedActors();
		}, PreviousTask);

		PreviousTask = Launch(UE_SOURCE_LOCATION, [this]
		{
			if (!bIsLoading)
			{
				SerializeVersions();

				// We've updated the VersionOffset, let's go back to the start and rewrite the header
				SaveArchive->GetArchive().Seek(0);
				SerializeVersionOffset();
			}

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
		SaveArchive->GetArchive().SetEngineVer(EngineVersion);
		SaveArchive->GetArchive().SetUEVer(PackageVersion);
	}

	// If we already have a map name, don't change it
	if (!bIsLoading && MapName.IsEmpty())
	{
		MapName = Subsystem->GetWorld()->GetOutermost()->GetLoadedPath().GetPackageName();
	}

	Record << SA_VALUE(TEXT("Map"), MapName);
}

template<bool bIsLoading>
void TSaveGameSerializer<bIsLoading>::SerializeActors()
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_SaveGame_SerializeActors);

	// We start in the game thread, as we want to ensure we have control over what accesses UObjects
	check(IsInGameThread());

	// This serialize method assumes that we don't have any streamed/sub levels
	UWorld* World = Subsystem->GetWorld();
	const FTopLevelAssetPath LevelAssetPath(World->GetCurrentLevel()->GetPackage()->GetFName(), World->GetCurrentLevel()->GetOuter()->GetFName());
	FTask PreviousTask;

	struct FActorInfo
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
		uint64 DataSize = UINT64_MAX;

		TArray<uint8> Data;
		TSaveGameArchive<bIsLoading>* Archive = nullptr;

	private:
		FArchive* MemoryArchive = nullptr;
	};

	TMap<FGuid, AActor*> SpawnIDs;
	TArray<FActorInfo> ActorData;
	TArray<TWeakObjectPtr<AActor>> SaveGameActors = Subsystem->SaveGameActors.Array();
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

	constexpr TCHAR DataSizeName[] = TEXT("DataSize");
	FStructuredArchive::FMap ActorMap = SaveArchive->GetRecord().EnterMap(TEXT("Actors"), NumActors);

	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_SaveGame_InitializeActors);

		ActorData.SetNumZeroed(NumActors);

		// Iterate through the saved actors and spawn or find their live equivalent
		for (int32 ActorIdx = 0; ActorIdx < NumActors; ++ActorIdx)
		{
			FSoftClassPath Class;
			FGuid SpawnID;
			FActorInfo& ActorInfo = ActorData[ActorIdx];
			TWeakObjectPtr<AActor>& Actor = ActorInfo.Actor;

			if (bIsLoading)
			{
				// For saving, need to do this later
				FStructuredArchive::FSlot ActorSlot = ActorMap.EnterElement(ActorInfo.Name);
				ActorSlot.EnterRecord().EnterField(DataSizeName) << ActorInfo.DataSize;

				// When loading, we already have the data, so reuse our current data
				ActorInfo.CreateArchive(Data, Redirects);
				ActorInfo.Archive->GetArchive().Seek(SaveArchive->GetArchive().Tell());
				ActorInfo.Archive->ConsolidateVersions(*SaveArchive);
			}
			else
			{
				ActorInfo.Actor = SaveGameActors[ActorIdx].Get();
				ActorInfo.Name = ActorInfo.Actor->GetName();

				// When saving, we need to dump the data into
				ActorInfo.CreateArchive(ActorInfo.Data, Redirects);

				if (!USaveGameFunctionLibrary::WasObjectLoaded(Actor.Get()))
				{
					// We're a spawned actor, stash the class
					Class = Actor->GetClass();
				}

				if (Actor->Implements<USaveGameSpawnActor>())
				{
					SpawnID = ISaveGameSpawnActor::Execute_GetSpawnID(Actor.Get());
				}
			}

			const uint64 ActorStartPosition = SaveArchive->GetArchive().Tell();

			FStructuredArchive::FRecord& Record = ActorInfo.Archive->GetRecord();

			ensureAlways(!ActorInfo.Name.IsEmpty());

			// If we have a class, we're a spawned actor
			if (TOptional<FStructuredArchive::FSlot> ClassSlot = Record.TryEnterField(TEXT("Class"), !Class.IsNull()))
			{
				ClassSlot.GetValue() << Class;
			}

			// If we have a GUID, we're a spawn actor that needs to be mapped by GUID
			TOptional<FStructuredArchive::FSlot> GuidSlot = Record.TryEnterField(TEXT("GUID"), SpawnID.IsValid());
			if (GuidSlot.IsSet())
			{
				GuidSlot.GetValue() << SpawnID;
			}

			if (bIsLoading)
			{
				if (Class.IsNull())
				{
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

				if (SpawnID.IsValid())
				{
					const FString ActorSubPath = LEVEL_SUBPATH_PREFIX + ActorInfo.Name;

					// We potentially have a spawned actor that other actors reference
					// If the name has changed, be sure to redirect the old actor path to the new one
					ActorInfo.Archive->GetArchive().AddRedirect(FSoftObjectPath(LevelAssetPath, ActorSubPath), FSoftObjectPath(Actor.Get()));
				}

				// Ensure we have an initialized data size
				check(ActorInfo.DataSize != UINT64_MAX);

				// As we'll be deferring serialization, we need to skip over to the next actor
				SaveArchive->GetArchive().Seek(ActorStartPosition + ActorInfo.DataSize);
			}
		}
	}

	// Actually do the serialization of each actor (now that we've updated redirects)
	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_SaveGame_Serialize);

		FSaveGameTheadScope GameThreadScope;
		FTaskConcurrencyLimiter ConcurrencyLimiter(FTaskGraphInterface::Get().GetNumWorkerThreads(), ETaskPriority::BackgroundHigh);

		for (int32 ActorIdx = 0; ActorIdx < NumActors; ++ActorIdx)
		{
			constexpr bool bForceSingleThreaded = false;

			auto SerializeLambda = [&ActorData,ActorIdx](uint32)
			{
				check(bForceSingleThreaded || !IsInGameThread());

				QUICK_SCOPE_CYCLE_COUNTER(STAT_SaveGame_SerializeScriptProperties);

				FActorInfo& ActorInfo = ActorData[ActorIdx];
				AActor* Actor = ActorInfo.Actor.Get();
				FStructuredArchive::FRecord& Record = ActorInfo.Archive->GetRecord();

				// Since we have control of the game thread, we should be pretty safe to serialize our properties
				Actor->SerializeScriptProperties(Record.EnterField(TEXT("Properties")));

				ISaveGameThreadQueue::FTaskFunction CallOnSerialize = [&ActorInfo]
				{
					QUICK_SCOPE_CYCLE_COUNTER(STAT_SaveGame_OnSerialize);

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
			};

			if (bForceSingleThreaded)
			{
				SerializeLambda(ActorIdx);
			}
			else
			{
				ConcurrencyLimiter.Push(UE_SOURCE_LOCATION, SerializeLambda);
			}
		}

		QUICK_SCOPE_CYCLE_COUNTER(STAT_SaveGame_PumpGameThread);
		const FTimespan ConcurrencyTimeout(1000);

		do
		{
			// Pump the Work Queue on the game thread
			GameThreadScope.PumpThread();
		}
		while (!ConcurrencyLimiter.Wait(ConcurrencyTimeout));

		GameThreadScope.PumpThread();
	}

	if (!bIsLoading)
	{
		// Merge each actor's save data
		for (int32 ActorIdx = 0; ActorIdx < NumActors; ++ActorIdx)
		{
			FActorInfo& ActorInfo = ActorData[ActorIdx];

			SaveArchive->ConsolidateVersions(*ActorInfo.Archive);

			FStructuredArchive::FSlot ActorSlot = ActorMap.EnterElement(ActorInfo.Name);
			FStructuredArchive::FRecord ActorRecord = ActorSlot.EnterRecord();

#if USE_TEXT_FORMATTER
			// Merge our JSON structure into the main Save Game archive's
			FSaveGameArchiveFormatter& Formatter = reinterpret_cast<FSaveGameArchiveFormatter&>(SaveArchive->Formatter);
			Formatter.JsonFormatter.Serialize(reinterpret_cast<FSaveGameArchiveFormatter&>(ActorInfo.Archive->Formatter).JsonFormatter.GetRoot());
#endif

			const uint64 DataSizePosition = SaveArchive->GetArchive().Tell();
			ActorRecord.EnterField(DataSizeName) << ActorInfo.DataSize;
			const uint64 PostDataSizePosition = SaveArchive->GetArchive().Tell();

			// We are appending the data, as serialising will prepend data on the length of the array
			Data.Append(ActorInfo.Data);

			// Update our data size and then skip
			const uint64 PreviousPosition = Data.Num();
			SaveArchive->GetArchive().Seek(DataSizePosition);
			ActorInfo.DataSize = ActorInfo.Data.Num();
			ActorRecord.EnterField(DataSizeName) << ActorInfo.DataSize;
			SaveArchive->GetArchive().Seek(PreviousPosition);
		}
	}
}

template <bool bIsLoading>
void TSaveGameSerializer<bIsLoading>::SerializeDestroyedActors()
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_SaveGame_SerializeDestroyedActors);

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

	VersionOffset = SaveArchive->GetArchive().Tell();

	FCustomVersionContainer VersionContainer;

	if (!bIsLoading)
	{
		// Grab a copy of our archive's current versions
		VersionContainer = SaveArchive->GetArchive().GetCustomVersions();
	}

	VersionContainer.Serialize(SaveArchive->GetRecord().EnterField(TEXT("Versions")));

	if (bIsLoading)
	{
		// Assign our serialized versions
		SaveArchive->GetArchive().SetCustomVersions(VersionContainer);
	}
}

// Instantiate the permutations of TSaveGameSerializer
template TSaveGameSerializer<false>;
template TSaveGameSerializer<true>;
