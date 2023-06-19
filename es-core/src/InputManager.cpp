#include "InputManager.h"

#include "utils/FileSystemUtil.h"
#include "CECInput.h"
#include "Log.h"
#include "platform.h"
#include "Scripting.h"
#include "Window.h"
#include <pugixml/src/pugixml.hpp>
#include <SDL.h>
#include <fstream>
#include <iostream>
#include <assert.h>
#include "Settings.h"
#include <algorithm>
#include <mutex>
#include "EsLocale.h"
#include "renderers/Renderer.h"

#define KEYBOARD_GUID_STRING          "-1"
#define CEC_GUID_STRING               "-2"
#define SYSTEM_HOT_KEY_NAME_STRING    "system_hk"

// SO HEY POTENTIAL POOR SAP WHO IS TRYING TO MAKE SENSE OF ALL THIS (by which I mean my future self)
// There are like four distinct IDs used for joysticks (crazy, right?)
// 1. Device index - this is the "lowest level" identifier, and is just the Nth joystick plugged in to the system (like /dev/js#).
//    It can change even if the device is the same, and is only used to open joysticks (required to receive SDL events).
// 2. SDL_JoystickID - this is an ID for each joystick that is supposed to remain consistent between plugging and unplugging.
//    ES doesn't care if it does, though.
// 3. "Device ID" - this is something I made up and is what InputConfig's getDeviceID() returns.
//    This is actually just an SDL_JoystickID (also called instance ID), but -1 means "keyboard" instead of "error."
// 4. Joystick GUID - this is some squashed version of joystick vendor, version, and a bunch of other device-specific things.
//    It should remain the same across runs of the program/system restarts/device reordering and is what I use to identify which joystick to load.

// hack for cec support
int SDL_USER_CECBUTTONDOWN = -1;
int SDL_USER_CECBUTTONUP   = -1;

static std::mutex mJoysticksLock;

InputManager* InputManager::mInstance = NULL;
Delegate<IJoystickChangedEvent> InputManager::joystickChanged;

InputManager::InputManager() : mKeyboardInputConfig(NULL), mCECInputConfig(NULL), mIgnoreKeys(false)
{
}

InputManager::~InputManager()
{
	deinit();
}

InputManager* InputManager::getInstance()
{
	if(!mInstance)
		mInstance = new InputManager();

	return mInstance;
}

void InputManager::init()
{
	LOG(LogInfo) << "InputManager::init()";

	if(initialized())
		deinit();

	rebuildAllJoysticks(false);

	mKeyboardInputConfig = new InputConfig(DEVICE_KEYBOARD, -1, "Keyboard", KEYBOARD_GUID_STRING, 0, 0, 0); 
	loadInputConfig(mKeyboardInputConfig);

#ifdef HAVE_LIBCEC
	SDL_USER_CECBUTTONDOWN = SDL_RegisterEvents(2);
	SDL_USER_CECBUTTONUP   = SDL_USER_CECBUTTONDOWN + 1;
	CECInput::init();
	mCECInputConfig = new InputConfig(DEVICE_CEC, -1, "CEC", CEC_GUID_STRING, 0, 0, 0); 
	loadInputConfig(mCECInputConfig);
#else
	mCECInputConfig = nullptr;
#endif
}

void InputManager::deinit()
{
	LOG(LogInfo) << "InputManager::deinit()";

	if(!initialized())
		return;

	clearJoysticks();

	if(mKeyboardInputConfig != NULL)
	{
		delete mKeyboardInputConfig;
		mKeyboardInputConfig = NULL;
	}

	if(mCECInputConfig != NULL)
	{
		delete mCECInputConfig;
		mCECInputConfig = NULL;
	}

#ifdef HAVE_LIBCEC
	CECInput::deinit();
#endif

	SDL_JoystickEventState(SDL_DISABLE);
	SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
}

int InputManager::getNumJoysticks() 
{ 
	std::unique_lock<std::mutex> lock(mJoysticksLock);
	return (int)mJoysticks.size(); 
}

InputConfig* InputManager::getInputConfigByDevice(int device)
{
	if(device == DEVICE_KEYBOARD)
		return mKeyboardInputConfig;

	if(device == DEVICE_CEC)
		return mCECInputConfig;
	
	return mInputConfigs[device];
}

void InputManager::clearJoysticks()
{
	mJoysticksLock.lock();

	for (auto iter = mJoysticks.begin(); iter != mJoysticks.end(); iter++)
		SDL_JoystickClose(iter->second);

	mJoysticks.clear();

	for (auto iter = mInputConfigs.begin(); iter != mInputConfigs.end(); iter++)
		if (iter->second)
			delete iter->second;

	mInputConfigs.clear();

	for (auto iter = mPrevAxisValues.begin(); iter != mPrevAxisValues.end(); iter++)
		if (iter->second)
			delete[] iter->second;

	mPrevAxisValues.clear();

	mJoysticksLock.unlock();
}

