// Copyright Alex Stevens (@MilkyEngineer). All Rights Reserved.

#include "SaveGameThreading.h"

class FSaveGameThreadQueue final : public ISaveGameThreadQueue
{
public:
	FSaveGameThreadQueue()
		: ThreadId(FPlatformTLS::GetCurrentThreadId())
	{}

	virtual ~FSaveGameThreadQueue() override
	{
		check(ThreadId == FPlatformTLS::GetCurrentThreadId());
		check(IsComplete());
	}

	virtual void AddTask(TFunction<void()>&& Task) override
	{
		WorkQueue.Push(new FTaskFunction(Task));
	}

	void ProcessThread()
	{
		check(ThreadId == FPlatformTLS::GetCurrentThreadId());

		while (const FTaskFunction* Function = WorkQueue.Pop())
		{
			(*Function)();
			delete Function;
		}
	}

	bool IsComplete() const { return WorkQueue.IsEmpty(); }

private:
	const uint32 ThreadId;
	TLockFreePointerListFIFO<FTaskFunction, PLATFORM_CACHE_LINE_SIZE> WorkQueue;
};

static TSharedPtr<FSaveGameThreadQueue> GSaveGameThreadQueue;

ISaveGameThreadQueue& ISaveGameThreadQueue::Get()
{
	checkf(GSaveGameThreadQueue.IsValid(), TEXT("Use FSaveGameThreadScope to set up a Thread Queue!"));
	return *GSaveGameThreadQueue;
}

FSaveGameTheadScope::FSaveGameTheadScope()
{
	check(!GSaveGameThreadQueue.IsValid());
	GSaveGameThreadQueue = MakeShared<FSaveGameThreadQueue>();
}

FSaveGameTheadScope::~FSaveGameTheadScope()
{
	check(GSaveGameThreadQueue.IsValid());
	GSaveGameThreadQueue.Reset();
}

void FSaveGameTheadScope::ProcessThread() const
{
	GSaveGameThreadQueue->ProcessThread();
}
