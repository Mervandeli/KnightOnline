#include "pch.h"
#include "AppThread.h"
#include "ftxui_sink_mt.h"
#include "TelnetThread.h"
#include "utilities.h"

#include <shared/Ini.h>

#include <argparse/argparse.hpp>

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include <spdlog/spdlog.h>

#include <csignal>
#include <string>
#include <vector>

AppThread* AppThread::s_instance = nullptr;
bool AppThread::s_shutdown       = false;

AppThread::AppThread(logger::Logger& logger) : _logger(logger)
{
	assert(s_instance == nullptr);
	s_instance = this;

	_iniFile   = new CIni();
}

AppThread::~AppThread()
{
	delete _iniFile;

	if (_telnetThread != nullptr)
	{
		spdlog::debug("AppThread::~AppThread: Shutting down telnet thread.");
		_telnetThread->shutdown();
		spdlog::debug("AppThread::~AppThread: Telnet thread shut down.");
	}

	assert(s_instance != nullptr);
	s_instance = nullptr;
}

std::filesystem::path AppThread::LogBaseDir() const
{
	return std::filesystem::current_path();
}

void AppThread::before_shutdown()
{
	_appStatus = AppStatus::STOPPING;
}

bool AppThread::parse_commandline(int argc, char* argv[])
{
	argparse::ArgumentParser parser(_logger.AppName());

	SetupCommandLineArgParser(parser);

	try
	{
		parser.parse_args(argc, argv);
		return ProcessCommandLineArgs(parser);
	}
	catch (const std::exception& ex)
	{
		spdlog::error("AppThread::parse_commandline: {}", ex.what());
		return false;
	}
}

void AppThread::SetupCommandLineArgParser(argparse::ArgumentParser& parser)
{
	parser.add_argument("--headless")
		.help("run in headless mode, without the ftxui terminal UI for input")
		.flag()
		.store_into(_headless);
	parser.add_argument("--enable-telnet")
		.help("enables the telnet command server, overriding config")
		.flag()
		.store_into(_overrideEnableTelnet);
	parser.add_argument("--telnet-address")
		.help("sets the address for the telnet command server to listen on, overriding config")
		.store_into(_overrideTelnetAddress);
}

bool AppThread::ProcessCommandLineArgs(const argparse::ArgumentParser& /*parser*/)
{
	/* for implementation, only if needed by the app - bound args won't need this */
	return true;
}

void AppThread::thread_loop()
{
	CIni& iniFile         = IniFile();
	bool configFileLoaded = iniFile.Load(ConfigPath());

	// Setup the logger
	_logger.Setup(iniFile, LogBaseDir());

	if (!configFileLoaded)
	{
		std::u8string filenameUtf8 = iniFile.GetPath().u8string();
		std::string filename(filenameUtf8.begin(), filenameUtf8.end());

		spdlog::warn(
			"AppThread::thread_loop: {} does not exist, will use configured defaults.", filename);
	}

	auto color = ftxui::Terminal::ColorSupport();

	// Explicitly run in headless mode to avoid using ftxui.
	if (_headless)
	{
		spdlog::info("AppThread::thread_loop: Running in headless mode. No input available.");
		_exitCode = thread_loop_fallback(iniFile);
	}
	// This isn't a very robust test, but if we're on a terminal with this little support, we shouldn't use ftxui.
	else if (color == ftxui::Terminal::Color::Palette16)
	{
		spdlog::warn("AppThread::thread_loop: No terminal support detected for ftxui. Proceeding "
					 "with regular console logger.");
		_exitCode = thread_loop_fallback(iniFile);
	}
	// We can assume ftxui should be used in all other cases.
	else
	{
		_exitCode = thread_loop_ftxui(iniFile);
	}
}

