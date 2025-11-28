// VersionManager.cpp : contains the main() function to start the server
//

#include "pch.h"
#include "VersionManagerInstance.h"

#include <signal.h>
#include <spdlog/spdlog.h>

VersionManagerInstance* _appThread = nullptr;

void signalHandler(int signalNumber)
{
	spdlog::info("VersionManager::signalHandler: Caught {}", signalNumber);
	switch (signalNumber)
	{
		case SIGINT:
		case SIGABRT:
		case SIGTERM:
			// Shutdown the application thread
			if (_appThread != nullptr)
			{
				_appThread->shutdown(false);
				_appThread = nullptr;
			}
			break;
	}

	signal(signalNumber, signalHandler);
}

int main()
{
	logger::Logger logger(logger::VersionManager);

	// catch interrupt signals for graceful shutdowns.
	signal(SIGINT, signalHandler);
	signal(SIGABRT, signalHandler);
	signal(SIGTERM, signalHandler);

	// Logger config/setup is handled by the server instance.
	// We just instantiate it early for signal handling.
	VersionManagerInstance appThread(logger);
	_appThread = &appThread;
	appThread.start();

	// We keep the main() thread alive to catch interrupt signals and call shutdown
	appThread.join();
	_appThread = nullptr;

	return EXIT_SUCCESS;
}