void InputManager::rebuildAllJoysticks(bool deinit)
{
	if (deinit)
		SDL_JoystickEventState(SDL_DISABLE);

	clearJoysticks();

	if (deinit)
		SDL_QuitSubSystem(SDL_INIT_JOYSTICK);

	SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, Settings::getInstance()->getBool("BackgroundJoystickInput") ? "1" : "0");
	SDL_InitSubSystem(SDL_INIT_JOYSTICK);	

	mJoysticksLock.lock();

	int numJoysticks = SDL_NumJoysticks();

	for (int idx = 0; idx < numJoysticks; idx++)
	{
		// open joystick & add to our list
		SDL_Joystick* joy = SDL_JoystickOpen(idx);
		if (joy == nullptr)
			continue;

		// add it to our list so we can close it again later
		SDL_JoystickID joyId = SDL_JoystickInstanceID(joy);

		mJoysticks.erase(joyId);
		mJoysticks[joyId] = joy;

		char guid[40];
		SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(joy), guid, 40);

		// create the InputConfig
		auto cfg = mInputConfigs.find(joyId);
		if (cfg != mInputConfigs.cend())
		{
			if (cfg->second != nullptr)
				delete cfg->second;

			mInputConfigs.erase(joyId);
		}

		// if SDL_JoystickPathForIndex does not exist, store a value containing index + guid
		std::string devicePath = Utils::String::padLeft(std::to_string(idx), 4, '0') + "@" + std::string(guid);

#if SDL_VERSION_ATLEAST(2, 24, 0)
		devicePath = SDL_JoystickPathForIndex(idx);
		LOG(LogDebug) << "InputManager::rebuildAllJoysticks() - SDL 2.24.0 o superior";
#endif

		// create the InputConfig
		std::string addedDeviceName = SDL_JoystickName(joy);
		InputConfig *config = new InputConfig(joyId, idx, addedDeviceName, guid, SDL_JoystickNumButtons(joy), SDL_JoystickNumHats(joy), SDL_JoystickNumAxes(joy), devicePath);
		mInputConfigs[joyId] = config;
		std::string logMessage = "Added known joystick";
		if (!loadInputConfig(config))
		{
			std::string mappingString;

			if (SDL_IsGameController(idx))
				mappingString = SDL_GameControllerMappingForDeviceIndex(idx);

			if (!mappingString.empty() && loadFromSdlMapping(config, mappingString))
			{
				InputManager::getInstance()->writeDeviceConfig(config); // save
				logMessage = "Creating joystick from SDL Game Controller mapping";
			}
			else
				logMessage = "Added unconfigured joystick";
		}
		LOG(LogInfo) << "InputManager::rebuildAllJoysticks() - " << logMessage << " '" << addedDeviceName << "' (GUID: " << guid << ", instance ID: " << joyId << ", device index: " << idx << ", device path : " << devicePath << "), default input: " << Utils::String::boolToString(config->isDefaultInput()) << ".";
		Settings::getInstance()->setInt("last.device.added.id", joyId);

		// set up the prevAxisValues
		int numAxes = SDL_JoystickNumAxes(joy);
		
		mPrevAxisValues.erase(joyId);
		mPrevAxisValues[joyId] = new int[numAxes];
		std::fill(mPrevAxisValues[joyId], mPrevAxisValues[joyId] + numAxes, 0); //initialize array to 0
	}

	mJoysticksLock.unlock();

	computeLastKnownPlayersDeviceIndexes();

	joystickChanged.invoke([](IJoystickChangedEvent* c) { c->onJoystickChanged(); });

	SDL_JoystickEventState(SDL_ENABLE);
}

