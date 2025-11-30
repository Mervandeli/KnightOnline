#pragma once

#include "Define.h"

#include <shared-server/AppThread.h>
#include <shared-server/logger.h>
#include <shared-server/SharedMemoryQueue.h>

class ItemManagerLogger : public logger::Logger
{
public:
	ItemManagerLogger()
		: Logger(logger::ItemManager)
	{
	}

	void SetupExtraLoggers(CIni& ini,
		std::shared_ptr<spdlog::details::thread_pool> threadPool,
		const std::filesystem::path& baseDir) override;
};

class ReadQueueThread;
class ItemManagerInstance : public AppThread
{
public:
	SharedMemoryQueue m_LoggerRecvQueue;
	std::unique_ptr<ReadQueueThread> _readQueueThread;

public:
	static ItemManagerInstance* instance()
	{
		return static_cast<ItemManagerInstance*>(s_instance);
	}

	ItemManagerInstance(ItemManagerLogger& logger);
	~ItemManagerInstance() override;

	bool OnStart() override;

	void ItemLogWrite(const char* pBuf);
	void ExpLogWrite(const char* pBuf);
};
