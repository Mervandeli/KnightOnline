#include "pch.h"
#include "VersionManagerInstance.h"
#include "User.h"

#include <db-library/ConnectionManager.h>
#include <db-library/RecordSetLoader.h>
#include <shared/Ini.h>
#include <shared/TimerThread.h>

#include <spdlog/spdlog.h>

import VersionManagerBinder;

constexpr int WM_PROCESS_LISTBOX_QUEUE = WM_APP + 1;

using namespace std::chrono_literals;

VersionManagerInstance::VersionManagerInstance(logger::Logger& logger)
	: AppThread(logger),
	_socketManager(SOCKET_BUFF_SIZE, SOCKET_BUFF_SIZE)
{
	memset(_ftpUrl, 0, sizeof(_ftpUrl));
	memset(_ftpPath, 0, sizeof(_ftpPath));
	_lastVersion = 0;

	db::ConnectionManager::DefaultConnectionTimeout = DB_PROCESS_TIMEOUT;
	db::ConnectionManager::Create();

	_dbPoolCheckThread = std::make_unique<TimerThread>(
		1min,
		std::bind(&db::ConnectionManager::ExpireUnusedPoolConnections));
}

VersionManagerInstance::~VersionManagerInstance()
{
	spdlog::info("VersionManagerInstance::~VersionManagerInstance: Shutting down, releasing resources.");
	_socketManager.Shutdown();
	spdlog::info("VersionManagerInstance::~VersionManagerInstance: SocketManager stopped.");

	spdlog::info("VersionManagerInstance::~VersionManagerInstance: Waiting for worker threads to fully shut down.");

	if (_dbPoolCheckThread != nullptr)
	{
		spdlog::info("VersionManagerInstance::~VersionManagerInstance: Shutting down CheckAliveThread...");

		_dbPoolCheckThread->shutdown();

		spdlog::info("VersionManagerInstance::~VersionManagerInstance: DB pool check thread stopped.");
	}

	spdlog::info("VersionManagerInstance::~VersionManagerInstance: All worker threads stopped, freeing caches.");

	for (_SERVER_INFO* pInfo : ServerList)
		delete pInfo;
	ServerList.clear();

	spdlog::info("VersionManagerInstance::~VersionManagerInstance: All resources safely released.");

	db::ConnectionManager::Destroy();
}

bool VersionManagerInstance::OnStart()
{
	_socketManager.Init(MAX_USER, 0, 1);
	_socketManager.AllocateServerSockets<CUser>();

	if (!GetInfoFromIni())
	{
		spdlog::error("Ini File Info Error!!");
		return false;
	}

	// print the ODBC connection string
	// TODO: modelUtil::DbType::ACCOUNT;  Currently all models are assigned to GAME
	spdlog::debug(
		db::ConnectionManager::GetOdbcConnectionString(modelUtil::DbType::GAME));

	if (!DbProcess.InitDatabase())
	{
		spdlog::error("Database Connection Fail!!");
		return false;
	}

	if (!LoadVersionList())
	{
		spdlog::error("Load Version List Fail!!");
		return false;
	}

	if (!_socketManager.Listen(_LISTEN_PORT))
	{
		spdlog::error("FAIL TO CREATE LISTEN STATE");
		return false;
	}

	_socketManager.StartAccept();

	spdlog::info("Listening on 0.0.0.0:{}", _LISTEN_PORT);

	_dbPoolCheckThread->start();

	return true;
}

