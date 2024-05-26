// Copyright Alex Stevens (@MilkyEngineer). All Rights Reserved.

#include "SaveGameThreading.h"

class FSaveGameThreadQueue final : public ISaveGameThreadQueue
{
public:
	virtual ~FSaveGameThreadQueue() override
	{
		check(IsComplete());
	}

	virtual void AddTask(TFunction<void()>&& Task) override
	{
		WorkQueue.Push(new FTaskFunction(Task));
	}

	void PumpQueue()
	{
		while (FTaskFunction* Function = WorkQueue.Pop())
		{
			(*Function)();
			delete Function;
		}
	}

	bool IsComplete() const { return WorkQueue.IsEmpty(); }

private:
	TLockFreePointerListFIFO<FTaskFunction, PLATFORM_CACHE_LINE_SIZE> WorkQueue;
};

static TSharedPtr<FSaveGameThreadQueue> GSaveGameThreadQueue;

ISaveGameThreadQueue& ISaveGameThreadQueue::Get()
{
	checkf(GSaveGameThreadQueue.IsValid(), TEXT("Use FSaveGameThreadScope to set up a Thread Queue!"));
	return *GSaveGameThreadQueue;
}

FSaveGameTheadScope::FSaveGameTheadScope()
	: ThreadId(FPlatformTLS::GetCurrentThreadId())
{
	check(!GSaveGameThreadQueue.IsValid());
	GSaveGameThreadQueue = MakeShared<FSaveGameThreadQueue>();
}

FSaveGameTheadScope::~FSaveGameTheadScope()
{
	check(GSaveGameThreadQueue.IsValid());
	check(ThreadId == FPlatformTLS::GetCurrentThreadId());
	GSaveGameThreadQueue.Reset();
}

void FSaveGameTheadScope::PumpThread() const
{
	check(ThreadId == FPlatformTLS::GetCurrentThreadId());
	GSaveGameThreadQueue->PumpQueue();
}
