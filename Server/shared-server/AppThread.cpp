#include "pch.h"
#include "AppThread.h"

#include <signal.h>
#include <stdlib.h>
#include <spdlog/spdlog.h>

AppThread* AppThread::s_instance = nullptr;
bool AppThread::s_shutdown = false;

AppThread::AppThread(logger::Logger& logger)
	: _logger(logger), _exitCode(EXIT_SUCCESS)
{
	assert(s_instance == nullptr);
	s_instance = this;
}

AppThread::~AppThread()
{
	assert(s_instance != nullptr);
	s_instance = nullptr;
}

/// \brief The main thread loop for the server instance
void AppThread::thread_loop()
{
	if (!OnStart())
	{
		_exitCode = EXIT_FAILURE;
		return;
	}

	while (_canTick)
	{
		std::unique_lock<std::mutex> lock(_mutex);
		_cv.wait(lock);
	}

	_exitCode = EXIT_SUCCESS;
}

void AppThread::catchInterruptSignals()
{
	// catch interrupt signals for graceful shutdowns.
	signal(SIGINT, signalHandler);
	signal(SIGABRT, signalHandler);
	signal(SIGTERM, signalHandler);
}

void AppThread::signalHandler(int signalNumber)
{
	spdlog::info("AppThread::signalHandler: Caught {}", signalNumber);
	switch (signalNumber)
	{
		case SIGINT:
		case SIGABRT:
		case SIGTERM:
			// Shutdown the application thread
			if (!s_shutdown
				&& s_instance != nullptr)
			{
				s_instance->shutdown(false);
				s_shutdown = true;
			}
			break;
	}

	signal(signalNumber, signalHandler);
}
