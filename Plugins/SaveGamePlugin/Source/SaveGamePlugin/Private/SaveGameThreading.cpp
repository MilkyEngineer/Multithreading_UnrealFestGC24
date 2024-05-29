// Copyright Alex Stevens (@MilkyEngineer). All Rights Reserved.

#include "SaveGameThreading.h"

class FSaveGameThreadQueue final : public ISaveGameThreadQueue
{
public:
	FSaveGameThreadQueue()
		: ThreadId(FPlatformTLS::GetCurrentThreadId())
		, Event(FPlatformProcess::GetSynchEventFromPool(true))
	{}

	virtual ~FSaveGameThreadQueue() override
	{
		check(ThreadId == FPlatformTLS::GetCurrentThreadId());
		check(IsComplete());

		FPlatformProcess::ReturnSynchEventToPool(Event);
		Event = nullptr;
	}

	virtual void AddTask(TFunction<void()>&& Task) override
	{
		WorkQueue.Push(new FTaskFunction(Task));
		Event->Trigger();
	}

	bool ProcessThread(int64 WaitCycles)
	{
		check(ThreadId == FPlatformTLS::GetCurrentThreadId());
		bool bDidWork = false;
		bool bTriggered;

		do
		{
			{
				QUICK_SCOPE_CYCLE_COUNTER(STAT_SaveGame_ProcessThreadQueue);

				while (const FTaskFunction* Function = WorkQueue.Pop())
				{
					(*Function)();
					delete Function;
					bDidWork = true;
				}
			}

			QUICK_SCOPE_CYCLE_COUNTER(STAT_SaveGame_WaitThreadQueue);
			Event->Reset();
			bTriggered = Event->Wait(FTimespan(WaitCycles));
		} while (bTriggered || !IsComplete());

		return bDidWork;
	}

	bool IsComplete() const { return WorkQueue.IsEmpty(); }

private:
	const uint32 ThreadId;
	TLockFreePointerListFIFO<FTaskFunction, PLATFORM_CACHE_LINE_SIZE> WorkQueue;
	FEvent* Event;
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

bool FSaveGameTheadScope::ProcessThread(int64 WaitCycles) const
{
	return GSaveGameThreadQueue->ProcessThread(WaitCycles);
}