bool InputManager::parseEvent(const SDL_Event& ev, Window* window)
{
	bool causedEvent = false;
	switch (ev.type)
	{
	case SDL_JOYAXISMOTION:
	{		
	// some axes are "full" : from -32000 to +32000
	// in this case, their unpressed state is not 0
	// SDL provides a function to get this value
	// in es, the trick is to minus this value to the value to do as if it started at 0
		int initialValue = 0;
		Sint16 x;

#if SDL_VERSION_ATLEAST(2, 0, 9)
		LOG(LogDebug) << "InputManager::rebuildAllJoysticks() - SDL 2.0.9 o superior";
		// SDL_JoystickGetAxisInitialState doesn't work with 8bitdo start+b
		// required for several pads like xbox and 8bitdo

		auto inputConfig = mInputConfigs.find(ev.jaxis.which);
		if (inputConfig != mInputConfigs.cend())
		{
			std::string guid = std::to_string(ev.jaxis.axis) + "@" + inputConfig->second->getDeviceGUIDString();

			auto it = mJoysticksInitialValues.find(guid);
			if (it != mJoysticksInitialValues.cend())
				initialValue = it->second;
			else if (SDL_JoystickGetAxisInitialState(mJoysticks[ev.jaxis.which], ev.jaxis.axis, &x))
			{
				mJoysticksInitialValues[guid] = x;
				initialValue = x;
			}
		}
#endif

		if (mPrevAxisValues.find(ev.jaxis.which) != mPrevAxisValues.cend())
		{			
			//if it switched boundaries
			if ((abs(ev.jaxis.value - initialValue) > DEADZONE) != (abs(mPrevAxisValues[ev.jaxis.which][ev.jaxis.axis]) > DEADZONE))
			{
				int normValue;
				if (abs(ev.jaxis.value - initialValue) <= DEADZONE) 
					normValue = 0;
				else
					if (ev.jaxis.value - initialValue > 0) 
						normValue = 1;
					else
						normValue = -1;

				propagateInputEvent(getInputConfigByDevice(ev.jaxis.which), Input(ev.jaxis.which, TYPE_AXIS, ev.jaxis.axis, normValue, false), window);
				causedEvent = true;
			}

			mPrevAxisValues[ev.jaxis.which][ev.jaxis.axis] = ev.jaxis.value - initialValue; 
		}

		return causedEvent;
	}
	case SDL_JOYBUTTONDOWN:
	case SDL_JOYBUTTONUP:
		propagateInputEvent(getInputConfigByDevice(ev.jbutton.which), Input(ev.jbutton.which, TYPE_BUTTON, ev.jbutton.button, ev.jbutton.state == SDL_PRESSED, false), window);
		return true;

	case SDL_JOYHATMOTION:
		propagateInputEvent(getInputConfigByDevice(ev.jhat.which), Input(ev.jhat.which, TYPE_HAT, ev.jhat.hat, ev.jhat.value, false), window);
		return true;

	case SDL_KEYDOWN:
		if (ev.key.keysym.sym == SDLK_BACKSPACE && SDL_IsTextInputActive())
			window->textInput("\b");

		if (ev.key.repeat)
			return false;

		if (ev.key.keysym.sym == SDLK_F4)
		{
			SDL_Event* quit = new SDL_Event();
			quit->type = SDL_QUIT;
			SDL_PushEvent(quit);
			return false;
		}

		propagateInputEvent(getInputConfigByDevice(DEVICE_KEYBOARD), Input(DEVICE_KEYBOARD, TYPE_KEY, ev.key.keysym.sym, 1, false), window);
		return true;

	case SDL_KEYUP:
		propagateInputEvent(getInputConfigByDevice(DEVICE_KEYBOARD), Input(DEVICE_KEYBOARD, TYPE_KEY, ev.key.keysym.sym, 0, false), window);
		return true;

	case SDL_TEXTINPUT:
		window->textInput(ev.text.text);
		break;

	case SDL_JOYDEVICEADDED:
		{
			std::string addedDeviceName;
			auto id = SDL_JoystickGetDeviceInstanceID(ev.jdevice.which);
			auto it = std::find_if(mInputConfigs.cbegin(), mInputConfigs.cend(), [id](const std::pair<SDL_JoystickID, InputConfig*> & t) { return t.second != nullptr && t.second->getDeviceId() == id; });
			if (it == mInputConfigs.cend())
				addedDeviceName = SDL_JoystickNameForIndex(ev.jdevice.which);

			rebuildAllJoysticks();

			if (!addedDeviceName.empty()) // && !mInputConfigs[id]->isDefaultInput())
			{
				SDL_JoystickID new_id = Settings::getInstance()->getInt("last.device.added.id");
				InputConfig *iConfig = getInputConfigByDevice(new_id);
				std::string deviceGUIDString = "";
				std::string deviceID = "";
				std::string deviceIndex = "";
				std::string devicePath = "";
				if (iConfig && (iConfig->getDeviceName() == addedDeviceName))
				{
					if (iConfig->isDefaultInput())
						return true;

					deviceGUIDString = iConfig->getDeviceGUIDString();
					deviceID = std::to_string(iConfig->getDeviceId());
					deviceIndex = std::to_string(iConfig->getDeviceIndex());
					devicePath = iConfig->getDevicePath();
				}

				Scripting::fireEvent("input-controller-added", addedDeviceName, deviceGUIDString, deviceID, deviceIndex, devicePath, "false");

				if (Settings::getInstance()->getBool("bluetooth.use.alias"))
				{
					std::string alias = Settings::getInstance()->getString(addedDeviceName + ".bluetooth.input_gaming.alias");
					if (!alias.empty())
						addedDeviceName = alias;
				}
				window->displayNotificationMessage(_U("\uF11B ") + Utils::String::format(_("%s connected").c_str(), Utils::String::trim(addedDeviceName).c_str()));
			}
		}
		return true;

	case SDL_JOYDEVICEREMOVED:
		{
			auto it = mInputConfigs.find(ev.jdevice.which);
			InputConfig *iConfig = it->second;
			if (it != mInputConfigs.cend() && iConfig != nullptr && !iConfig->isDefaultInput())
			{
				std::string removedDeviceName = iConfig->getDeviceName();
				Scripting::fireEvent("input-controller-removed", removedDeviceName, iConfig->getDeviceGUIDString(), std::to_string(iConfig->getDeviceId()), std::to_string(iConfig->getDeviceIndex()), iConfig->getDevicePath(), Utils::String::boolToString(iConfig->isDefaultInput()));
				if (Settings::getInstance()->getBool("bluetooth.use.alias"))
				{
					std::string alias = Settings::getInstance()->getString(removedDeviceName + ".bluetooth.input_gaming.alias");
					if (!alias.empty())
						removedDeviceName = alias;
				}
				window->displayNotificationMessage(_U("\uF11B ") + Utils::String::format(_("%s disconnected").c_str(), Utils::String::trim(removedDeviceName).c_str()));
			}

			rebuildAllJoysticks();
		}
		return false;
	}

	if (mCECInputConfig && (ev.type == (unsigned int)SDL_USER_CECBUTTONDOWN || ev.type == (unsigned int)SDL_USER_CECBUTTONUP))
	{
		propagateInputEvent(getInputConfigByDevice(DEVICE_CEC), Input(DEVICE_CEC, TYPE_CEC_BUTTON, ev.user.code, ev.type == (unsigned int)SDL_USER_CECBUTTONDOWN, false), window);
		return true;
	}

	return false;
}

