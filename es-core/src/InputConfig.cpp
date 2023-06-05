#include "InputConfig.h"

#include "Log.h"
#include "Settings.h"
#include "utils/StringUtil.h"
#include <pugixml/src/pugixml.hpp>

static char ABUTTON[2] = "a";
static char BBUTTON[2] = "b";
static char L1BUTTON[4] = "l1";
static char L2BUTTON[4] = "l2";
static char R1BUTTON[4] = "r1";
static char R2BUTTON[4] = "r2";
static char L3BUTTON[4] = "l3";
static char R3BUTTON[4] = "r3";

//some util functions
std::string inputTypeToString(InputType type)
{
	switch(type)
	{
	case TYPE_AXIS:
		return "axis";
	case TYPE_BUTTON:
		return "button";
	case TYPE_HAT:
		return "hat";
	case TYPE_KEY:
		return "key";
	case TYPE_CEC_BUTTON:
		return "cec-button";
	default:
		return "error";
	}
}

InputType stringToInputType(const std::string& type)
{
	if(type == "axis")
		return TYPE_AXIS;
	if(type == "button")
		return TYPE_BUTTON;
	if(type == "hat")
		return TYPE_HAT;
	if(type == "key")
		return TYPE_KEY;
	if(type == "cec-button")
		return TYPE_CEC_BUTTON;
	return TYPE_COUNT;
}


std::string toLower(std::string str)
{
	for(unsigned int i = 0; i < str.length(); i++)
	{
		str[i] = (char)tolower(str[i]);
	}

	return str;
}
//end util functions

InputConfig::InputConfig(int deviceId, const std::string& deviceName, const std::string& deviceGUID)
	: mDeviceId(deviceId), mDeviceName(deviceName), mDeviceGUID(deviceGUID)
{
	mDefaultInput = false;
}

void InputConfig::clear()
{
	mNameMap.clear();
}

bool InputConfig::isConfigured()
{
	return mNameMap.size() > 0;
}

void InputConfig::mapInput(const std::string& name, Input input)
{
	mNameMap[toLower(name)] = input;
}

void InputConfig::unmapInput(const std::string& name)
{
	auto it = mNameMap.find(toLower(name));
	if(it != mNameMap.cend())
		mNameMap.erase(it);
}

bool InputConfig::getInputByName(const std::string& name, Input* result)
{
	auto it = mNameMap.find(toLower(name));
	if(it != mNameMap.cend())
	{
		*result = it->second;
		return true;
	}

	return false;
}

bool InputConfig::isMappedTo(const std::string& name, Input input, bool reversedAxis)
{
//	LOG(LogDebug) << "InputConfig::InputConfig::isMappedTo() - name: '" << name << "', ID: " << input.id <<
//				", value: " << std::to_string(input.value) << ", reversedAxis: " << Utils::String::boolToString(reversedAxis);
	Input comp;
	if (!getInputByName(name, &comp))
		return false;

	if (reversedAxis)
		comp.value *= -1;

	if (comp.configured && comp.type == input.type && comp.id == input.id)
	{
		if (comp.type == TYPE_HAT)
			return (input.value == 0 || input.value & comp.value);

		if (comp.type == TYPE_AXIS)
			return input.value == 0 || comp.value == input.value;

		return true;
	}

	return false;
}

bool InputConfig::isMappedLike(const std::string& name, Input input)
{
//	LOG(LogDebug) << "InputConfig::InputConfig::isMappedLike() - name: '" << name << "', ID: " << input.id << ", value: " << std::to_string(input.value);
	if (name == "left")
		return isMappedTo("left", input)
				|| isMappedTo("leftanalogleft", input) || isMappedTo("rightanalogleft", input)
				|| isMappedTo("joystick1left", input) || isMappedTo("joystick2left", input);

	if (name == "right")
		return isMappedTo("right", input)
				|| isMappedTo("leftanalogright", input) || isMappedTo("rightanalogright", input)
				|| isMappedTo("joystick1left", input, true) || isMappedTo("joystick2left", input, true);

	if (name == "up")
		return isMappedTo("up", input)
				|| isMappedTo("leftanalogup", input) || isMappedTo("rightanalogup", input)
				|| isMappedTo("joystick1up", input) || isMappedTo("joystick2up", input);

	if (name == "down")
		return isMappedTo("down", input)
				|| isMappedTo("leftanalogdown", input) || isMappedTo("rightanalogdown", input)
				|| isMappedTo("joystick1up", input, true) || isMappedTo("joystick2up", input, true);

	return isMappedTo(name, input);
}

