// Copyright Alex Stevens (@MilkyEngineer). All Rights Reserved.

#pragma once

#include "Tasks/Task.h"

namespace UE::Tasks
{
	namespace Private
	{
		inline EExtendedTaskPriority ConvertPriorityToGameThread(ETaskPriority TaskPriority)
		{
			switch (TaskPriority)
			{
			case ETaskPriority::BackgroundHigh:
			case ETaskPriority::High:
				return EExtendedTaskPriority::GameThreadHiPri;
			default:
				return EExtendedTaskPriority::GameThreadNormalPri;
			}
		}
	}

	template<typename TaskBodyType>
	FTask LaunchGameThread(const TCHAR* DebugName, TaskBodyType&& TaskBody,
		ETaskPriority TaskPriority = ETaskPriority::Normal,
		ETaskFlags Flags = ETaskFlags::None
		)
	{
		return Launch(DebugName, Forward<TaskBodyType>(TaskBody),
			TaskPriority, Private::ConvertPriorityToGameThread(TaskPriority), Flags);
	}

	template<typename TaskBodyType, typename PrerequisitesCollectionType>
	FTask LaunchGameThread(const TCHAR* DebugName, TaskBodyType&& TaskBody,
		PrerequisitesCollectionType&& Prerequisites,
		ETaskPriority TaskPriority = ETaskPriority::Normal,
		ETaskFlags Flags = ETaskFlags::None
		)
	{
		return Launch(DebugName, Forward<TaskBodyType>(TaskBody), Forward<PrerequisitesCollectionType>(Prerequisites),
			TaskPriority, Private::ConvertPriorityToGameThread(TaskPriority), Flags);
	}
}