void InputManager::propagateInputEvent(InputConfig* config, Input input, Window* window)
{
	if (config->isMappedTo(SYSTEM_HOT_KEY_NAME_STRING, input))
	{
		if (input.value != 0)
			mIgnoreKeys = true;
		else
			mIgnoreKeys = false;
	}

	if (mIgnoreKeys)
		return;

	window->input(config, input);
}

bool InputManager::tryLoadInputConfig(std::string path, InputConfig* config, bool allowApproximate)
{
	pugi::xml_node configNode(NULL);

	if (!Utils::FileSystem::exists(path))
		return false;

	pugi::xml_document doc;
	pugi::xml_parse_result res = doc.load_file(path.c_str());

	if (!res)
	{
		LOG(LogError) << "InputManager::tryLoadInputConfig() - Error parsing input config: " << res.description();
		return false;
	}

	pugi::xml_node root = doc.child("inputList");
	if (!root)
		return false;
	
	// Search for exact match guid + name
	for (pugi::xml_node item = root.child("inputConfig"); item; item = item.next_sibling("inputConfig"))
	{
		if (strcmp(config->getDeviceGUIDString().c_str(), item.attribute("deviceGUID").value()) == 0 && strcmp(config->getDeviceName().c_str(), item.attribute("deviceName").value()) == 0)
		{
			configNode = item;
			break;
		}
	}

	// Search for guid + name match but test guid without the new crc16 bytes (SDL2 2.26)
	// Cf. https://github.com/libsdl-org/SDL/commit/c1e087394020a8cb9d2a04a1eabbcc23a6a5b20d
	if (!configNode)
	{
		for (pugi::xml_node item = root.child("inputConfig"); item; item = item.next_sibling("inputConfig"))
		{
			// Remove CRC-16 encoding from SDL2 2.26+ guids
			std::string guid = config->getDeviceGUIDString();
			for (int i = 4; i < 8 && i < guid.length(); i++)
				guid[i] = '0';

			if (guid != config->getDeviceGUIDString()
				&& (strcmp(guid.c_str(), item.attribute("deviceGUID").value()) == 0 && strcmp(config->getDeviceName().c_str(), item.attribute("deviceName").value()) == 0))
			{
				configNode = item;
				break;
			}
		}
	}

	// Search for exact match guid
	if (!configNode && allowApproximate)
	{
		for (pugi::xml_node item = root.child("inputConfig"); item; item = item.next_sibling("inputConfig"))
		{
			if (strcmp(config->getDeviceGUIDString().c_str(), item.attribute("deviceGUID").value()) == 0)
			{
				LOG(LogInfo) << "InputManager::tryLoadInputConfig() - Approximative device found using guid=" << configNode.attribute("deviceGUID").value() << " name=" << configNode.attribute("deviceName").value() << ")";
				configNode = item;
				break;
			}
		}
	}

	// Search for name match
	if (!configNode && allowApproximate)
	{
		for (pugi::xml_node item = root.child("inputConfig"); item; item = item.next_sibling("inputConfig"))
		{
			if (strcmp(config->getDeviceName().c_str(), item.attribute("deviceName").value()) == 0)
			{
				LOG(LogInfo) << "InputManager::tryLoadInputConfig() - Approximative device found using guid=" << configNode.attribute("deviceGUID").value() << " name=" << configNode.attribute("deviceName").value() << ")";
				configNode = item;
				break;
			}
		}
	}

	if (!configNode)
		return false;

	config->loadFromXML(configNode);
	return true;
}