std::vector<std::string> InputConfig::getMappedTo(Input input)
{
	std::vector<std::string> maps;

	typedef std::map<std::string, Input>::const_iterator it_type;
	for (it_type iterator = mNameMap.cbegin(); iterator != mNameMap.cend(); iterator++)
	{
		Input chk = iterator->second;

		if (!chk.configured)
			continue;

		if (chk.device == input.device && chk.type == input.type && chk.id == input.id)
		{
			if (chk.type == TYPE_HAT)
			{
				if (input.value == 0 || input.value & chk.value)
					maps.push_back(iterator->first);

				continue;
			}

			if (input.type == TYPE_AXIS)
			{
				if (input.value == 0 || chk.value == input.value)
					maps.push_back(iterator->first);
			}
			else
				maps.push_back(iterator->first);
		}
	}

	return maps;
}

void InputConfig::loadFromXML(pugi::xml_node& node)
{
	clear();

	for (pugi::xml_node input = node.child("input"); input; input = input.next_sibling("input"))
	{
		std::string name = input.attribute("name").as_string();
		std::string type = input.attribute("type").as_string();
		InputType typeEnum = stringToInputType(type);
		std::string lower_name = toLower(name);
//		LOG(LogDebug) << "InputConfig::loadFromXML() - button name: " << name << ", lower_name: " << lower_name;
		if (typeEnum == TYPE_BUTTON)
		{
			if (lower_name == "leftshoulder")
				lower_name = L1BUTTON;
			else if (lower_name == "rightshoulder")
				lower_name = R1BUTTON;
			else if (lower_name == "lefttrigger")
				lower_name = L2BUTTON;
			else if (lower_name == "righttrigger")
				lower_name = R2BUTTON;
			else if (lower_name == "pageup")
				lower_name = BUTTON_PU;
			else if (lower_name == "pagedown")
				lower_name = BUTTON_PD;
			else if (lower_name == "leftthumb")
				lower_name = BUTTON_LTH;
			else if (lower_name == "rightthumb")
				lower_name = BUTTON_RTH;
			
//			LOG(LogDebug) << "InputConfig::loadFromXML() - button name changed from '" << name << "' to '" << lower_name << "'";
		}

		if (typeEnum == TYPE_COUNT)
		{
			LOG(LogError) << "InputConfig::loadFromXML() - load error - input of type \"" << type << "\" is invalid! Skipping input '" << name << "' ('" << lower_name << "').\n";
			continue;
		}

		int id = input.attribute("id").as_int();
		int value = input.attribute("value").as_int();

		if (value == 0)
			LOG(LogWarning) << "InputConfig::loadFromXML() - value is 0 for '" << name << "' ('" << lower_name << "'), type: " << type << " id: " << id << "!\n";

//		LOG(LogDebug) << "InputConfig::loadFromXML() - input values, name: " << name << "' ('" << lower_name << "'), id: " << id << ", type: " << type << ", value: " << value;
		if (mNameMap.find(lower_name) == mNameMap.end())
			mNameMap[lower_name] = Input(mDeviceId, typeEnum, id, value, true);
		else
			LOG(LogWarning) << "InputConfig::loadFromXML() - button name '" << name << "' ('" << lower_name << "'), already defined, skipping!!!";
	}
}

