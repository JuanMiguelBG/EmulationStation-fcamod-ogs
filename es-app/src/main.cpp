//EmulationStation, a graphical front-end for ROM browsing. Created by Alec "Aloshi" Lofquist.
//http://www.aloshi.com

#include "guis/GuiDetectDevice.h"
#include "guis/GuiMsgBox.h"
#include "utils/FileSystemUtil.h"

#if defined(USE_PROFILING)
#include "utils/ProfilingUtil.h"
#endif // USE_PROFILING

#include "views/ViewController.h"
#include "CollectionSystemManager.h"
#include "EmulationStation.h"
#include "InputManager.h"
#include "Log.h"
#include "MameNames.h"
#include "platform.h"
#include "PowerSaver.h"
#include "ScraperCmdLine.h"
#include "Settings.h"
#include "SystemConf.h"
#include "SystemData.h"
#include "SystemScreenSaver.h"
#include <SDL_events.h>
#include <SDL_main.h>
#include <SDL_timer.h>
#include <iostream>
#include <time.h>

#include <unistd.h>

#include "resources/TextureData.h"
#include <FreeImage.h>
#include "AudioManager.h"
#include "NetworkThread.h"
#include "scrapers/ThreadedScraper.h"
#include "ImageIO.h"
#include "ApiSystem.h"

#include "utils/AsyncUtil.h"
#include "Scripting.h"

#include <thread>
#include <chrono>
#include <csignal>

const std::string INVALID_HOME_PATH = "Invalid home path supplied.";
const std::string INVALID_CONFIG_PATH = "Invalid config path supplied.";
const std::string INVALID_USERDATA_PATH = "Invalid userdata path supplied.";
const std::string INVALID_SCREEN_ROTATE = "Invalid screenrotate supplied.";
const std::string ERROR_CONFIG_DIRECTORY = "Config directory could not be created!";
const std::string WINDOW_FAILED_INITIALIZE = "Window failed to initialize!";

bool scrape_cmdline = false;

#include "components/VideoVlcComponent.h"

static std::string gPlayVideo;
static int gPlayVideoDuration = 0;

struct BootGameInfo
{
	BootGameInfo()
	{
		name = "N/A";
		system = "N/A";
		enable_startup_game = true;
		launched = false;
		time_played = 0;
		date = Utils::Time::now();
	}

	std::string name;
	std::string system;
	bool enable_startup_game;
	bool launched;
	long time_played;
	time_t date;
};
static BootGameInfo bootGame;

void playVideo()
{
	LOG(LogInfo) << "MAIN::playVideo() - playing video splash \"" << gPlayVideo << '"';
	Settings::getInstance()->setBool("AlwaysOnTop", true);

	Window window;
	if (!window.init(true))
	{
		LOG(LogError) << WINDOW_FAILED_INITIALIZE;
		return;
	}

	Settings::getInstance()->setBool("VideoAudio", true);

	bool exitLoop = false;

	VideoVlcComponent vid(&window);
	vid.setVideo(gPlayVideo);
	vid.setOrigin(0.5f, 0.5f);
	vid.setPosition(Renderer::getScreenWidth() / 2.0f, Renderer::getScreenHeight() / 2.0f);
	vid.setMaxSize(Renderer::getScreenWidth(), Renderer::getScreenHeight());

	vid.setOnVideoEnded([&exitLoop]()
	{
		exitLoop = true;
		return false;
	});

	window.pushGui(&vid);

	vid.onShow();
	vid.topWindow(true);

	unsigned int firstTime = SDL_GetTicks();
	unsigned int totalTime = 0;
	unsigned int deltaTime = 1000;

	while (!exitLoop)
	{
		SDL_Event event;

		if (SDL_PollEvent(&event))
		{
			do
			{
				if (event.type == SDL_QUIT)
				{
					LOG(LogInfo) << "MAIN::playVideo() - exit by SDL_QUIT";
					return;
				}
			} while (SDL_PollEvent(&event));
		}

		if (vid.isPlaying())
		{
			unsigned int totalTime = SDL_GetTicks() - firstTime;

			if (gPlayVideoDuration > 0 && totalTime > gPlayVideoDuration )
			{
				LOG(LogInfo) << "MAIN::playVideo() - exit by videoduration";
				break;
			}
		}

		Transform4x4f transform = Transform4x4f::Identity();
		vid.update(deltaTime);
		vid.render(transform);

		Renderer::swapBuffers();
	}
	window.deinit(true);
}