static std::map<std::string, std::string> _sdlToEsMapping =
{
   { "a",             "b" },
   { "b",             "a" },
   { "x",             "y" },
   { "y",             "x" },
   { "back",          "select" },
   { "start",         "start" },
   { "leftshoulder",  "l1" },
   { "rightshoulder", "r1" },
   { "dpup",          "up" },
   { "dpright",       "right" },
   { "dpdown",        "down" },
   { "dpleft",        "left" },
   { "leftx",         "joystick1left" },
   { "lefty",         "joystick1up" },
   { "rightx",        "joystick2left" },
   { "righty",        "joystick2up" },
   { "lefttrigger",   "l2" },
   { "righttrigger",  "r2" },
   { "leftstick",     "l3" },
   { "rightstick",    "r3" }
   // { "guide",         "hotkey" } // Better take select for hotkey ?
};

bool InputManager::loadFromSdlMapping(InputConfig* config, const std::string& mapping)
{	
	bool isConfigured = false;

	auto mapArray = Utils::String::split(mapping, ',', true);
	for (auto tt : mapArray)
	{
		size_t pos = tt.find(':');
		if (pos == std::string::npos) 
			continue;

		std::string key = tt.substr(0, pos);
		std::string value = tt.substr(pos + 1);

		auto inputName = _sdlToEsMapping.find(key);
		if (inputName == _sdlToEsMapping.cend())
		{
			LOG(LogError) << "InputManager::loadFromSdlMapping() - [InputDevice] Unknown mapping: " << key;
			continue;
		}

		int valueSign = 1;
		int idx = 0;
				
		switch (value[idx])
		{
		case '-': 
			valueSign = -1;
			idx++;
			break;
		case '+': 
			idx++;
			break;
		}

		Input input;
		input.device = config->getDeviceId();
		input.type = InputType::TYPE_COUNT;
		input.configured = false;		

		switch (value[idx])
		{
		case 'a':
			{
				input.type = InputType::TYPE_AXIS;
				input.id = Utils::String::toInteger(value.substr(idx + 1));				
				input.value = 1 * valueSign;
				input.configured = true;

				// Invert directions for these mappings
				if (inputName->second == "joystick1up" || inputName->second == "joystick1left" || inputName->second == "joystick2up" || inputName->second == "joystick2left")
					input.value *= -1;
			}
			break;
		case 'b':
			{
				input.type = InputType::TYPE_BUTTON;
				input.id = Utils::String::toInteger(value.substr(idx + 1));
				input.value = 1;
				input.configured = true;
			}
			break;
		case 'h':
			{
				auto hatIds = Utils::String::split(value.substr(idx + 1), '.', true);
				if (hatIds.size() > 1)
				{
					input.type = InputType::TYPE_HAT;
					input.id = Utils::String::toInteger(hatIds[0]);
					input.value = Utils::String::toInteger(hatIds[1]);
					input.configured = true;
				}
			}
			break;
		}

		if (input.configured)
		{
			// BATOCERA : Uncomment the next line using a patch to compute the code
			// input.computeCode();
			
			config->mapInput(inputName->second, input);
			isConfigured = true;
		}
	}

	if (isConfigured)
	{
		// Use select button if hotkey is undefined
		Input tmp;
		if (!config->getInputByName("hotkey", &tmp))
			if (config->getInputByName("select", &tmp))
				config->mapInput("hotkey", tmp);
	}

	return isConfigured;
}

bool InputManager::loadInputConfig(InputConfig* config)
{
	std::string path = getConfigPath();

	// Find exact device
	if (tryLoadInputConfig(path, config, false))
		return true;

	// Find system exact device
	std::string sharedPath = Utils::FileSystem::getSharedConfigPath() + "/es_input.cfg";
	if (tryLoadInputConfig(sharedPath, config, false))
		return true;

	// Find user Approximative device
	if (tryLoadInputConfig(path, config, true))
		return true;

	// Find system Approximative device
	if (tryLoadInputConfig(sharedPath, config, true))
		return true;

	return false;
}