void InputConfig::writeToXML(pugi::xml_node& parent)
{
	pugi::xml_node cfg = parent.append_child("inputConfig");

	if (mDeviceId == DEVICE_KEYBOARD)
	{
		cfg.append_attribute("type") = "keyboard";
		cfg.append_attribute("deviceName") = "Keyboard";
	}
	else if (mDeviceId == DEVICE_CEC)
	{
		cfg.append_attribute("type") = "cec";
		cfg.append_attribute("deviceName") = "CEC";
	}
	else
	{
		cfg.append_attribute("type") = "joystick";
		cfg.append_attribute("deviceName") = mDeviceName.c_str();
	}

	cfg.append_attribute("deviceGUID") = mDeviceGUID.c_str();
	cfg.append_attribute("deviceDefault") = mDefaultInput ? "true" : "false";

	typedef std::map<std::string, Input>::const_iterator it_type;
	for (it_type iterator = mNameMap.cbegin(); iterator != mNameMap.cend(); iterator++)
	{
		if (!iterator->second.configured)
			continue;

		pugi::xml_node input = cfg.append_child("input");
		input.append_attribute("name") = iterator->first.c_str();
		input.append_attribute("type") = inputTypeToString(iterator->second.type).c_str();
		input.append_attribute("id").set_value(iterator->second.id);
		input.append_attribute("value").set_value(iterator->second.value);
	}
}

char* BUTTON_OK = ABUTTON;
char* BUTTON_BACK = BBUTTON;
char* BUTTON_L1 = L1BUTTON;
char* BUTTON_L2 = L2BUTTON;
char* BUTTON_R1 = R1BUTTON;
char* BUTTON_R2 = R2BUTTON;
char* BUTTON_PU = L2BUTTON;
char* BUTTON_PD = R2BUTTON;
char* BUTTON_LTH = L3BUTTON;
char* BUTTON_RTH = R3BUTTON;

void InputConfig::AssignActionButtons()
{
//	LOG(LogDebug) << "InputConfig::AssignActionButtons() - reload configurable buttons";
	bool invertButtonsAB = Settings::getInstance()->getBool("InvertButtonsAB");
//	LOG(LogDebug) << "InputConfig::AssignActionButtons() - InvertButtonsAB: " << Utils::String::boolToString(invertButtonsAB);

	BUTTON_OK = invertButtonsAB ? BBUTTON : ABUTTON;
	BUTTON_BACK = invertButtonsAB ? ABUTTON : BBUTTON;

	bool invertButtonsPU = Settings::getInstance()->getBool("InvertButtonsPU");
//	LOG(LogDebug) << "InputConfig::AssignActionButtons() - InvertButtonsPU: " << Utils::String::boolToString(invertButtonsPU);
	BUTTON_PU = invertButtonsPU ? L1BUTTON : L2BUTTON;

	bool invertButtonsPD = Settings::getInstance()->getBool("InvertButtonsPD");
//	LOG(LogDebug) << "InputConfig::AssignActionButtons() - InvertButtonsPD: " << Utils::String::boolToString(invertButtonsPD);
	BUTTON_PD = invertButtonsPD ? R1BUTTON : R2BUTTON;
/*
	LOG(LogDebug) << "InputConfig::AssignActionButtons() - Values:\n" <<
		 "\n - BUTTON_OK: '" << BUTTON_OK << "'" <<
		 "\n - BUTTON_BACK: '" << BUTTON_BACK << "'" <<
		 "\n - BUTTON_L1: '" << BUTTON_L1 << "'" <<
		 "\n - BUTTON_R1: '" << BUTTON_R1 << "'" <<
		 "\n - BUTTON_LTH: '" << BUTTON_LTH << "'" <<
		 "\n - BUTTON_L2: '" << BUTTON_L2 << "'" <<
		 "\n - BUTTON_R2: '" << BUTTON_R2 << "'" <<
		 "\n - BUTTON_RTH: '" << BUTTON_RTH << "'" <<
		 "\n - BUTTON_PU: '" << BUTTON_PU << "'" <<
		 "\n - BUTTON_PD: '" << BUTTON_PD << "'";
*/
}
