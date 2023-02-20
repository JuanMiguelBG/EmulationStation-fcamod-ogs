#include "MameNames.h"

#include "resources/ResourceManager.h"
#include "utils/FileSystemUtil.h"
#include "Log.h"
#include <pugixml/src/pugixml.hpp>
#include "utils/StringUtil.h"
#include <string.h>

MameNames* MameNames::sInstance = nullptr;

void MameNames::init()
{
	if(!sInstance)
		sInstance = new MameNames();

} // init

void MameNames::deinit()
{
	if(sInstance)
	{
		delete sInstance;
		sInstance = nullptr;
	}

} // deinit

MameNames* MameNames::getInstance()
{
	if(!sInstance)
		sInstance = new MameNames();

	return sInstance;

} // getInstance

MameNames::MameNames()
{
	std::string xmlpath;

	pugi::xml_document doc;
	pugi::xml_parse_result result;

	// Read mame games information
	xmlpath = ResourceManager::getInstance()->getResourcePath(":/mamenames.xml");
	if (Utils::FileSystem::exists(xmlpath))
	{
		result = doc.load_file(xmlpath.c_str());
		if (result)
		{
			LOG(LogInfo) << "MameNames::MameNames() - Parsing XML file \"" << xmlpath << "\"...";

			pugi::xml_node root = doc;

			pugi::xml_node games = doc.child("games");
			if (games)
				root = games;

			std::string sTrue = "true";
			for (pugi::xml_node gameNode = root.child("game"); gameNode; gameNode = gameNode.next_sibling("game"))
			{
				NamePair namePair = { gameNode.child("mamename").text().get(), gameNode.child("realname").text().get() };
				mNamePairs.push_back(namePair);

				if (gameNode.attribute("vert") && gameNode.attribute("vert").value() == sTrue)
					mVerticalGames.insert(namePair.mameName);
			}
		}
		else
			LOG(LogError) << "MameNames::MameNames() - Error parsing XML file \"" << xmlpath << "\"!\n	" << result.description();
	}

	// Read bios
	xmlpath = ResourceManager::getInstance()->getResourcePath(":/mamebioses.xml");
	if (Utils::FileSystem::exists(xmlpath))
	{
		result = doc.load_file(xmlpath.c_str());
		if (result)
		{
			LOG(LogInfo) << "MameNames::MameNames() - Parsing XML file \"" << xmlpath << "\"...";

			pugi::xml_node root = doc;

			pugi::xml_node bioses = doc.child("bioses");
			if (bioses)
				root = bioses;

			for (pugi::xml_node biosNode = root.child("bios"); biosNode; biosNode = biosNode.next_sibling("bios"))
			{
				std::string bios = biosNode.text().get();
				mMameBioses.insert(bios);
			}
		}
		else
			LOG(LogError) << "MameNames::MameNames() - Error parsing XML file \"" << xmlpath << "\"!\n	" << result.description();

	}

	// Read devices
	xmlpath = ResourceManager::getInstance()->getResourcePath(":/mamedevices.xml");
	if (Utils::FileSystem::exists(xmlpath))
	{
		result = doc.load_file(xmlpath.c_str());
		if (result)
		{
			LOG(LogInfo) << "MameNames::MameNames() - Parsing XML file \"" << xmlpath << "\"...";

			pugi::xml_node root = doc;

			pugi::xml_node devices = doc.child("devices");
			if (devices)
				root = devices;

			for (pugi::xml_node deviceNode = root.child("device"); deviceNode; deviceNode = deviceNode.next_sibling("device"))
			{
				std::string device = deviceNode.text().get();
				mMameDevices.insert(device);
			}
		}
		else
			LOG(LogError) << "MameNames::MameNames() - Error parsing XML file \"" << xmlpath << "\"!\n	" << result.description();
	}

} // MameNames

MameNames::~MameNames()
{

} // ~MameNames

std::string MameNames::getRealName(const std::string& _mameName)
{
	size_t start = 0;
	size_t end   = mNamePairs.size();

	while(start < end)
	{
		const size_t index   = (start + end) / 2;
		const int    compare = strcmp(mNamePairs[index].mameName.c_str(), _mameName.c_str());

		if(compare < 0)       start = index + 1;
		else if( compare > 0) end   = index;
		else                  return mNamePairs[index].realName;
	}

	return _mameName;

} // getRealName

const bool MameNames::isBios(const std::string& _biosName)
{
	return (mMameBioses.find(_biosName) != mMameBioses.cend());
} // isBios

const bool MameNames::isDevice(const std::string& _deviceName)
{
	return (mMameDevices.find(_deviceName) != mMameDevices.cend());
} // isDevice

const bool MameNames::isVertical(const std::string& _gameName)
{
	return (mVerticalGames.find(_gameName) != mVerticalGames.cend());
} // isVertical