bool parseArgs(int argc, char* argv[])
{
	//LOG(LogInfo) << "MAIN::parseArgs() - parsing arguments, number: " << std::to_string(argc);
	Utils::FileSystem::setExePath(argv[0]);
	// We need to process --home before any call to Settings::getInstance(), because settings are loaded from homepath
	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "--home") == 0)
		{
			if (i >= argc - 1)
			{
				std::cerr << INVALID_HOME_PATH;
				//LOG(LogError) << "MAIN::parseArgs()" << INVALID_HOME_PATH;
				return false;
			}
			Utils::FileSystem::setHomePath(argv[i + 1]);
			break;
		}
		else if (strcmp(argv[i], "--config-path") == 0)
		{
			if (i >= argc - 1)
			{
				std::cerr << INVALID_CONFIG_PATH;
				//LOG(LogError) << "MAIN::parseArgs()" << INVALID_CONFIG_PATH;
				return false;
			}
			Utils::FileSystem::setEsConfigPath(argv[i + 1]);
			i++; // skip the argument value
		}
		else if (strcmp(argv[i], "--userdata-path") == 0)
		{
			if (i >= argc - 1)
			{
				std::cerr << INVALID_USERDATA_PATH;
				//LOG(LogError) << "MAIN::parseArgs()" << INVALID_USERDATA_PATH;
				return false;
			}
			Utils::FileSystem::setUserDataPath(argv[i + 1]);
			i++; // skip the argument value
		}
	}

	for(int i = 1; i < argc; i++)
	{
		//LOG(LogInfo) << "MAIN::parseArgs() - execution argument: " << argv[i];
		if (strcmp(argv[i], "--videoduration") == 0)
		{
			gPlayVideoDuration = atoi(argv[i + 1]);
			i++; // skip the argument value
		}
		else if (strcmp(argv[i], "--video") == 0)
		{
			gPlayVideo = argv[i + 1];
			i++; // skip the argument value
		}
		else if (strcmp(argv[i], "--screenrotate") == 0)
		{
			if (i >= argc - 1)
			{
				std::cerr << INVALID_SCREEN_ROTATE;
				//LOG(LogError) << "MAIN::parseArgs() - " << INVALID_SCREEN_ROTATE;
				return false;
			}

			int rotate = atoi(argv[i + 1]);
			++i; // skip the argument value
			Settings::getInstance()->setInt("ScreenRotate", rotate);
		}
		else if (strcmp(argv[i], "--gamelist-only") == 0)
		{
			Settings::getInstance()->setBool("ParseGamelistOnly", true);
		}
		else if (strcmp(argv[i], "--ignore-gamelist") == 0)
		{
			Settings::getInstance()->setBool("IgnoreGamelist", true);
		}
		else if (strcmp(argv[i], "--show-hidden-files") == 0)
		{
			Settings::getInstance()->setBool("ShowHiddenFiles", true);
		}
		else if (strcmp(argv[i], "--draw-framerate") == 0)
		{
			Settings::getInstance()->setBool("DrawFramerate", true);
		}
		else if (strcmp(argv[i], "--no-exit") == 0)
		{
			Settings::getInstance()->setBool("ShowExit", false);
		}
		else if (strcmp(argv[i], "--no-splash") == 0)
		{
			Settings::getInstance()->setBool("SplashScreen", false);
		}
		else if(strcmp(argv[i], "--no-startup-game") == 0)
		{
			bootGame.enable_startup_game = false;
		}
		else if (strcmp(argv[i], "--debug") == 0)
		{
			Settings::getInstance()->setBool("Debug", true);
			Settings::getInstance()->setBool("DebugGrid", true);
			Settings::getInstance()->setBool("DebugText", true);
			Settings::getInstance()->setBool("DebugImage", true);
			Log::setReportingLevel(LogDebug);
		}
		else if (strcmp(argv[i], "--no-preload-vlc") == 0)
		{
			Settings::getInstance()->setBool("PreloadVLC", false);
		}
		else if (strcmp(argv[i], "--vsync") == 0 || strcmp(argv[i], "-vsync") == 0)
		{			
			bool vsync = false;
			if (i == argc - 1)
				vsync = true;
			else
			{
				std::string arg = argv[i + 1];
				if (arg.find("-") == 0)
					vsync = true;
				else
				{
					vsync = (strcmp(argv[i + 1], "on") == 0 || strcmp(argv[i + 1], "1") == 0) ? true : false;
					i++; // skip vsync value
				}
			}
			Settings::getInstance()->setBool("VSync", vsync);
		}
		else if (strcmp(argv[i], "--scrape") == 0)
		{
			scrape_cmdline = true;
		}
		else if (strcmp(argv[i], "--max-vram") == 0)
		{
			int maxVRAM = atoi(argv[i + 1]);
			Settings::getInstance()->setInt("MaxVRAM", maxVRAM);
		}
		else if (strcmp(argv[i], "--force-kiosk") == 0)
		{
			Settings::getInstance()->setBool("ForceKiosk", true);
		}
		else if (strcmp(argv[i], "--force-kid") == 0)
		{
			Settings::getInstance()->setBool("ForceKid", true);
		}
		else if (strcmp(argv[i], "--force-disable-filters") == 0)
		{
			Settings::getInstance()->setBool("ForceDisableFilters", true);
		}
		else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
		{
			std::cout <<
				"EmulationStation, a graphical front-end for ROM browsing.\n"
				"Written by Alec \"Aloshi\" Lofquist.\n"
				"Version " << PROGRAM_VERSION_STRING << ", built " << PROGRAM_BUILT_STRING << "\n\n"
				"Command line arguments:\n"
				"--gamelist-only			skip automatic game search, only read from gamelist.xml\n"
				"--ignore-gamelist		ignore the gamelist (useful for troubleshooting)\n"
				"--draw-framerate		display the framerate\n"
				"--no-exit			don't show the exit option in the menu\n"
				"--no-splash			don't show the splash screen\n"
				"--debug				more logging, show console on Windows\n"
				"--scrape			scrape using command line interface\n"
				"--vsync [1/on or 0/off]		turn vsync on or off (default is on)\n"
				"--max-vram [size]		Max VRAM to use in Mb before swapping. 0 for unlimited\n"
				"--force-kid		Force the UI mode to be Kid\n"
				"--force-kiosk		Force the UI mode to be Kiosk\n"
				"--force-disable-filters		Force the UI to ignore applied filters in gamelist\n"
				"--video		path to the video spalsh\n"
				"--videoduration		the video spalsh durarion in milliseconds\n"
				"--fullscreen		use fullscreen  mode\n"
				"--config-path		set config directory path\n"
				"--userdata-path		set userdata directory path, default '/roms'\n"
				"--help, -h			summon a sentient, angry tuba\n\n"
				"More information available in README.md.\n";
			return false; //exit after printing help
		}
	}

	return true;
}