//used in an "emergency" where no keyboard config could be loaded from the inputmanager config file
//allows the user to select to reconfigure in menus if this happens without having to delete es_input.cfg manually
void InputManager::loadDefaultKBConfig()
{
	InputConfig* cfg = getInputConfigByDevice(DEVICE_KEYBOARD);

	cfg->clear();
	cfg->mapInput("up", Input(DEVICE_KEYBOARD, TYPE_KEY, SDLK_UP, 1, true));
	cfg->mapInput("down", Input(DEVICE_KEYBOARD, TYPE_KEY, SDLK_DOWN, 1, true));
	cfg->mapInput("left", Input(DEVICE_KEYBOARD, TYPE_KEY, SDLK_LEFT, 1, true));
	cfg->mapInput("right", Input(DEVICE_KEYBOARD, TYPE_KEY, SDLK_RIGHT, 1, true));

	cfg->mapInput(BUTTON_OK, Input(DEVICE_KEYBOARD, TYPE_KEY, SDLK_RETURN, 1, true));
	cfg->mapInput(BUTTON_BACK, Input(DEVICE_KEYBOARD, TYPE_KEY, SDLK_ESCAPE, 1, true));
	cfg->mapInput("start", Input(DEVICE_KEYBOARD, TYPE_KEY, SDLK_F1, 1, true));
	cfg->mapInput("select", Input(DEVICE_KEYBOARD, TYPE_KEY, SDLK_F2, 1, true));

	cfg->mapInput(BUTTON_PU, Input(DEVICE_KEYBOARD, TYPE_KEY, SDLK_RIGHTBRACKET, 1, true));
	cfg->mapInput(BUTTON_PD, Input(DEVICE_KEYBOARD, TYPE_KEY, SDLK_LEFTBRACKET, 1, true));
}

void InputManager::writeDeviceConfig(InputConfig* config)
{
	assert(initialized());

	std::string path = getConfigPath();

	LOG(LogInfo) << "InputManager::writeDeviceConfig() - Saving input config file '" << path << "'...";

	pugi::xml_document doc;

	if (Utils::FileSystem::exists(path))
	{
		// merge files
		pugi::xml_parse_result result = doc.load_file(path.c_str());
		if (!result)
		{
			LOG(LogError) << "InputManager::writeDeviceConfig() - Error parsing input config: " << result.description();
		}
		else
		{
			// successfully loaded, delete the old entry if it exists
			pugi::xml_node root = doc.child("inputList");
			if (root)
			{				
				pugi::xml_node oldEntry(NULL);
				for (pugi::xml_node item = root.child("inputConfig"); item; item = item.next_sibling("inputConfig")) 
				{
					if (strcmp(config->getDeviceGUIDString().c_str(), item.attribute("deviceGUID").value()) == 0
						&& strcmp(config->getDeviceName().c_str(), item.attribute("deviceName").value()) == 0
						) 
					{
						oldEntry = item;
						break;
					}
				}
				if (oldEntry)
					root.remove_child(oldEntry);
			}
		}
	}

	pugi::xml_node root = doc.child("inputList");
	if(!root)
		root = doc.append_child("inputList");

	config->writeToXML(root);
	doc.save_file(path.c_str());
        
	/* create a es_last_input.cfg so that people can easily share their config */
	pugi::xml_document lastdoc;
	pugi::xml_node lastroot = lastdoc.append_child("inputList");
	config->writeToXML(lastroot);
	std::string lastpath = getTemporaryConfigPath();
	lastdoc.save_file(lastpath.c_str());

	Scripting::fireEvent("config-changed");
	Scripting::fireEvent("controls-changed", std::to_string(config->getDeviceId()), config->getDeviceName(), config->getDeviceGUIDString(), Utils::String::boolToString(config->isDefaultInput()));
	
	// execute any onFinish commands and re-load the config for changes
	doOnFinish();
	loadInputConfig(config);
}

void InputManager::doOnFinish()
{
	if (!initialized())
		return;

	std::string path = getConfigPath();
	pugi::xml_document doc;

	LOG(LogInfo) << "InputManager::doOnFinish() - Do on finish of config file '" << path << "'...";

	if(Utils::FileSystem::exists(path))
	{
		pugi::xml_parse_result result = doc.load_file(path.c_str());
		if(!result)
		{
			LOG(LogError) << "InputManager::doOnFinish() - Error parsing input config: " << result.description();
		}
		else
		{
			pugi::xml_node root = doc.child("inputList");
			if(root)
			{
				root = root.find_child_by_attribute("inputAction", "type", "onfinish");
				if(root)
				{
					for(pugi::xml_node command = root.child("command"); command;
							command = command.next_sibling("command"))
					{
						std::string tocall = command.text().get();

						LOG(LogInfo) << "InputManager::doOnFinish() - 	" << tocall;
						std::cout << "==============================================\ninput config finish command:\n";
						int exitCode = runSystemCommand(tocall, "", nullptr);
						std::cout << "==============================================\n";

						if(exitCode != 0)
						{
							LOG(LogWarning) << "...launch terminated with nonzero exit code " << exitCode << "!";
						}
					}
				}
			}
		}
	}
}

