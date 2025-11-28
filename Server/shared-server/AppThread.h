#pragma once

#include <shared/Thread.h>
#include "logger.h"

class AppThread : public Thread
{
public:
	int exitCode() const
	{
		return _exitCode;
	}

	AppThread(logger::Logger& logger);
	~AppThread();

	/// \brief The main thread loop for the server instance
	void thread_loop() override;

	/// \brief Initializes the server, loading caches, socket managers, etc.
	/// \returns true when successful, false otherwise
	virtual bool OnStart() = 0;

	template <typename AppThreadType, typename LoggerType>
	static int main(LoggerType& logger)
	{
		// catch interrupt signals for graceful shutdowns.
		catchInterruptSignals();

		// Logger config/setup is handled by the server instance.
		// We just instantiate it early for signal handling.
		AppThreadType appThread(logger);
		appThread.start();

		// We keep the main() thread alive to catch interrupt signals and call shutdown
		appThread.join();

		return appThread.exitCode();
	}

private:
	static void catchInterruptSignals();
	static void signalHandler(int signalNumber);

protected:
	logger::Logger&		_logger;
	int					_exitCode;

	static AppThread*	s_instance;
	static bool			s_shutdown;
};
