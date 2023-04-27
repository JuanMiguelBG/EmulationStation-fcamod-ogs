#include "InputManager.h"

#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"
#include "CECInput.h"
#include "Log.h"
#include "platform.h"
#include "Scripting.h"
#include "Window.h"
#include <pugixml/src/pugixml.hpp>
#include <SDL.h>
#include <iostream>
#include <assert.h>
#include "Scripting.h"
#include "Window.h"

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

InputManager* InputManager::mInstance = NULL;

InputManager::InputManager() : mKeyboardInputConfig(NULL), mIgnoreKeys(false)
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
	if(initialized())
		deinit();

	SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS,
		Settings::getInstance()->getBool("BackgroundJoystickInput") ? "1" : "0");
	SDL_InitSubSystem(SDL_INIT_JOYSTICK);
	SDL_JoystickEventState(SDL_ENABLE);

	// first, open all currently present joysticks
	int numJoysticks = SDL_NumJoysticks();
	for(int i = 0; i < numJoysticks; i++)
	{
		addJoystickByDeviceIndex(i);
	}

	mKeyboardInputConfig = new InputConfig(DEVICE_KEYBOARD, "Keyboard", KEYBOARD_GUID_STRING);
	loadInputConfig(mKeyboardInputConfig);

	SDL_USER_CECBUTTONDOWN = SDL_RegisterEvents(2);
	SDL_USER_CECBUTTONUP   = SDL_USER_CECBUTTONDOWN + 1;
	CECInput::init();
	mCECInputConfig = new InputConfig(DEVICE_CEC, "CEC", CEC_GUID_STRING);
	loadInputConfig(mCECInputConfig);
}

void InputManager::addJoystickByDeviceIndex(int id, Window* window)
{
	assert(id > -1);
	assert(id < SDL_NumJoysticks());

	// open joystick & add to our list
	SDL_Joystick* joy = SDL_JoystickOpen(id);
	assert(joy);

	// add it to our list so we can close it again later
	SDL_JoystickID joyId = SDL_JoystickInstanceID(joy);
	mJoysticks[joyId] = joy;

	char guid[65];
	SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(joy), guid, 65);

	// create the InputConfig
	std::string addedDeviceName = SDL_JoystickName(joy);
	mInputConfigs[joyId] = new InputConfig(joyId, addedDeviceName, guid);
	std::string is_defalut_config = Utils::String::boolToString(mInputConfigs[joyId]->isDefaultInput());
	if (!loadInputConfig(mInputConfigs[joyId]))
	{
		LOG(LogInfo) << "InputManager::addJoystickByDeviceIndex() - Added unconfigured joystick '" << addedDeviceName << "' (GUID: " << guid << ", instance ID: " << joyId << ", device index: " << id << ", default input: " << is_defalut_config << ").";
		Scripting::fireEvent("input-controller-added", addedDeviceName, guid, std::to_string(id), is_defalut_config);
	}
	else
	{
		LOG(LogInfo) << "InputManager::addJoystickByDeviceIndex() - Added known joystick '" << addedDeviceName << "' (instance ID: " << joyId << ", device index: " << id << ", default input: " << is_defalut_config << ").";
		Scripting::fireEvent("input-controller-added", addedDeviceName, guid, std::to_string(id), is_defalut_config);
	}

	if (!addedDeviceName.empty() && window && !mInputConfigs[joyId]->isDefaultInput())
	{
		if (Settings::getInstance()->getBool("bluetooth.use.alias"))
		{
			std::string alias = Settings::getInstance()->getString(addedDeviceName + ".bluetooth.input_gaming.alias");
			if (!alias.empty())
				addedDeviceName = alias;
		}
		window->displayNotificationMessage(_U("\uF11B ") + Utils::String::format(_("%s connected").c_str(), Utils::String::trim(addedDeviceName).c_str()));
	}

	// set up the prevAxisValues
	int numAxes = SDL_JoystickNumAxes(joy);
	mPrevAxisValues[joyId] = new int[numAxes];
	std::fill(mPrevAxisValues[joyId], mPrevAxisValues[joyId] + numAxes, 0); //initialize array to 0
}

void InputManager::removeJoystickByJoystickID(SDL_JoystickID joyId, Window* window)
{
	assert(joyId != -1);

	// delete old prevAxisValues
	auto axisIt = mPrevAxisValues.find(joyId);
	delete[] axisIt->second;
	mPrevAxisValues.erase(axisIt);

	// delete old InputConfig
	auto it = mInputConfigs.find(joyId);
	bool isDefaultInput = it->second->isDefaultInput();
	delete it->second;
	mInputConfigs.erase(it);

	// close the joystick
	auto joyIt = mJoysticks.find(joyId);
	if (joyIt != mJoysticks.cend())
	{
		std::string removedDeviceName = SDL_JoystickName(joyIt->second);
		LOG(LogInfo) << "InputManager::removeJoystickByJoystickID() - Removed joystick '" << removedDeviceName << "' (instance ID: " << joyId << ")";
		char guid[65];
		SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(joyIt->second), guid, 65);
		Scripting::fireEvent("input-controller-removed", removedDeviceName, guid);

		SDL_JoystickClose(joyIt->second);
		mJoysticks.erase(joyIt);

		if (!removedDeviceName.empty() && window && !isDefaultInput)
		{
			if (Settings::getInstance()->getBool("bluetooth.use.alias"))
			{
				std::string alias = Settings::getInstance()->getString(removedDeviceName + ".bluetooth.input_gaming.alias");
				if (!alias.empty())
					removedDeviceName = alias;
			}
			window->displayNotificationMessage(_U("\uF11B ") + Utils::String::format(_("%s disconnected").c_str(), Utils::String::trim(removedDeviceName).c_str()));
		}
	}
	else
	{
		LOG(LogError) << "InputManager::removeJoystickByJoystickID() - Could not find joystick to close (instance ID: " << joyId << ")";
	}
}