std::string InputManager::getConfigPath()
{
	std::string path = Utils::FileSystem::getEsConfigPath() + "/es_input.cfg";

	if(Utils::FileSystem::exists(path))
		return path;

	return "/etc/emulationstation/es_input.cfg";
}

std::string InputManager::getTemporaryConfigPath()
{
	return Utils::FileSystem::getEsConfigPath() + "/es_temporaryinput.cfg";
}

bool InputManager::initialized() const
{
	return mKeyboardInputConfig != NULL;
}

int InputManager::getNumConfiguredDevices()
{
	int num = 0;

	for(auto it = mInputConfigs.cbegin(); it != mInputConfigs.cend(); it++)
		if(it->second && it->second->isConfigured())
			num++;

	if (mKeyboardInputConfig && mKeyboardInputConfig->isConfigured())
		num++;

	if (mCECInputConfig && mCECInputConfig->isConfigured())
		num++;

	return num;
}

void InputManager::computeLastKnownPlayersDeviceIndexes() 
{
	std::map<int, InputConfig*> playerJoysticks = computePlayersConfigs();

	m_lastKnownPlayersDeviceIndexes.clear();
	for (int player = 0; player < MAX_PLAYERS; player++)
	{
		auto playerJoystick = playerJoysticks[player];
		if (playerJoystick != nullptr)
		{
			PlayerDeviceInfo dev;
			dev.index = playerJoystick->getDeviceIndex();
			dev.batteryLevel = playerJoystick->getBatteryLevel();

			m_lastKnownPlayersDeviceIndexes[player] = dev;
		}
	}
}

std::map<int, InputConfig*> InputManager::computePlayersConfigs()
{
	std::unique_lock<std::mutex> lock(mJoysticksLock);

	// 1. Retrieve the configured
	std::vector<InputConfig *> availableConfigured;
	for (auto conf : mInputConfigs)
		if (conf.second != nullptr && conf.second->isConfigured())
			availableConfigured.push_back(conf.second);

	// sort available configs
	std::sort(availableConfigured.begin(), availableConfigured.end(), [](InputConfig * a, InputConfig * b) -> bool { return a->getDeviceIndex() < b->getDeviceIndex(); });

	// 2. For each player check if there is a configured
	// associate the input to the player
	// remove from available
	std::map<int, InputConfig*> playerJoysticks;

	// First loop, search for PATH. Ultra High Priority
	for (int player = 0; player < MAX_PLAYERS; player++)
	{
		std::string playerConfigPath = Settings::getInstance()->getString(Utils::String::format("INPUT P%iPATH", player + 1));
		if (!playerConfigPath.empty())
		{
			for (auto it1 = availableConfigured.begin(); it1 != availableConfigured.end(); ++it1)
			{
				InputConfig* config = *it1;
				if (playerConfigPath == config->getSortDevicePath())
				{
					availableConfigured.erase(it1);
					playerJoysticks[player] = config;
					break;
				}
			}
		}
	}

	// First loop, search for GUID + NAME. High Priority
	for (int player = 0; player < MAX_PLAYERS; player++) 
	{
		std::string playerConfigName = Settings::getInstance()->getString(Utils::String::format("INPUT P%iNAME", player + 1));
		std::string playerConfigGuid = Settings::getInstance()->getString(Utils::String::format("INPUT P%iGUID", player + 1));

		for (auto it1 = availableConfigured.begin(); it1 != availableConfigured.end(); ++it1)
		{
			InputConfig* config = *it1;
			if (playerConfigName == config->getDeviceName() && playerConfigGuid == config->getDeviceGUIDString())
			{
				availableConfigured.erase(it1);
				playerJoysticks[player] = config;
				break;
			}
		}
	}

	// Second loop, search for NAME. Low Priority
	for (int player = 0; player < MAX_PLAYERS; player++) 
	{
		if (playerJoysticks[player] != nullptr)
			continue;

		std::string playerConfigName = Settings::getInstance()->getString(Utils::String::format("INPUT P%dNAME", player + 1));

		for (auto it1 = availableConfigured.begin(); it1 != availableConfigured.end(); ++it1)
		{
			InputConfig * config = *it1;
			if (playerConfigName == config->getDeviceName())
			{
				availableConfigured.erase(it1);
				playerJoysticks[player] = config;
				break;
			}
		}
	}

	// Last loop, search for free controllers for remaining players.
	for (int player = 0; player < MAX_PLAYERS; player++) 
	{
		if (playerJoysticks[player] != nullptr)
			continue;

		// if no config has been found for the player, we try to give him a free one
		for (auto it1 = availableConfigured.begin(); it1 != availableConfigured.end(); ++it1)
		{
			playerJoysticks[player] = *it1;
			availableConfigured.erase(it1);
			break;
		}
	}

	// in case of hole (player 1 missing, but player 4 set, fill the holes with last players joysticks)
	for (int player = 0; player < MAX_PLAYERS; player++) 
	{
		if (playerJoysticks[player] != nullptr)
			continue;

		for (int repplayer = MAX_PLAYERS; repplayer > player; repplayer--) 
		{
			if (playerJoysticks[player] == NULL && playerJoysticks[repplayer] != NULL) 
			{
				playerJoysticks[player] = playerJoysticks[repplayer];
				playerJoysticks[repplayer] = NULL;
			}
		}		
	}

	for (int player = 0; player < MAX_PLAYERS; player++)
	{
		if (playerJoysticks[player] == nullptr)
			continue;

		LOG(LogInfo) << "InputManager::computePlayersConfigs() - Player " << player << " => " << playerJoysticks[player]->getDevicePath();
	}

	return playerJoysticks;
}