void loadOtherSettings()
{
	LOG(LogDebug) << "MAIN::loadOtherSettings() - Enter function";
	Utils::Async::run( [] (void)
		{
			if (ApiSystem::getInstance()->isScriptingSupported(ApiSystem::ScriptId::WIFI))
			{
				SystemConf::getInstance()->set("already.connection.exist.flag", ApiSystem::getInstance()->getWifiNetworkExistFlag());
				SystemConf::getInstance()->setBool("wifi.enabled", ApiSystem::getInstance()->isWifiEnabled());
				SystemConf::getInstance()->set("system.hostname", ApiSystem::getInstance()->getHostname());

				std::string ssid = ApiSystem::getInstance()->getWifiSsid();
				if (SystemConf::getInstance()->get("wifi.ssid").empty() || (ssid != SystemConf::getInstance()->get("wifi.ssid")))
					SystemConf::getInstance()->set("wifi.ssid", ssid);

				if (!SystemConf::getInstance()->get("wifi.ssid").empty())
					SystemConf::getInstance()->set("wifi.key", ApiSystem::getInstance()->getWifiPsk(ssid));

				SystemConf::getInstance()->set("wifi.dns1", ApiSystem::getInstance()->getDnsOne());
				SystemConf::getInstance()->set("wifi.dns2", ApiSystem::getInstance()->getDnsTwo());
			}
			if (ApiSystem::getInstance()->isScriptingSupported(ApiSystem::ScriptId::BLUETOOTH))
			{
				bool btEnabled = ApiSystem::getInstance()->isBluetoothEnabled();
				SystemConf::getInstance()->setBool("bluetooth.enabled", btEnabled);
				std::string btAudioDevice = "";
				if (btEnabled)
					btAudioDevice = ApiSystem::getInstance()->getBluetoothAudioDevice();

				SystemConf::getInstance()->set("bluetooth.audio.device", btAudioDevice);
				SystemConf::getInstance()->setBool("bluetooth.audio.connected", !btAudioDevice.empty());
			}
		});
		LOG(LogDebug) << "MAIN::loadOtherSettings() - exit function";
}

