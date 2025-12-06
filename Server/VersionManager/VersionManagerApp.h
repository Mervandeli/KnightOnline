#pragma once

#include "Define.h"
#include "DBProcess.h"

#include <shared-server/AppThread.h>
#include <shared-server/SocketManager.h>

#include <vector>
#include <string>

class TimerThread;
class VersionManagerApp : public AppThread
{
	using ServerInfoList = std::vector<_SERVER_INFO*>;

public:
	static VersionManagerApp* instance()
	{
		return static_cast<VersionManagerApp*>(s_instance);
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

	VersionManagerApp(logger::Logger& logger);
	~VersionManagerApp();
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
	bool OnStart() override;

protected:
	char			_ftpUrl[256];
	char			_ftpPath[256];

	int				_lastVersion;

	std::unique_ptr<TimerThread>	_dbPoolCheckThread;
};