int AppThread::thread_loop_ftxui(CIni& iniFile)
{
	using namespace ftxui;

	int exitCode        = EXIT_SUCCESS;

	bool isServerLoaded = false;
	auto screen         = ScreenInteractive::Fullscreen();

	auto fxtuiSink      = _logger.FxtuiSink();
	if (fxtuiSink != nullptr)
		fxtuiSink->set_screen(nullptr);

	std::string inputText;
	Elements logElements;

	int focusedLineNumber = 0;
	bool autoScroll       = true;
	int elementCount = 0, lastElementIndex = 0;

	auto input  = Input(&inputText, "Enter command...");

	input      |= CatchEvent(
        [&](const Event& event)
        {
            if (event == Event::Return && isServerLoaded)
            {
                ParseCommand(inputText);
                inputText.clear();
                return true;
            }

            return false;
        });

	auto renderer            = Renderer(input,
				   [&]
				   {
            {
                std::lock_guard<std::mutex> lock(fxtuiSink->lock());
                // this is intentionally a copy, but it's a container of shared pointers
                logElements = fxtuiSink->log_buffer();
            }

            // clamping
            int oldElementCount  = elementCount;
            elementCount         = static_cast<int>(logElements.size());
            lastElementIndex     = std::max(0, elementCount - 1);
            focusedLineNumber    = std::clamp(focusedLineNumber, 0, lastElementIndex);

            float scrollPosition = 0.0f;
            if (elementCount > 0)
                scrollPosition = std::clamp(
                    static_cast<float>(focusedLineNumber) / static_cast<float>(elementCount), 0.0f,
                    1.0f);

            // Auto-scroll to bottom when new lines are added
            if (autoScroll && oldElementCount != elementCount)
            {
                focusedLineNumber = lastElementIndex;
                scrollPosition    = 1.0f;
            }

            // render
            auto logDisplay = vbox(logElements) | focusPositionRelative(0, scrollPosition)
                              | vscroll_indicator | yframe | flex;

            auto inputBox = hbox({ text(" Command: ") | bold, input->Render() | flex }) | border;

            if (!isServerLoaded)
                return logDisplay;

            return vbox({ logDisplay, inputBox });
        });

	constexpr int PageSize   = 10;
	constexpr int WheelSize  = 3;

	renderer                |= CatchEvent(
        [&](Event event)
        {
            // Keyboard events
            if (event == Event::ArrowUp)
            {
                --focusedLineNumber;
                return true;
            }

            if (event == Event::ArrowDown)
            {
                ++focusedLineNumber;

                if (focusedLineNumber >= lastElementIndex)
                    autoScroll = true;
                return true;
            }

            if (event == Event::PageUp)
            {
                focusedLineNumber -= PageSize;
                return true;
            }

            if (event == Event::PageDown)
            {
                focusedLineNumber += PageSize;

                if (focusedLineNumber >= lastElementIndex)
                    autoScroll = true;
                return true;
            }

            if (event == Event::Home)
            {
                focusedLineNumber = 0;
                return true;
            }

            if (event == Event::End)
            {
                focusedLineNumber = std::max(0, elementCount - 1);
                autoScroll        = true;
                return true;
            }

            // Mouse events
            if (event.is_mouse())
            {
                if (event.mouse().button == Mouse::WheelUp)
                {
                    focusedLineNumber -= WheelSize;
                    return true;
                }

                if (event.mouse().button == Mouse::WheelDown)
                {
                    focusedLineNumber += WheelSize;

                    if (focusedLineNumber >= lastElementIndex)
                        autoScroll = true;
                    return true;
                }
            }

            return HandleInputEvent(event);
        });

	std::thread uiThread(
		[&]
		{
			fxtuiSink->set_screen(&screen);
			screen.Loop(renderer);
			fxtuiSink->set_screen(nullptr);

			shutdown(false);
		});

	if (StartupImpl(iniFile))
	{
		isServerLoaded = true;
	}
	else
	{
		exitCode = EXIT_FAILURE;
		shutdown(false);
	}

	while (CanTick())
	{
		std::unique_lock<std::mutex> lock(ThreadMutex());
		ThreadCondition().wait(lock);
	}

	screen.Exit();

	if (uiThread.joinable())
		uiThread.join();

	return exitCode;
}

int AppThread::thread_loop_fallback(CIni& iniFile)
{
	auto ftxuiSink = _logger.FxtuiSink();
	if (ftxuiSink != nullptr)
		ftxuiSink->disable_log_buffer();

	int exitCode = EXIT_SUCCESS;
	if (!StartupImpl(iniFile))
	{
		exitCode = EXIT_FAILURE;
		shutdown(false);
	}

	while (CanTick())
	{
		std::unique_lock<std::mutex> lock(ThreadMutex());
		ThreadCondition().wait(lock);
	}

	return exitCode;
}

bool AppThread::LoadConfig(CIni& /*iniFile*/)
{
	return true;
}

bool AppThread::StartupImpl(CIni& iniFile)
{
	try
	{
		// Load application-specific config
		if (!LoadConfig(iniFile))
		{
			spdlog::error("AppThread::StartupImpl: LoadConfig() failed.");
			return false;
		}

		// Load Telnet config
		_enableTelnet         = iniFile.GetBool("TELNET", "ENABLED", _enableTelnet);
		_telnetAddress        = iniFile.GetString("TELNET", "IP", _telnetAddress);
		_telnetPort           = iniFile.GetInt("TELNET", "PORT", _telnetPort);
		std::string whitelist = iniFile.GetString("TELNET", "WHITELIST", "127.0.0.1");

		if (!_overrideTelnetAddress.empty())
			_telnetAddress = _overrideTelnetAddress;

		// Trigger a save to flush defaults to file.
		iniFile.Save();

		// Start the Telnet Server, if enabled
		if (_enableTelnet || _overrideEnableTelnet)
		{
			std::unordered_set<std::string> addressWhiteList;
			std::stringstream ss(whitelist);
			std::string value;
			while (std::getline(ss, value, ','))
				addressWhiteList.insert(value);

			_telnetThread = new TelnetThread(
				_telnetAddress, _telnetPort, std::move(addressWhiteList));

			spdlog::info("AppThread::StartupImpl: Telnet thread starting on {}:{}", _telnetAddress,
				_telnetPort);
			_telnetThread->start();
		}

		// Load application-specific startup logic
		_appStatus = AppStatus::STARTING;

		if (!OnStart())
		{
			spdlog::error("AppThread::StartupImpl: OnStart() failed.");
			return false;
		}

		_appStatus = AppStatus::READY;
		return true;
	}
	catch (const std::exception& ex)
	{
		spdlog::error("AppThread::StartupImpl: unhandled exception - {}", ex.what());
		return false;
	}
}

bool AppThread::HandleInputEvent(const ftxui::Event& /*event*/)
{
	return false;
}

void AppThread::ParseCommand(const std::string& command)
{
	if (command.empty())
		return;

	if (HandleCommand(command))
		spdlog::info("Command handled: {}", command);
	else
		spdlog::warn("Command not handled: {}", command);
}

bool AppThread::HandleCommand(const std::string& command)
{
	if (command == "/clear")
	{
		auto fxtuiSink = _logger.FxtuiSink();
		if (fxtuiSink != nullptr)
		{
			std::lock_guard<std::mutex> lock(fxtuiSink->lock());
			fxtuiSink->log_buffer().clear();
		}

		spdlog::info("Logs cleared");
		return true;
	}

	if (command == "/exit")
	{
		shutdown(false);
		return true;
	}

	return false;
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
			if (!s_shutdown && s_instance != nullptr)
			{
				s_instance->shutdown(false);
				s_shutdown = true;
			}
			break;

		default:
			break;
	}

	signal(signalNumber, signalHandler);
}