void InputManager::deinit()
{
	if(!initialized())
		return;

	for(auto iter = mJoysticks.cbegin(); iter != mJoysticks.cend(); iter++)
	{
		SDL_JoystickClose(iter->second);
	}
	mJoysticks.clear();

	for(auto iter = mInputConfigs.cbegin(); iter != mInputConfigs.cend(); iter++)
	{
		delete iter->second;
	}
	mInputConfigs.clear();

	for(auto iter = mPrevAxisValues.cbegin(); iter != mPrevAxisValues.cend(); iter++)
	{
		delete[] iter->second;
	}
	mPrevAxisValues.clear();

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

	CECInput::deinit();

	SDL_JoystickEventState(SDL_DISABLE);
	SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
}

int InputManager::getNumJoysticks() { return (int)mJoysticks.size(); }

int InputManager::getAxisCountByDevice(SDL_JoystickID id)
{
	return SDL_JoystickNumAxes(mJoysticks[id]);
}

int InputManager::getButtonCountByDevice(SDL_JoystickID id)
{
	if(id == DEVICE_KEYBOARD)
		return 120; //it's a lot, okay.
	else if(id == DEVICE_CEC)
#ifdef HAVE_CECLIB
		return CEC::CEC_USER_CONTROL_CODE_MAX;
#else // HAVE_LIBCEF
		return 0;
#endif // HAVE_CECLIB
	else
		return SDL_JoystickNumButtons(mJoysticks[id]);
}

InputConfig* InputManager::getInputConfigByDevice(int device)
{
	if(device == DEVICE_KEYBOARD)
		return mKeyboardInputConfig;
	else if(device == DEVICE_CEC)
		return mCECInputConfig;
	else
		return mInputConfigs[device];
}

