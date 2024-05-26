// Copyright Alex Stevens (@MilkyEngineer). All Rights Reserved.

#include "SaveGameSubsystem.h"

#include "SaveGameFunctionLibrary.h"
#include "SaveGameObject.h"
#include "SaveGameSerializer.h"

#include "EngineUtils.h"

using namespace UE::Tasks;

void USaveGameSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	FWorldDelegates::OnPostWorldInitialization.AddUObject(this, &ThisClass::OnWorldInitialized);
	FWorldDelegates::OnWorldInitializedActors.AddUObject(this, &ThisClass::OnActorsInitialized);
	FWorldDelegates::OnWorldCleanup.AddUObject(this, &ThisClass::OnWorldCleanup);

	// This example doesn't handle streaming levels, but if we did, we'd use a combination of
	// FWorldDelegates::LevelAddedToWorld and FWorldDelegates::PreLevelRemovedFromWorld
	// In these, we'd store the current state of actors within that level

	OnWorldInitialized(GetWorld(), UWorld::InitializationValues());
}

void USaveGameSubsystem::Deinitialize()
{
	FWorldDelegates::OnPostWorldInitialization.RemoveAll(this);
	FWorldDelegates::OnWorldInitializedActors.RemoveAll(this);
	FWorldDelegates::OnWorldCleanup.RemoveAll(this);

	FWorldDelegates::LevelAddedToWorld.RemoveAll(this);
	FWorldDelegates::PreLevelRemovedFromWorld.RemoveAll(this);
}

void USaveGameSubsystem::Save()
{
	TSharedPtr<TSaveGameSerializer<false>> Serializer = MakeShared<TSaveGameSerializer<false>>(this);

	SaveGamePipe.Launch(UE_SOURCE_LOCATION, [this, Serializer]
	{
		constexpr TCHAR RegionName[] = TEXT("SaveGame[Save]");
		TRACE_BEGIN_REGION(RegionName);

		FTask Previous = Serializer->DoOperation();

		// This will also keep the Serializer alive until we're complete
		AddNested(Launch(UE_SOURCE_LOCATION, [Serializer]
		{
			TRACE_END_REGION(RegionName);
		}, Previous));
	});
}

void USaveGameSubsystem::Load()
{
	TSharedPtr<TSaveGameSerializer<true>> Serializer = MakeShared<TSaveGameSerializer<true>>(this);

	SaveGamePipe.Launch(UE_SOURCE_LOCATION, [this, Serializer]
	{
		constexpr TCHAR RegionName[] = TEXT("SaveGame[Load]");
		TRACE_BEGIN_REGION(RegionName);

		FTask Previous = Serializer->DoOperation();

		// This will also keep the Serializer alive until we're complete
		AddNested(Launch(UE_SOURCE_LOCATION, [Serializer]
		{
			TRACE_END_REGION(RegionName);
		}, Previous));
	});
}

bool USaveGameSubsystem::IsLoadingSaveGame() const
{
	return SaveGamePipe.HasWork();
}

void USaveGameSubsystem::OnWorldInitialized(UWorld* World, const UWorld::InitializationValues)
{
	if (!IsValid(World) || GetWorld() != World)
	{
		return;
	}

	World->AddOnActorPreSpawnInitialization(FOnActorSpawned::FDelegate::CreateUObject(this, &ThisClass::OnActorPreSpawn));
	World->AddOnActorDestroyedHandler(FOnActorDestroyed::FDelegate::CreateUObject(this, &ThisClass::OnActorDestroyed));
}

void USaveGameSubsystem::OnActorsInitialized(const FActorsInitializedParams& Params)
{
	if (!IsValid(Params.World) || GetWorld() != Params.World)
	{
		return;
	}

	for (TActorIterator<AActor> It(Params.World); It; ++It)
	{
		AActor* Actor = *It;
		if (IsValid(Actor) && Actor->Implements<USaveGameObject>())
		{
			SaveGameActors.Add(Actor);
		}
	}
}

void USaveGameSubsystem::OnWorldCleanup(UWorld* World, bool, bool)
{
	if (!IsValid(World) || GetWorld() != World)
	{
		return;
	}

	SaveGameActors.Reset();
	DestroyedLevelActors.Reset();
}

void USaveGameSubsystem::OnActorPreSpawn(AActor* Actor)
{
	if (IsValid(Actor) && Actor->Implements<USaveGameObject>())
	{
		SaveGameActors.Add(Actor);
	}
}

void USaveGameSubsystem::OnActorDestroyed(AActor* Actor)
{
	SaveGameActors.Remove(Actor);

	if (USaveGameFunctionLibrary::WasObjectLoaded(Actor))
	{
		DestroyedLevelActors.Add(Actor);
	}
}
