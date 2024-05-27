// Copyright Alex Stevens (@MilkyEngineer). All Rights Reserved.

#pragma once

#include "Templates/FunctionFwd.h"

class ISaveGameThreadQueue
{
public:
	typedef TFunction<void()> FTaskFunction;

	static ISaveGameThreadQueue& Get();

	virtual ~ISaveGameThreadQueue() = default;
	virtual void AddTask(FTaskFunction&& Task) = 0;
};

class FSaveGameTheadScope
{
public:
	FSaveGameTheadScope();
	~FSaveGameTheadScope();

	void ProcessThread() const;
};
