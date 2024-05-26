// Copyright Alex Stevens (@MilkyEngineer). All Rights Reserved.

#pragma once

#include "Templates/ChooseClass.h"
#include "Tasks/Task.h"

class USaveGameSubsystem;

template <bool bIsLoading> class TSaveGameArchive;

class FSaveGameSerializer :  public TSharedFromThis<FSaveGameSerializer>
{
public:
	virtual ~FSaveGameSerializer() = default;

	virtual bool IsLoading() const = 0;
	virtual UE::Tasks::FTask DoOperation() = 0;
};

/**
 * The class that manages serializing the world.
 *
 * Archive data structured like so:
 * - Header
 *		- Map Name
 *		- Engine Versions
 * - Actors
 *		- Actor Name #1:
 *			- Class: If spawned
 *			- SpawnID: If implements ISaveGameSpawnActor
 *			- SaveGame Properties
 *			- Data written by ISaveGameObject::OnSerialize
 *		- ...
 * - Destroyed Level Actors
 *		- Actor Name #1
 *		- ...
 * - Versions
 *		- Version:
 *			- ID
 *			- Version Number
 *		- ...
 */
template<bool bIsLoading>
class TSaveGameSerializer final : public FSaveGameSerializer
{
	using TSaveGameMemoryArchive = typename TChooseClass<bIsLoading, FMemoryReader, FMemoryWriter>::Result;

public:
	TSaveGameSerializer(USaveGameSubsystem* InSaveGameSubsystem);
	virtual ~TSaveGameSerializer() override;

	virtual bool IsLoading() const override { return bIsLoading; }
	virtual UE::Tasks::FTask DoOperation() override;

private:
	static FString GetSaveName();

	void SerializeVersionOffset();

	/** Serializes information about the archive, like Map Name, and position of versioning information */
	void SerializeHeader();

	/**
	 * Serializes all of the actors that the SaveGameSubsystem is keeping track of.
	 * On load, it will also pre-spawn any actors and map any actors with Spawn IDs
	 * before running the actual serialization step.
	 */
	void SerializeActors();

	/** Serializes any destroyed level actors. On load, level actors will exist again, so this will re-destroy them */
	void SerializeDestroyedActors();

	/**
	 * Serialized at the end of the archive, the versions are useful for marshaling old data.
	 * These also contain the versions added by USaveGameFunctionLibrary::UseCustomVersion.
	 */
	void SerializeVersions();

private:
	USaveGameSubsystem* Subsystem;
	TArray<uint8> Data;
	TSaveGameMemoryArchive Archive;
	TMap<FSoftObjectPath, FSoftObjectPath> Redirects;
	TSaveGameArchive<bIsLoading>* SaveArchive;

	FString MapName;
	uint64 VersionOffset;
};