bool verifyHomeFolderExists()
{
	//make sure the config directory exists
	std::string configDir = Utils::FileSystem::getEsConfigPath();
	if(!Utils::FileSystem::exists(configDir))
	{
		LOG(LogInfo) << "MAIN::verifyHomeFolderExists() - Creating config directory \"" << configDir << '"';
		Log::flush();
		Utils::FileSystem::createDirectory(configDir);
		if(!Utils::FileSystem::exists(configDir))
		{
			std::cerr << ERROR_CONFIG_DIRECTORY << '\n';
			LOG(LogError) << "MAIN::verifyHomeFolderExists() - " << ERROR_CONFIG_DIRECTORY;
			Log::flush();
			return false;
		}
	}

	return true;
}

// Returns true if everything is OK,
bool loadSystemConfigFile(Window* window, const char** errorString)
{
	*errorString = NULL;
	
	ImageIO::loadImageCache();

	if (!SystemData::loadConfig(window))
	{
		LOG(LogError) << "MAIN::loadSystemConfigFile() - Error while parsing systems configuration file!";

		*errorString = "IT LOOKS LIKE YOUR SYSTEMS CONFIGURATION FILE HAS NOT BEEN SET UP OR IS INVALID. YOU'LL NEED TO DO THIS BY HAND, UNFORTUNATELY.\n\n"
			"VISIT EMULATIONSTATION.ORG FOR MORE INFORMATION.";

		return false;
	}

	if(SystemData::sSystemVector.size() == 0)
	{
		LOG(LogError) << "MAIN::loadSystemConfigFile() - No systems found! Does at least one system have a game present? (check that extensions match!)\n(Also, make sure you've updated your es_systems.cfg for XML!)";

		*errorString = "WE CAN'T FIND ANY SYSTEMS!\n"
			"CHECK THAT YOUR PATHS ARE CORRECT IN THE SYSTEMS CONFIGURATION FILE, "
			"AND YOUR GAME DIRECTORY HAS AT LEAST ONE GAME WITH THE CORRECT EXTENSION.\n\n"
			"VISIT EMULATIONSTATION.ORG FOR MORE INFORMATION.";

		return false;
	}

	return true;
}