std::string InputManager::configureEmulators() {
	std::map<int, InputConfig*> playerJoysticks = computePlayersConfigs();
	std::stringstream command;

	static std::string input_controllers = "/tmp/input_controllers.cfg";

	if (Utils::FileSystem::exists(input_controllers))
		Utils::FileSystem::removeFile(input_controllers);

	command << "[\n";
	bool isPlayerWrited = false;
	for (int player = 0; player < MAX_PLAYERS; player++)
	{
		InputConfig * playerInputConfig = playerJoysticks[player];
		if (playerInputConfig != NULL)
		{
			if (isPlayerWrited)
			{
				command << ",\n";
				isPlayerWrited = false;
			}

			command << "{\n";
			command << "    \"name\" : \"" << playerInputConfig->getDeviceName() << "\",\n";
			command << "    \"player\" : " << player + 1 << ",\n";
			command << "    \"index\" : " << playerInputConfig->getDeviceIndex() << ",\n";
			command << "    \"guid\" : \"" << playerInputConfig->getDeviceGUIDString() << "\",\n";
			command << "    \"path\" : \"" << playerInputConfig->getDevicePath() << "\",\n";
			command << "    \"nbbuttons\" : " << playerInputConfig->getDeviceNbButtons() << ",\n";
			command << "    \"nbhats\" : " << playerInputConfig->getDeviceNbHats() << ",\n";
			command << "    \"nbaxes\" : " << playerInputConfig->getDeviceNbAxes() << ",\n";
			command << "    \"ndefault\" : " << playerInputConfig->isDefaultInput() << "\n";
			command << "}";
			isPlayerWrited = true;
/*
			command <<  "-p" << player+1 << "index "      << playerInputConfig->getDeviceIndex();
			command << " -p" << player+1 << "guid "       << playerInputConfig->getDeviceGUIDString();
			// Windows
			command << " -p" << player+1 << "path \""     << playerInputConfig->getDevicePath() << "\"";
			command << " -p" << player+1 << "name \""     << playerInputConfig->getDeviceName() << "\"";
			command << " -p" << player+1 << "nbbuttons "  << playerInputConfig->getDeviceNbButtons();
			command << " -p" << player+1 << "nbhats "     << playerInputConfig->getDeviceNbHats();
			command << " -p" << player+1 << "nbaxes "     << playerInputConfig->getDeviceNbAxes();
			command << " ";
*/
		}
	}
	command << "\n]\n";
	LOG(LogInfo) << "InputManager::configureEmulators() - Configure emulators command : " << command.str().c_str();
//	return command.str();
	std::ofstream outfile(input_controllers);
	outfile << command.str().c_str();
    outfile.close();
	return "";
}
/*
void InputManager::updateBatteryLevel(int id, const std::string& device, const std::string& devicePath, int level)
{
	bool changed = false;

	mJoysticksLock.lock();

	for (auto joy : mJoysticks) 
	{
		InputConfig* config = getInputConfigByDevice(joy.first);
		if (config != NULL && config->isConfigured())
		{
			if (!devicePath.empty())
			{
				if (Utils::String::compareIgnoreCase(config->getDevicePath(), devicePath) == 0)
				{
					config->updateBatteryLevel(level);
					changed = true;
				}
			}
			else
			{
				if (Utils::String::compareIgnoreCase(config->getDeviceGUIDString(), device) == 0)
				{
					config->updateBatteryLevel(level);
					changed = true;
				}
			}
		}
	}

	mJoysticksLock.unlock();

	if (changed)
		computeLastKnownPlayersDeviceIndexes();
}
*/
std::vector<InputConfig*> InputManager::getInputConfigs()
{
	std::vector<InputConfig*> ret;

	std::map<int, InputConfig*> playerJoysticks = computePlayersConfigs();

	for (int player = 0; player < MAX_PLAYERS; player++)
	{
		auto playerJoystick = playerJoysticks[player];
		if (playerJoystick != nullptr && playerJoystick->isConfigured())
			ret.push_back(playerJoystick);
	}

	return ret;
}
