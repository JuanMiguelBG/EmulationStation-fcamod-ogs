//EmulationStation, a graphical front-end for ROM browsing. Created by Alec "Aloshi" Lofquist.
//http://www.aloshi.com

#include "guis/GuiDetectDevice.h"
#include "guis/GuiMsgBox.h"
#include "utils/FileSystemUtil.h"
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

void playVideo()
{
	LOG(LogInfo) << "MAIN::playVideo() - playing video splash \"" << gPlayVideo << '"';
	Settings::getInstance()->setBool("AlwaysOnTop", true);

	Window window;
	if (!window.init(true, true))
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
		else if (strcmp(argv[i], "--debug") == 0)
		{
			Settings::getInstance()->setBool("Debug", true);
			Settings::getInstance()->setBool("DebugGrid", true);
			Settings::getInstance()->setBool("DebugText", true);
			Settings::getInstance()->setBool("DebugImage", true);
			Log::setReportingLevel(LogDebug);
		}
		else if (strcmp(argv[i], "--fullscreen") == 0)
		{
			Settings::getInstance()->setBool("FullScreenMode", true);
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
	if (Settings::getInstance()->getBool("MusicTitles"))
	{
		std::string songName = AudioManager::getInstance()->popSongName();
		if (!songName.empty())
			window->displayNotificationMessage(_U("\uF028  ") + songName);
	}
}

//#include "ApiSystem.h"

int main(int argc, char* argv[])
{
	srand((unsigned int)time(NULL));

	std::locale::global(std::locale("C"));

	if(!parseArgs(argc, argv))
		return 0;

	//start the logger
	Log::setupReportingLevel();
	Log::init();
	LOG(LogInfo) << "MAIN::main() - EmulationStation - v" << PROGRAM_VERSION_STRING << ", built " << PROGRAM_BUILT_STRING;

/*
	ApiSystem::getInstance()->checkUpdateVersion();
	ApiSystem::getInstance()->updateSystem(nullptr);
	return 0;
	*/
	// only show the console on Windows if HideConsole is false

	// call this ONLY when linking with FreeImage as a static library
#ifdef FREEIMAGE_LIB
	FreeImage_Initialise();
#endif

	if (!gPlayVideo.empty())
	{
		playVideo();
		Log::flush();
		return 0;
	}

	//if ~/.emulationstation doesn't exist and cannot be created, bail
	if(!verifyHomeFolderExists())
	{
		Log::flush();
		return 1;
	}

	//always close the log on exit
	atexit(&onExit);

	// metadata init
	//Genres::init();
	MetaDataList::initMetadata();

	Window window;
	SystemScreenSaver screensaver(&window);
	PowerSaver::init();
	ViewController::init(&window);
	CollectionSystemManager::init(&window);
	MameNames::init();
	window.pushGui(ViewController::get());

	TextureData::OPTIMIZEVRAM = Settings::getInstance()->getBool("OptimizeVRAM");
	GuiComponent::ALLOWANIMATIONS = Settings::getInstance()->getString("TransitionStyle") != "instant";

	bool splashScreen = Settings::getInstance()->getBool("SplashScreen");
	bool splashScreenProgress = Settings::getInstance()->getBool("SplashScreenProgress");

	if (!scrape_cmdline)
	{
		if(!window.init(true, Settings::getInstance()->getBool("FullScreenMode")))
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

	//run the command line scraper then quit
	if (scrape_cmdline)
		return run_scraper_cmdline();

	//dont generate joystick events while we're loading (hopefully fixes "automatically started emulator" bug)
	SDL_JoystickEventState(SDL_DISABLE);

	// preload what we can right away instead of waiting for the user to select it
	// this makes for no delays when accessing content, but a longer startup time
	ViewController::get()->preload();

	// Initialize input
	InputConfig::AssignActionButtons();

	//choose which GUI to open depending on if an input configuration already exists
	if (errorMsg == NULL)
	{
//		if (Utils::FileSystem::exists(InputManager::getConfigPath()) && InputManager::getInstance()->getNumConfiguredDevices() > 0)
			ViewController::get()->goToStart(true);
//		else
//			window.pushGui(new GuiDetectDevice(&window, true, [] { ViewController::get()->goToStart(true); }));
	}

	//generate joystick events since we're done loading
	SDL_JoystickEventState(SDL_ENABLE);

	window.endRenderLoadingScreen();

	if (Settings::getInstance()->getBool("audio.bgmusic"))
		AudioManager::getInstance()->playRandomMusic();

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

	ThreadedScraper::stop();

	ApiSystem::getInstance()->deinit();

	while(window.peekGui() != ViewController::get())
		delete window.peekGui();

	if (SystemData::hasDirtySystems())
		window.renderLoadingScreen(_("SAVING DATA. PLEASE WAIT..."));

	ImageIO::saveImageCache();
	MameNames::deinit();
	CollectionSystemManager::deinit();
	SystemData::deleteSystems();

	// call this ONLY when linking with FreeImage as a static library
#ifdef FREEIMAGE_LIB
	FreeImage_DeInitialise();
#endif

	window.deinit(true);

	processQuitMode();

	LOG(LogInfo) << "MAIN::main() - EmulationStation cleanly shutting down.";
	Log::flush();

	return 0;
}