//called on exit, assuming we get far enough to have the log initialized
void onExit()
{
	Log::close();
}

void processAudioTitles(Window* window)
{
	if (Settings::getInstance()->getBool("audio.display_titles") && AudioManager::getInstance()->isSongNameChanged())
	{
		std::string songName = AudioManager::getInstance()->getSongName();
		if (!songName.empty())
		{
			int duration = Settings::getInstance()->getInt("audio.display_titles_time");
			if (duration <= 2 || duration > 120)
				duration = 10;

			duration *= 1000;

			window->displayNotificationMessage(_U("\u266B  ") + songName, duration);
		}
		AudioManager::getInstance()->resetSongNameChangedFlag();
	}
}

void playMusic()
{
	// Play music
	if (ViewController::get()->getState().viewing == ViewController::GAME_LIST || ViewController::get()->getState().viewing == ViewController::SYSTEM_SELECT)
		AudioManager::getInstance()->changePlaylist(ViewController::get()->getState().getSystem()->getTheme(), true);
	else
		AudioManager::getInstance()->playRandomMusic();
}


void checkPreloadVlc()
{
	Utils::Async::run( [] (void)
		{
			if (ApiSystem::getInstance()->isScriptingSupported(ApiSystem::ScriptId::PRELOAD_VLC) && Settings::getInstance()->getBool("PreloadVLC"))
				ApiSystem::getInstance()->preloadVLC();
		});
}

void waitPreloadVlc(Window *window)
{
	if (ApiSystem::getInstance()->isScriptingSupported(ApiSystem::ScriptId::PRELOAD_VLC) && Settings::getInstance()->getBool("PreloadVLC"))
	{
		//LOG(LogDebug) << "MAIN::waitPreloadVlc() - Check 'preload_vlc.lock'";
		std::string preloa_vlc_lock_file_path = Utils::FileSystem::getEsConfigPath() + "/preload_vlc.lock";
		if (Utils::FileSystem::exists(preloa_vlc_lock_file_path))
		{
			bool exit = false;
			auto start = std::chrono::high_resolution_clock::now();
			while (!exit)
			{
				//LOG(LogDebug) << "MAIN::waitPreloadVlc() - Check 'preload_vlc.lock' - while()";
				std::this_thread::sleep_for(std::chrono::milliseconds(500));
				auto actual_time = std::chrono::high_resolution_clock::now();
				auto elapsed = actual_time - start;
				auto f_secs = std::chrono::duration_cast<std::chrono::duration<float>>(elapsed);
				//LOG(LogDebug) << "MAIN::waitPreloadVlc() - Check 'preload_vlc.lock' - while() - f_secs: " << std::to_string(f_secs.count());
				window->renderLoadingScreen(_("Preload VLC..."), (f_secs.count() * 3.3 / 100));
				exit = !Utils::FileSystem::exists(preloa_vlc_lock_file_path);
				if (!exit && (f_secs.count() > 30))
				{
					//LOG(LogDebug) << "MAIN::waitPreloadVlc() - Check 'preload_vlc.lock' - while() - exit by time";
					Utils::FileSystem::removeFile(preloa_vlc_lock_file_path);
					exit = true;
				}
			}
			window->renderLoadingScreen(_("Preload VLC..."), 1);
		}
	}
}

void startAutoConnectBluetoothAudioDevice(std::string &log)
{
	if (!ApiSystem::getInstance()->isScriptingSupported(ApiSystem::ScriptId::BLUETOOTH)
		|| !SystemConf::getInstance()->getBool("bluetooth.enabled")
		|| !Settings::getInstance()->getBool("bluetooth.audio.device.autoconnect"))
		return;

	log.append("MAIN::startAutoConnectBluetoothAudioDevice()\n");
	ApiSystem::getInstance()->startAutoConnectBluetoothAudioDevice();
}

