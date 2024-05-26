// Copyright Alex Stevens (@MilkyEngineer). All Rights Reserved.

#pragma once

#include "Tasks/Pipe.h"

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SaveGameSubsystem.generated.h"

/**
 * The subsystem that manages the lifetime of a save game.
 */
UCLASS()
class SAVEGAMEPLUGIN_API USaveGameSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category="SaveGamePlugin|Save")
	void Save();

	UFUNCTION(BlueprintCallable, Category="SaveGamePlugin|Load")
	void Load();

	UFUNCTION(BlueprintCallable, Category="SaveGamePlugin|Load")
	bool IsLoadingSaveGame() const;

protected:
	void OnWorldInitialized(UWorld* World, const UWorld::InitializationValues);
	void OnActorsInitialized(const FActorsInitializedParams& Params);
	void OnWorldCleanup(UWorld* World, bool, bool);

	void OnActorPreSpawn(AActor* Actor);
	void OnActorDestroyed(AActor* Actor);

private:
	template<bool> friend class TSaveGameSerializer;
	UE::Tasks::FPipe SaveGamePipe = UE::Tasks::FPipe(TEXT("SaveGameSubsystem"));

	TSet<FSoftObjectPath> DestroyedLevelActors;
	TSet<TWeakObjectPtr<AActor>> SaveGameActors;
};
