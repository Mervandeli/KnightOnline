﻿#pragma once

#include <string>
#include <memory>

// forward declarations
class CIni;

namespace spdlog::details
{
	class thread_pool;
}

namespace logger
{

	class Logger
	{
		// setup defaults
		static constexpr uint16_t MessageQueueSize = 8192;
		static constexpr uint8_t ThreadPoolSize = 1;

	public:
		/// \param appName application name (VersionManager, Aujard, AIServer, Ebenezer)
		Logger(const std::string& appName);

		/// \brief Sets up spdlog from an ini file using standardized server settings
		/// \param ini server application's ini file (already loaded)
		/// \param baseDir base directory to store logs folder under
		void Setup(CIni& ini, const std::string& baseDir);

		virtual void SetupExtraLoggers(CIni& ini,
			std::shared_ptr<spdlog::details::thread_pool> threadPool,
			const std::string& baseDir);

		void SetupExtraLogger(CIni& ini,
			std::shared_ptr<spdlog::details::thread_pool> threadPool,
			const std::string& baseDir,
			const std::string& appName, const std::string& logFileConfigProp);

		virtual ~Logger();

	protected:
		std::string _appName;
		std::string _defaultLogPath;
	};

	// application names used by our loggers
	static constexpr char AIServer[] = "AIServer";
	static constexpr char AIServerItem[] = "AIServerItem";
	static constexpr char AIServerUser[] = "AIServerUser";
	static constexpr char Ebenezer[] = "Ebenezer";
	static constexpr char EbenezerEvent[] = "EbenezerEvent";
	static constexpr char EbenezerRegion[] = "EbenezerRegion";
	static constexpr char Aujard[] = "Aujard";
	static constexpr char VersionManager[] = "VersionManager";
}

namespace ini
{
	// LOGGER section
	static constexpr char LOGGER[] = "LOGGER";
	static constexpr char LEVEL[] = "LEVEL";
	static constexpr char PATTERN[] = "PATTERN";
	static constexpr char FILE[] = "FILE";
	static constexpr char ITEM_LOG_FILE[] = "ITEM_LOG_FILE";
	static constexpr char USER_LOG_FILE[] = "USER_LOG_FILE";
	static constexpr char REGION_LOG_FILE[] = "REGION_LOG_FILE";
	static constexpr char EVENT_LOG_FILE[] = "EVENT_LOG_FILE";

	/// \brief default logger line prefix ([12:59:59][AppName][  level] log line...)
	static constexpr char DEFAULT_LOG_PATTERN[] = "[%H:%M:%S][%n][%7l] %v";
}
