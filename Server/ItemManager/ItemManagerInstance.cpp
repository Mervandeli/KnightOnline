#include "pch.h"
#include "ItemManagerInstance.h"
#include "ItemManagerReadQueueThread.h"

#include <shared/Ini.h>
#include <spdlog/spdlog.h>

#include <filesystem>

ItemManagerInstance::ItemManagerInstance(ItemManagerLogger& logger)
	: AppThread(logger)
{
	_readQueueThread = std::make_unique<ItemManagerReadQueueThread>();
}

ItemManagerInstance::~ItemManagerInstance()
{
	spdlog::info("ItemManagerInstance::~ItemManagerInstance: Shutting down, releasing resources.");

	spdlog::info("ItemManagerInstance::~ItemManagerInstance: Waiting for worker threads to fully shut down.");

	if (_readQueueThread != nullptr)
	{
		spdlog::info("ItemManagerInstance::~ItemManagerInstance: Shutting down ReadQueueThread...");

		_readQueueThread->shutdown();

		spdlog::info("ItemManagerInstance::~ItemManagerInstance: ReadQueueThread stopped.");
	}

	spdlog::info("ItemManagerInstance::~ItemManagerInstance: All resources safely released.");
}

bool ItemManagerInstance::OnStart()
{
	//----------------------------------------------------------------------
	//	Logfile initialize
	//----------------------------------------------------------------------
	std::filesystem::path exePath = GetProgPath();
	std::filesystem::path iniPath = exePath / "ItemManager.ini";

	CIni ini(iniPath);

	// configure logger
	_logger.Setup(ini, exePath.string());

	if (!m_LoggerRecvQueue.Open(SMQ_ITEMLOGGER))
	{
		spdlog::error("Shared memory queue not yet available. Run Ebenezer first.");
		return false;
	}

	_readQueueThread->start();

	spdlog::info("ItemManager started");

	return true;
}

void ItemManagerLogger::SetupExtraLoggers(CIni& ini,
	std::shared_ptr<spdlog::details::thread_pool> threadPool,
	const std::string& baseDir)
{
	SetupExtraLogger(ini, threadPool, baseDir, logger::ItemManagerItem, ini::ITEM_LOG_FILE);
	SetupExtraLogger(ini, threadPool, baseDir, logger::ItemManagerExp, ini::EXP_LOG_FILE);
}

void ItemManagerInstance::ItemLogWrite(const char* pBuf)
{
	int index = 0, srclen = 0, tarlen = 0, type = 0, putitem = 0, putcount = 0, putdure = 0;
	int64_t putserial = 0;
	char srcid[MAX_ID_SIZE + 1] = {},
		tarid[MAX_ID_SIZE + 1] = {};

	srclen = GetShort(pBuf, index);
	if (srclen <= 0
		|| srclen > MAX_ID_SIZE)
	{
		spdlog::trace("### ItemLogWrite Fail : srclen = %d ###\n", srclen);
		return;
	}

	GetString(srcid, pBuf, srclen, index);

	tarlen = GetShort(pBuf, index);
	if (tarlen <= 0
		|| tarlen > MAX_ID_SIZE)
	{
		spdlog::trace("### ItemLogWrite Fail : tarlen = %d ###\n", tarlen);
		return;
	}

	GetString(tarid, pBuf, tarlen, index);

	type = GetByte(pBuf, index);
	putserial = GetInt64(pBuf, index);
	putitem = GetDWORD(pBuf, index);
	putcount = GetShort(pBuf, index);
	putdure = GetShort(pBuf, index);

	spdlog::get(logger::ItemManagerItem)->info("{}, {}, {}, {}, {}, {}, {}",
		srcid, tarid, type, putserial, putitem, putcount, putdure);
}

void ItemManagerInstance::ExpLogWrite(const char* pBuf)
{
	int index = 0, aclen = 0, charlen = 0, type = 0, level = 0, exp = 0, loyalty = 0, money = 0;
	char acname[MAX_ID_SIZE + 1] = {},
		charid[MAX_ID_SIZE + 1] = {};

	aclen = GetShort(pBuf, index);
	if (aclen <= 0
		|| aclen > MAX_ID_SIZE)
	{
		spdlog::trace("### ExpLogWrite Fail : tarlen = %d ###\n", aclen);
		return;
	}

	GetString(acname, pBuf, aclen, index);
	charlen = GetShort(pBuf, index);
	if (charlen <= 0
		|| charlen > MAX_ID_SIZE)
	{
		spdlog::trace("### ExpLogWrite Fail : tarlen = %d ###\n", charlen);
		return;
	}

	GetString(charid, pBuf, charlen, index);
	type = GetByte(pBuf, index);
	level = GetByte(pBuf, index);
	exp = GetDWORD(pBuf, index);
	loyalty = GetDWORD(pBuf, index);
	money = GetDWORD(pBuf, index);

	spdlog::get(logger::ItemManagerExp)->info("{}, {}, {}, {}, {}, {}, {}",
		acname, charid, type, level, exp, loyalty, money);
}