void stopAutoConnectBluetoothAudioDevice()
{
	if (!ApiSystem::getInstance()->isScriptingSupported(ApiSystem::ScriptId::BLUETOOTH)
		|| !SystemConf::getInstance()->getBool("bluetooth.enabled")
		|| !Settings::getInstance()->getBool("bluetooth.audio.device.autoconnect"))
		return;

	LOG(LogInfo) << "MAIN::stopAutoConnectBluetoothAudioDevice()";
	ApiSystem::getInstance()->stopAutoConnectBluetoothAudioDevice();
}

void waitForBluetoothDevices()
{
	if (!ApiSystem::getInstance()->isScriptingSupported(ApiSystem::ScriptId::BLUETOOTH)
		|| !SystemConf::getInstance()->getBool("bluetooth.enabled"))
		return;

	int bt_timeout = Settings::getInstance()->getInt("bluetooth.boot.game.timeout");
	if (bt_timeout > 0)
		Utils::Async::sleep(bt_timeout * 1000);
}

void removeLockFiles()
{
	Utils::FileSystem::removeFile( Utils::FileSystem::getEsConfigPath() + "/brightness.lock" );
}

void addLogs(std::string &logs)
{
	if (!logs.empty())
	{
		for (auto log_entry : Utils::String::split(logs, '\n', true))
			LOG(LogInfo) << log_entry;

		logs.clear();
	}
}

void signalHandler(int signum) 
{
	if (signum == SIGSEGV)
		LOG(LogError) << "Interrupt signal SIGSEGV received.\n";
	else if (signum == SIGFPE)
		LOG(LogError) << "Interrupt signal SIGFPE received.\n";
	else if (signum == SIGFPE)
		LOG(LogError) << "Interrupt signal SIGFPE received.\n";
	else
		LOG(LogError) << "Interrupt signal (" << signum << ") received.\n";

	// cleanup and close up stuff here  
	exit(signum);
}

void launchStartupGame()
{
	auto gamePath = SystemConf::getInstance()->get("global.bootgame.path");
	if (gamePath.empty() || !Utils::FileSystem::exists(gamePath))
		return;

	auto command = SystemConf::getInstance()->get("global.bootgame.cmd");
	if (command.empty())
		return;

	std::string gameInfo = SystemConf::getInstance()->get("global.bootgame.info");
	if (!gameInfo.empty())
	{
		std::vector<std::string> gameData = Utils::String::split(gameInfo, ';');
		bootGame.system = gameData[0];
		bootGame.name = gameData[1];
	}

	int exitCode = -1;
	const std::string path = SystemConf::getInstance()->get("global.bootgame.path"),
					  rom = Utils::FileSystem::getEscapedPath(path),
					  basename = Utils::FileSystem::getStem(path);

	LOG(LogInfo) << "MAIN::launchStartupGame() - game: '" << bootGame.name << "', system: '" << bootGame.system << "', command: '" << command << "'";

	bootGame.launched = true;
	Scripting::fireEvent("game-start", rom, basename, bootGame.name);
	time_t game_tstart = time(NULL);
	exitCode = runSystemCommand(command, gamePath, nullptr);

	Scripting::fireEvent("game-end");

	if (exitCode == 0)
	{
		//update game time played
		time_t game_tend = time(NULL);
		bootGame.time_played = difftime(game_tend, game_tstart);
		bootGame.date = Utils::Time::now();
	}
	else
		LOG(LogWarning) << "MAIN::launchStartupGame() - ...launch terminated with nonzero exit code " << exitCode << "!";
}

void updateMetadataStartupGame()
{
	//LOG(LogDebug) << "MAIN::updateMetadataStartupGame() - system: '" << bootGame.system << "', game: '" << bootGame.name << "', launched: '" << Utils::String::boolToString(bootGame.launched) << "'";
	if (!bootGame.launched)
		return;

	Utils::Async::run( [] (void)
		{
			CollectionSystemManager::getInstance()->updateBootGameMetadata(bootGame.system, bootGame.name, bootGame.time_played, bootGame.date);
		});
}