bool InputManager::parseEvent(const SDL_Event& ev, Window* window)
{
	bool causedEvent = false;
	switch(ev.type)
	{
	case SDL_JOYAXISMOTION:
		//if it switched boundaries
		if((abs(ev.jaxis.value) > DEADZONE) != (abs(mPrevAxisValues[ev.jaxis.which][ev.jaxis.axis]) > DEADZONE))
		{
			int normValue;
			if(abs(ev.jaxis.value) <= DEADZONE)
				normValue = 0;
			else
				if(ev.jaxis.value > 0)
					normValue = 1;
				else
					normValue = -1;

			propagateInputEvent(getInputConfigByDevice(ev.jaxis.which), Input(ev.jaxis.which, TYPE_AXIS, ev.jaxis.axis, normValue, false), window);
			causedEvent = true;
		}

		mPrevAxisValues[ev.jaxis.which][ev.jaxis.axis] = ev.jaxis.value;
		return causedEvent;

	case SDL_JOYBUTTONDOWN:
	case SDL_JOYBUTTONUP:
		propagateInputEvent(getInputConfigByDevice(ev.jbutton.which), Input(ev.jbutton.which, TYPE_BUTTON, ev.jbutton.button, ev.jbutton.state == SDL_PRESSED, false), window);
		return true;

	case SDL_JOYHATMOTION:
		propagateInputEvent(getInputConfigByDevice(ev.jhat.which), Input(ev.jhat.which, TYPE_HAT, ev.jhat.hat, ev.jhat.value, false), window);
		return true;

	case SDL_KEYDOWN:
		if(ev.key.keysym.sym == SDLK_BACKSPACE && SDL_IsTextInputActive())
		{
			window->textInput("\b");
		}

		if(ev.key.repeat)
			return false;

		if(ev.key.keysym.sym == SDLK_F4)
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
		addJoystickByDeviceIndex(ev.jdevice.which, window); // ev.jdevice.which is a device index
		return true;

	case SDL_JOYDEVICEREMOVED:
		removeJoystickByJoystickID(ev.jdevice.which, window); // ev.jdevice.which is an SDL_JoystickID (instance ID)
		return false;
	}

	if((ev.type == (unsigned int)SDL_USER_CECBUTTONDOWN) || (ev.type == (unsigned int)SDL_USER_CECBUTTONUP))
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

bool InputManager::loadInputConfig(InputConfig* config)
{
	std::string path = getConfigPath();

	LOG(LogInfo) << "InputManager::loadInputConfig() - Loading input config file '" << path << "'...";

	if(!Utils::FileSystem::exists(path))
		return false;

	pugi::xml_document doc;
	pugi::xml_parse_result res = doc.load_file(path.c_str());

	if(!res)
	{
		LOG(LogError) << "InputManager::loadInputConfig() - Error parsing input config: " << res.description();
		return false;
	}

	pugi::xml_node root = doc.child("inputList");
	if(!root)
		return false;

	pugi::xml_node configNode = root.find_child_by_attribute("inputConfig", "deviceGUID", config->getDeviceGUIDString().c_str());
	if(!configNode)
		configNode = root.find_child_by_attribute("inputConfig", "deviceName", config->getDeviceName().c_str());
	if(!configNode)
		return false;

	pugi::xml_attribute defaultDevideAtt = configNode.attribute("deviceDefault");
	bool defaultDevice = false;
	if (defaultDevideAtt)
	{
		LOG(LogInfo) << "InputManager::loadInputConfig() - devide default input config";
		defaultDevice = defaultDevideAtt.as_bool();
	}
	
	config->setDefaultInput(defaultDevice);
	config->loadFromXML(configNode);
	return true;
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

	cfg->mapInput("pageup", Input(DEVICE_KEYBOARD, TYPE_KEY, SDLK_RIGHTBRACKET, 1, true));
	cfg->mapInput("pagedown", Input(DEVICE_KEYBOARD, TYPE_KEY, SDLK_LEFTBRACKET, 1, true));
}

void InputManager::writeDeviceConfig(InputConfig* config)
{
	assert(initialized());

	std::string path = getConfigPath();

	LOG(LogInfo) << "InputManager::writeDeviceConfig() - Saving input config file '" << path << "'...";

	pugi::xml_document doc;

	if(Utils::FileSystem::exists(path))
	{
		// merge files
		pugi::xml_parse_result result = doc.load_file(path.c_str());
		if(!result)
		{
			LOG(LogError) << "InputManager::writeDeviceConfig() - Error parsing input config: " << result.description();
		}
		else
		{
			// successfully loaded, delete the old entry if it exists
			pugi::xml_node root = doc.child("inputList");
			if(root)
			{
				// if inputAction @type=onfinish is set, let onfinish command take care for creating input configuration.
				// we just put the input configuration into a temporary input config file.
				pugi::xml_node actionnode = root.find_child_by_attribute("inputAction", "type", "onfinish");
				if(actionnode)
				{
					path = getTemporaryConfigPath();
					doc.reset();
					root = doc.append_child("inputList");
					root.append_copy(actionnode);
				}
				else
				{
					pugi::xml_node oldEntry = root.find_child_by_attribute("inputConfig", "deviceGUID",
											  config->getDeviceGUIDString().c_str());
					if(oldEntry)
					{
						root.remove_child(oldEntry);
					}
					oldEntry = root.find_child_by_attribute("inputConfig", "deviceName",
															config->getDeviceName().c_str());
					if(oldEntry)
					{
						root.remove_child(oldEntry);
					}
				}
			}
		}
	}

	pugi::xml_node root = doc.child("inputList");
	if(!root)
		root = doc.append_child("inputList");

	config->writeToXML(root);
	doc.save_file(path.c_str());

	Scripting::fireEvent("config-changed");
	Scripting::fireEvent("controls-changed", std::to_string(config->getDeviceId()), config->getDeviceName(), config->getDeviceGUIDString());

	// execute any onFinish commands and re-load the config for changes
	doOnFinish();
	loadInputConfig(config);
}

void InputManager::doOnFinish()
{
	assert(initialized());
	std::string path = getConfigPath();

	LOG(LogInfo) << "InputManager::doOnFinish() - Do on finish of config file '" << path << "'...";

	pugi::xml_document doc;

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

						LOG(LogInfo) << "	" << tocall;
						std::cout << "==============================================\ninput config finish command:\n";
						int exitCode = runSystemCommand(tocall, "", NULL);
						std::cout << "==============================================\n";

						if(exitCode != 0)
						{
							LOG(LogWarning) << "InputManager::doOnFinish() - ...launch terminated with nonzero exit code " << exitCode << "!";
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
	{
		if(it->second->isConfigured())
			num++;
	}

	if(mKeyboardInputConfig->isConfigured())
		num++;

	if(mCECInputConfig->isConfigured())
		num++;

	return num;
}

std::string InputManager::getDeviceGUIDString(int deviceId)
{
	if(deviceId == DEVICE_KEYBOARD)
		return KEYBOARD_GUID_STRING;

	if(deviceId == DEVICE_CEC)
		return CEC_GUID_STRING;

	auto it = mJoysticks.find(deviceId);
	if(it == mJoysticks.cend())
	{
		LOG(LogError) << "InputManager::getDeviceGUIDString() - deviceId " << deviceId << " not found!";
		return "something went horribly wrong";
	}

	char guid[65];
	SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(it->second), guid, 65);
	return std::string(guid);
}
