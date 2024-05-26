// Copyright Alex Stevens (@MilkyEngineer). All Rights Reserved.

#pragma once

#include "Tasks/Task.h"

namespace UE::Tasks
{
	namespace Private
	{
		template<typename TaskBodyType>
		void ExecuteGameThreadTask(TaskBodyType&& TaskBody, const TCHAR* DebugName)
		{
			TSharedPtr<FTaskEvent> GameThreadEvent = MakeShared<FTaskEvent>(DebugName);

			// When EExtendedTaskPriority::GameThread* priorities are compiled in the engine, use them instead of task events
			// PreviousTask = Launch(UE_SOURCE_LOCATION, [this] { SerializeActors(); }, PreviousTask, ETaskPriority::Normal, EExtendedTaskPriority::GameThreadNormalPri);

			AsyncTask(ENamedThreads::GameThread, [TaskBody = MoveTempIfPossible(TaskBody), GameThreadEvent] () mutable
			{
				TaskBody();
				GameThreadEvent->Trigger();
			});

			AddNested(*GameThreadEvent);
		}
	}

	template<typename TaskBodyType>
	FTask LaunchGameThread(const TCHAR* DebugName, TaskBodyType&& TaskBody,
		ETaskPriority TaskPriority = ETaskPriority::Normal,
		EExtendedTaskPriority ExtendedPriority = EExtendedTaskPriority::None,
		ETaskFlags Flags = ETaskFlags::None
		)
	{
		return Launch(DebugName, [TaskBody = MoveTemp(TaskBody), DebugName]
		{
			Tasks::Private::ExecuteGameThreadTask(TaskBody, DebugName);
		}, TaskPriority, ExtendedPriority, Flags);
	}

	template<typename TaskBodyType, typename PrerequisitesCollectionType>
	FTask LaunchGameThread(const TCHAR* DebugName, TaskBodyType&& TaskBody,
		PrerequisitesCollectionType&& Prerequisites,
		ETaskPriority TaskPriority = ETaskPriority::Normal,
		EExtendedTaskPriority ExtendedPriority = EExtendedTaskPriority::None,
		ETaskFlags Flags = ETaskFlags::None
		)
	{
		return Launch(DebugName, [TaskBody = MoveTemp(TaskBody), DebugName]
		{
			Tasks::Private::ExecuteGameThreadTask(TaskBody, DebugName);
		}, Forward<PrerequisitesCollectionType>(Prerequisites), TaskPriority, ExtendedPriority, Flags);
	}
}
