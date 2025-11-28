#pragma once

#include "Define.h"
#include "DBProcess.h"

#include <shared/Thread.h>

#include <shared-server/logger.h>
#include <shared-server/SocketManager.h>

#include <vector>
#include <string>

namespace recordset_loader
{
	struct Error;
}

class TimerThread;
class VersionManagerInstance : public Thread
{
	using ServerInfoList = std::vector<_SERVER_INFO*>;

public:
	static VersionManagerInstance* instance()
	{
		return s_instance;
	}

	const char* FtpUrl() const
	{
		return _ftpUrl;
	}

	const char* FtpPath() const
	{
		return _ftpPath;
	}

	int LastVersion() const
	{
		return _lastVersion;
	}

	VersionManagerInstance(logger::Logger& logger);
	~VersionManagerInstance();
	bool GetInfoFromIni();
	bool LoadVersionList();

	SocketManager	_socketManager;

	VersionInfoList	VersionList;
	ServerInfoList	ServerList;
	_NEWS			News;
	CDBProcess		DbProcess;

protected:
	/// \brief Loads config, database caches, then starts sockets and thread pools.
	/// \returns true when successful, false otherwise
	bool OnStart();

	/// \brief The main thread loop for the server instance
	void thread_loop() override;

protected:
	char			_ftpUrl[256];
	char			_ftpPath[256];

	int				_lastVersion;

	logger::Logger&	_logger;

	std::unique_ptr<TimerThread>	_dbPoolCheckThread;

	static VersionManagerInstance* s_instance;
};