bool VersionManagerInstance::GetInfoFromIni()
{
	std::filesystem::path exePath = GetProgPath();
	std::filesystem::path iniPath = exePath / "Version.ini";

	CIni ini(iniPath);

	// ftp config
	ini.GetString(ini::DOWNLOAD, ini::URL, "127.0.0.1", _ftpUrl, _countof(_ftpUrl));
	ini.GetString(ini::DOWNLOAD, ini::PATH, "/", _ftpPath, _countof(_ftpPath));

	// configure logger
	_logger.Setup(ini, exePath);
	
	// TODO: KN_online should be Knight_Account
	std::string datasourceName = ini.GetString(ini::ODBC, ini::DSN, "KN_online");
	std::string datasourceUser = ini.GetString(ini::ODBC, ini::UID, "knight");
	std::string datasourcePass = ini.GetString(ini::ODBC, ini::PWD, "knight");

	db::ConnectionManager::SetDatasourceConfig(
		modelUtil::DbType::ACCOUNT,
		datasourceName, datasourceUser, datasourcePass);

	// TODO: Remove this - currently all models are assigned to GAME
	db::ConnectionManager::SetDatasourceConfig(
		modelUtil::DbType::GAME,
		datasourceName, datasourceUser, datasourcePass);

	int serverCount = ini.GetInt(ini::SERVER_LIST, ini::COUNT, 1);

	if (strlen(_ftpUrl) == 0
		|| strlen(_ftpPath) == 0)
		return false;

	if (datasourceName.length() == 0
		// TODO: Should we not validate UID/Pass length?  Would that allow Windows Auth?
		|| datasourceUser.length() == 0
		|| datasourcePass.length() == 0)
		return false;

	if (serverCount <= 0)
		return false;

	char key[20] = {};
	ServerList.reserve(serverCount);

	for (int i = 0; i < serverCount; i++)
	{
		_SERVER_INFO* pInfo = new _SERVER_INFO;

		snprintf(key, sizeof(key), "SERVER_%02d", i);
		ini.GetString(ini::SERVER_LIST, key, "127.0.0.1", pInfo->strServerIP, _countof(pInfo->strServerIP));

		snprintf(key, sizeof(key), "NAME_%02d", i);
		ini.GetString(ini::SERVER_LIST, key, "TEST|Server 1", pInfo->strServerName, _countof(pInfo->strServerName));

		snprintf(key, sizeof(key), "ID_%02d", i);
		pInfo->sServerID = static_cast<int16_t>(ini.GetInt(ini::SERVER_LIST, key, 1));

		snprintf(key, sizeof(key), "USER_LIMIT_%02d", i);
		pInfo->sUserLimit = static_cast<int16_t>(ini.GetInt(ini::SERVER_LIST, key, MAX_USER));

		ServerList.push_back(pInfo);
	}

	// Read news from INI (max 3 blocks)
	std::stringstream ss;
	std::string title, message;

	News.Size = 0;
	for (int i = 0; i < MAX_NEWS_COUNT; i++)
	{
		snprintf(key, sizeof(key), "TITLE_%02d", i);
		title = ini.GetString("NEWS", key, "");
		if (title.empty())
			continue;

		snprintf(key, sizeof(key), "MESSAGE_%02d", i);
		message = ini.GetString("NEWS", key, "");
		if (message.empty())
			continue;

		ss << title;
		ss.write(NEWS_MESSAGE_START, sizeof(NEWS_MESSAGE_START));
		ss << message;
		ss.write(NEWS_MESSAGE_END, sizeof(NEWS_MESSAGE_END));
	}

	const std::string newsContent = ss.str();
	if (!newsContent.empty())
	{
		if (newsContent.size() > sizeof(News.Content))
		{
			spdlog::error("VersionManagerInstance::GetInfoFromIni: News too long");
			return false;
		}

		memcpy(&News.Content, newsContent.c_str(), newsContent.size());
		News.Size = static_cast<int16_t>(newsContent.size());
	}

	// Trigger a save to flush defaults to file.
	ini.Save();

	spdlog::info("Version Manager initialized");

	return true;
}

bool VersionManagerInstance::LoadVersionList()
{
	VersionInfoList versionList;
	if (!DbProcess.LoadVersionList(&versionList))
		return false;

	int lastVersion = 0;

	for (const auto& [_, pInfo] : versionList)
	{
		if (lastVersion < pInfo->Number)
			lastVersion = pInfo->Number;
	}

	if (lastVersion != _lastVersion)
		spdlog::info("Latest Version: {}", lastVersion);

	_lastVersion = lastVersion;

	VersionList.Swap(versionList);
	return true;
}