int main(int argc, char* argv[])
{
	// signal(SIGABRT, signalHandler);
	signal(SIGFPE, signalHandler);
	signal(SIGILL, signalHandler);
	signal(SIGINT, signalHandler);
	signal(SIGSEGV, signalHandler);
	// signal(SIGTERM, signalHandler);

	srand((unsigned int)time(NULL));

	std::locale::global(std::locale("C"));

	// to store the log prior to starting the logger
	std::string init_log;
	
	init_log.append("MAIN::main(), MAIN : ") .append(std::to_string(Utils::Async::getThreadId())).append("\n");

	startAutoConnectBluetoothAudioDevice(init_log);
	checkPreloadVlc();

	if(!parseArgs(argc, argv))
		return 0;

	// start the logger
	Log::setupReportingLevel();
	Log::init();
	LOG(LogInfo) << "MAIN::main() - EmulationStation - v" << PROGRAM_VERSION_STRING << ", built " << PROGRAM_BUILT_STRING;

	addLogs(init_log);

	// remove special lock files
	removeLockFiles();
/*
	ApiSystem::getInstance()->checkUpdateVersion();
	ApiSystem::getInstance()->updateSystem(nullptr);
	return 0;
*/
	// call this ONLY when linking with FreeImage as a static library
#ifdef FREEIMAGE_LIB
	FreeImage_Initialise();
#endif

	//if ~/.emulationstation doesn't exist and cannot be created, bail
	if(!verifyHomeFolderExists())
	{
		Log::flush();
		return 1;
	}

	if (!gPlayVideo.empty())
	{
		playVideo();
		Log::flush();
		return 0;
	}

	//always close the log on exit
	atexit(&onExit);

	if (bootGame.enable_startup_game) {
		waitForBluetoothDevices();
		// Run boot game, before Window Create for linux
		launchStartupGame();
	}

	// metadata init
	//Genres::init();
	MetaDataList::initMetadata();

	Window window;
	SystemScreenSaver screensaver(&window);
	PowerSaver::init();
	ViewController::init(&window);
	CollectionSystemManager::init(&window);
	VideoVlcComponent::init();

	MameNames::init();
	window.pushGui(ViewController::get());

	TextureData::OPTIMIZEVRAM = Settings::getInstance()->getBool("OptimizeVRAM");
	GuiComponent::ALLOWANIMATIONS = Settings::getInstance()->getString("TransitionStyle") != "instant";

	bool splashScreen = Settings::getInstance()->getBool("SplashScreen");
	bool splashScreenProgress = Settings::getInstance()->getBool("SplashScreenProgress");

	if (!scrape_cmdline)
	{
		if(!window.init(true))
		{
			LOG(LogError) << WINDOW_FAILED_INITIALIZE;
			Log::flush();
			return 1;
		}

		if (splashScreen)
			window.renderLoadingScreen(_("Loading..."));
	}

	const char* errorMsg = NULL;
	if(!loadSystemConfigFile(&window, &errorMsg))
	{
		// something went terribly wrong
		if (errorMsg == NULL)
		{
			LOG(LogError) << "MAIN::main() - Unknown error occured while parsing system config file.";

			if (!scrape_cmdline)
				Renderer::deinit();

			Log::flush();
			return 1;
		}

		// we can't handle es_systems.cfg file problems inside ES itself, so display the error message then quit
		window.pushGui(new GuiMsgBox(&window,
			_(errorMsg),
			_("QUIT"), [] {
				SDL_Event* quit = new SDL_Event();
				quit->type = SDL_QUIT;
				Log::flush();
				SDL_PushEvent(quit);
			}));
	}

	if (bootGame.enable_startup_game) {
		// update game metadata info
		updateMetadataStartupGame();
	}

	//run the command line scraper then quit
	if (scrape_cmdline)
		return run_scraper_cmdline();

	// preload what we can right away instead of waiting for the user to select it
	// this makes for no delays when accessing content, but a longer startup time
	ViewController::get()->preload();

	waitPreloadVlc(&window);

	// Initialize input
	InputConfig::AssignActionButtons();
	InputManager::getInstance()->init();

	//choose which GUI to open depending on if an input configuration already exists
	if (errorMsg == NULL)
	{
//		if (Utils::FileSystem::exists(InputManager::getConfigPath()) && InputManager::getInstance()->getNumConfiguredDevices() > 0)
			ViewController::get()->goToStart(true);
//		else
//			window.pushGui(new GuiDetectDevice(&window, true, [] { ViewController::get()->goToStart(true); }));
	}

	window.endRenderLoadingScreen();

	stopAutoConnectBluetoothAudioDevice();

	loadOtherSettings();

	playMusic();

	unsigned int lastTime = SDL_GetTicks(),
							 ps_time = lastTime;
	int exitMode = 0;

	bool running = true;

	while(running)
	{
		SDL_Event event;
		bool ps_standby = PowerSaver::getState() && (int) SDL_GetTicks() - ps_time > PowerSaver::getMode();

		if (ps_standby ? SDL_WaitEventTimeout(&event, PowerSaver::getTimeout()) : SDL_PollEvent(&event))
		{
			do
			{
				InputManager::getInstance()->parseEvent(event, &window);

				if (event.type == SDL_QUIT)
					running = false;
			} 
			while(SDL_PollEvent(&event));

			// triggered if exiting from SDL_WaitEvent due to event
			if (ps_standby)
				// show as if continuing from last event
				lastTime = SDL_GetTicks();

			// reset counter
			ps_time = SDL_GetTicks();
		}
		else if (ps_standby)
		{
			// If exitting SDL_WaitEventTimeout due to timeout. Trail considering
			// timeout as an event
			ps_time = SDL_GetTicks();
		}

		if (window.isSleeping())
		{
			lastTime = SDL_GetTicks();
			SDL_Delay(10); // this doesn't need to be accurate, we're just giving up our CPU time until something wakes us up
			continue;
		}

		unsigned int curTime = SDL_GetTicks();
		int deltaTime = curTime - lastTime;
		lastTime = curTime;

		// cap deltaTime if it ever goes negative
		if (deltaTime < 0)
			deltaTime = 1000;

		processAudioTitles(&window);

		window.update(deltaTime);
		window.render();

		Log::flush();

		Renderer::swapBuffers();
	}

	if (isFastShutdown())
	{
		LOG(LogInfo) << "MAIN::main() - EmulationStation Fast Shutting Down/Restarting, not saving metadata.";
		Settings::getInstance()->setBool("IgnoreGamelist", true);
	}

	ThreadedScraper::stop();

	ApiSystem::getInstance()->deinit();

	while(window.peekGui() != ViewController::get())
		delete window.peekGui();

	if (SystemData::hasDirtySystems())
		window.renderLoadingScreen(_("SAVING DATA. PLEASE WAIT..."));

	ImageIO::saveImageCache();
	MameNames::deinit();
	ViewController::saveState();
	CollectionSystemManager::deinit();
	SystemData::deleteSystems();

	// call this ONLY when linking with FreeImage as a static library
#ifdef FREEIMAGE_LIB
	FreeImage_DeInitialise();
#endif

	InputManager::getInstance()->deinit();
	window.deinit(true);

	// remove special lock files
	removeLockFiles();

	processQuitMode();

#if defined(USE_PROFILING)
	ProfileDump();
#endif // USE_PROFILING

	LOG(LogInfo) << "MAIN::main() - EmulationStation cleanly shutting down.";
	Log::flush();

	return 0;
}
