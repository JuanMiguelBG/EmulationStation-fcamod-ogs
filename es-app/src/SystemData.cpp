#include "SystemData.h"

#include "utils/Randomizer.h"
#include "utils/FileSystemUtil.h"
#include "CollectionSystemManager.h"
#include "FileFilterIndex.h"
#include "FileSorts.h"
#include "Gamelist.h"
#include "Log.h"
#include "platform.h"
#include "Settings.h"
#include "ThemeData.h"
#include "views/UIModeController.h"
#include <fstream>
#include "utils/StringUtil.h"
#include "utils/ThreadPool.h"
#include "utils/AsyncUtil.h"
#include "GuiComponent.h"
#include "Window.h"
#include "views/ViewController.h"


#include "MetaData.h"

using namespace Utils;

std::vector<SystemData*> SystemData::sSystemVector;

SystemData::SystemData(const SystemMetadata& meta, SystemEnvironmentData* envData, bool CollectionSystem, bool groupedSystem, bool withTheme, bool loadThemeOnlyIfElements) :
	mMetadata(meta), mEnvData(envData), mIsCollectionSystem(CollectionSystem), mIsGameSystem(true), mHasTheme(withTheme)
{
	mIsGroupSystem = groupedSystem;
	mGameListHash = 0;
	mSortId = Settings::getInstance()->getInt(getName() + ".sort"),
	mGameCountInfo = nullptr;
	mGridSizeOverride = Vector2f(0, 0);
	mViewModeChanged = false;
	mFilterIndex = nullptr;// new FileFilterIndex();

	auto hiddenSystems = Utils::String::split(Settings::getInstance()->getString("HiddenSystems"), ';');
	mHidden = (mIsCollectionSystem ? withTheme : (std::find(hiddenSystems.cbegin(), hiddenSystems.cend(), getName()) != hiddenSystems.cend()));

	// if it's an actual system, initialize it, if not, just create the data structure
	if(!CollectionSystem && !mIsGroupSystem)
	{
		mRootFolder = new FolderData(mEnvData->mStartPath, this);
		mRootFolder->setMetadata(MetaDataId::Name, mMetadata.fullName);

		std::unordered_map<std::string, FileData*> fileMap;
		
		if (!Settings::getInstance()->getBool("ParseGamelistOnly"))
		{
			populateFolder(mRootFolder, fileMap);
			if (mRootFolder->getChildren().size() == 0)
				return;

			if (mHidden)// && !Settings::HiddenSystemsShowGames())
				return;
		}

		if (!Settings::getInstance()->getBool("IgnoreGamelist") && (mMetadata.name != "imageviewer"))
			parseGamelist(this, fileMap);

	}
	else
	{
		// virtual systems are updated afterwards, we're just creating the data structure
		mRootFolder = new FolderData("" + mMetadata.name, this);

//		mRootFolder = new FolderData(mMetadata.fullName, this);
//		mRootFolder->getMetadata().set(MetaDataId::Name, mMetadata.fullName);
	}

	mRootFolder->getMetadata().resetChangedFlag();

	if (withTheme && (!loadThemeOnlyIfElements || mRootFolder->mChildren.size() > 0))
	{
		loadTheme();
		auto defaultView = Settings::getInstance()->getString(getName() + ".defaultView");
		auto gridSizeOverride = Vector2f::parseString(Settings::getInstance()->getString(getName() + ".gridSize"));
		setSystemViewMode(defaultView, gridSizeOverride, false);

		setIsGameSystemStatus();
	}
}

bool SystemData::setSystemViewMode(std::string newViewMode, Vector2f gridSizeOverride, bool setChanged)
{
	if (newViewMode == "automatic")
		newViewMode = "";

	if (mViewMode == newViewMode && gridSizeOverride == mGridSizeOverride)
		return false;

	mGridSizeOverride = gridSizeOverride;
	mViewMode = newViewMode;
	mViewModeChanged = setChanged;

	if (setChanged)
	{
		Settings::getInstance()->setString(getName() + ".defaultView", mViewMode);
		Settings::getInstance()->setString(getName() + ".gridSize", Utils::String::replace(Utils::String::replace(mGridSizeOverride.toString(), ".000000", ""), "0 0", ""));
	}

	return true;
}

Vector2f SystemData::getGridSizeOverride()
{
	return mGridSizeOverride;
}

SystemData::~SystemData()
{
	if (mRootFolder)
		delete mRootFolder;

	if (!mIsCollectionSystem && mEnvData != nullptr)
		delete mEnvData;

	if (mGameCountInfo != nullptr)
		delete mGameCountInfo;

	if (mFilterIndex != nullptr)
		delete mFilterIndex;
}

void SystemData::setIsGameSystemStatus()
{
	// we exclude non-game systems from specific operations (i.e. the "RetroPie" system, at least)
	// if/when there are more in the future, maybe this can be a more complex method, with a proper list
	// but for now a simple string comparison is more performant
	mIsGameSystem = (mMetadata.name != "retropie");
}

void SystemData::populateFolder(FolderData* folder, std::unordered_map<std::string, FileData*>& fileMap)
{
	const std::string& folderPath = folder->getPath();
	if(!Utils::FileSystem::isDirectory(folderPath))
	{
		LOG(LogWarning) << "SystemData::populateFolder() - ERROR: folder with path \"" << folderPath << "\" is not a directory!";
		return;
	}

	//make sure that this isn't a symlink to a thing we already have
	if(Utils::FileSystem::isSymlink(folderPath))
	{
		//if this symlink resolves to somewhere that's at the beginning of our path, it's gonna recurse
		if(folderPath.find(Utils::FileSystem::getCanonicalPath(folderPath)) == 0)
		{
			LOG(LogWarning) << "SystemData::populateFolder() - Skipping infinitely recursive symlink \"" << folderPath << "\"";
			return;
		}
	}
	
//	std::string filePath;
	std::string extension;
	bool isGame;
	bool showHidden = Settings::getInstance()->getBool("ShowHiddenFiles");
	bool preloadMedias = Settings::getInstance()->getBool("PreloadMedias");
	
	Utils::FileSystem::fileList dirContent = Utils::FileSystem::getDirInfo(folderPath);

	for(Utils::FileSystem::fileList::const_iterator it = dirContent.cbegin(); it != dirContent.cend(); ++it)
	{
		auto fileInfo = *it;
		//filePath = *it;
		std::string filePath = fileInfo.path;

		// skip hidden files and folders
		if(!showHidden && fileInfo.hidden)
			continue;

		//this is a little complicated because we allow a list of extensions to be defined (delimited with a space)
		//we first get the extension of the file itself:
		extension = Utils::String::toLower(Utils::FileSystem::getExtension(filePath));

		//fyi, folders *can* also match the extension and be added as games - this is mostly just to support higan
		//see issue #75: https://github.com/Aloshi/EmulationStation/issues/75
		
		isGame = false;
		if (mEnvData->isValidExtension(extension)) //std::find(mEnvData->mSearchExtensions.cbegin(), mEnvData->mSearchExtensions.cend(), extension) != mEnvData->mSearchExtensions.cend())
		{
			if (fileMap.find(filePath) == fileMap.end())
			{
				FileData* newGame = new FileData(GAME, filePath, this);

				// preventing new arcade assets to be added
				if (extension != ".zip" || !newGame->isArcadeAsset())
				{
					folder->addChild(newGame);
					fileMap[filePath] = newGame;
					isGame = true;
				}
			}
		}
		
		//add directories that also do not match an extension as folders
		if (!isGame && fileInfo.directory)
		{
			std::string fn = Utils::String::toLower(Utils::FileSystem::getFileName(filePath));

			if (preloadMedias && !mHidden)// (!mHidden || Settings::HiddenSystemsShowGames()))
			{
				// Recurse list files in medias folder, just to let OS build filesystem cache
				if (fn == "media" || fn == "medias")
				{
					Utils::FileSystem::getDirContent(filePath, true);
					continue;
				}

				// List files in folder, just to get OS build filesystem cache
				if (fn == "manuals" || fn == "images" || fn == "videos" || Utils::String::startsWith(fn, "downloaded_"))
				{
					Utils::FileSystem::getDirectoryFiles(filePath);
					continue;
				}
			}

			// Don't loose time looking in downloaded_images, downloaded_videos & media folders
			if (fn == "media" || fn == "medias" || fn == "images" || fn == "manuals" || fn == "videos" || fn == "assets" || Utils::String::startsWith(fn, "downloaded_") || Utils::String::startsWith(fn, "."))
				continue;

			// Hardcoded optimisation : WiiU has so many files in content & meta directories
			if (mMetadata.name == "wiiu" && (fn == "content" || fn == "meta"))
				continue;

			FolderData* newFolder = new FolderData(filePath, this);
			populateFolder(newFolder, fileMap);

			if (newFolder->getChildren().size() == 0)
				delete newFolder;
			else
			{
				const std::string& key = newFolder->getPath();
				if (fileMap.find(key) == fileMap.end())
				{
					folder->addChild(newFolder);
					fileMap[key] = newFolder;
				}
			}
		}
	}
}

FileFilterIndex* SystemData::getIndex(bool createIndex) 
{ 
	if (mFilterIndex == nullptr && createIndex)
	{
		mFilterIndex = new FileFilterIndex();
		indexAllGameFilters(mRootFolder);
		mFilterIndex->setUIModeFilters();
	}

	return mFilterIndex; 
}

void SystemData::deleteIndex()
{
	if (mFilterIndex != nullptr)
	{
		delete mFilterIndex;
		mFilterIndex = nullptr;
	}
}

void SystemData::indexAllGameFilters(const FolderData* folder)
{
	const std::vector<FileData*>& children = folder->getChildren();

	for(std::vector<FileData*>::const_iterator it = children.cbegin(); it != children.cend(); ++it)
	{
		switch((*it)->getType())
		{
			case GAME:   { mFilterIndex->addToIndex(*it); } break;
			case FOLDER: { indexAllGameFilters((FolderData*)*it); } break;
		}
	}
}

std::vector<std::string> readList(const std::string& str, const char* delims = " \t\r\n,")
{
	std::vector<std::string> ret;

	size_t prevOff = str.find_first_not_of(delims, 0);
	size_t off = str.find_first_of(delims, prevOff);
	while(off != std::string::npos || prevOff != std::string::npos)
	{
		ret.push_back(str.substr(prevOff, off - prevOff));

		prevOff = str.find_first_not_of(delims, off);
		off = str.find_first_of(delims, prevOff);
	}

	return ret;
}

SystemData* SystemData::loadSystem(pugi::xml_node system)
{
	std::vector<EmulatorData> emulatorList;

	std::string path, cmd, defaultCore;

	SystemMetadata md;
	md.name = system.child("name").text().get();
	md.fullName = system.child("fullname").text().get();
	md.manufacturer = system.child("manufacturer").text().get();
	md.releaseYear = atoi(system.child("release").text().get());
	md.hardwareType = system.child("hardware").text().get();
	md.themeFolder = system.child("theme").text().as_string(md.name.c_str());
	path = system.child("path").text().get();
	defaultCore = system.child("defaultCore").text().get();

	pugi::xml_node emulators = system.child("emulators");
	if (emulators != NULL)
	{
		for (pugi::xml_node emulator : emulators.children())
		{
			EmulatorData emulatorData;
			emulatorData.mName = emulator.attribute("name").value();
			emulatorData.mCommandLine = emulator.attribute("command").value();

			pugi::xml_node cores = emulator.child("cores");
			if (cores != NULL)
			{
				for (pugi::xml_node core : cores.children())
				{
					const std::string& corename = core.text().get();

					if (defaultCore.length() == 0)
						defaultCore = corename;

					emulatorData.mCores.push_back(corename);
				}
			}

			emulatorList.push_back(emulatorData);
		}
	}
	/*
	if (window != NULL)
		window->renderLoadingScreen(fullname, systemCount == 0 ? 0 : currentSystem / systemCount);

	currentSystem++;
	*/
	// convert extensions list from a string into a vector of strings
	std::set<std::string> extensions;
	for (auto ext : readList(system.child("extension").text().get()))
	{
		std::string extlow = Utils::String::toLower(ext);
		if (extensions.find(extlow) == extensions.cend())
			extensions.insert(extlow);
	}

	cmd = system.child("command").text().get();

	// platform id list
	const char* platformList = system.child("platform").text().get();
	std::vector<std::string> platformStrs = readList(platformList);
	std::vector<PlatformIds::PlatformId> platformIds;
	for (auto it = platformStrs.cbegin(); it != platformStrs.cend(); it++)
	{
		const char* str = it->c_str();
		PlatformIds::PlatformId platformId = PlatformIds::getPlatformId(str);

		if (platformId == PlatformIds::PLATFORM_IGNORE)
		{
			// when platform is ignore, do not allow other platforms
			platformIds.clear();
			platformIds.push_back(platformId);
			break;
		}

		// if there appears to be an actual platform ID supplied but it didn't match the list, warn
		if (platformId != PlatformIds::PLATFORM_UNKNOWN)
			platformIds.push_back(platformId);
		else if (str != NULL && str[0] != '\0' && platformId == PlatformIds::PLATFORM_UNKNOWN)
			LOG(LogWarning) << "SystemData::loadSystem() - Unknown platform for system \"" << md.name << "\" (platform \"" << str << "\" from list \"" << platformList << "\")";
	}

	//convert path to generic directory seperators
	path = Utils::FileSystem::getGenericPath(path);

	//expand home symbol if the startpath contains ~
	if (path[0] == '~')
	{
		path.erase(0, 1);
		path.insert(0, Utils::FileSystem::getHomePath());
		path = Utils::FileSystem::getCanonicalPath(path);
	}

	//validate
	if (md.name.empty() || path.empty() || extensions.empty() || cmd.empty() || !Utils::FileSystem::exists(path))
	{
		LOG(LogError) << "SystemData::loadSystem() - System \"" << md.name << "\" is missing name, path, extension, or command!";
		return nullptr;
	}

	//create the system runtime environment data
	SystemEnvironmentData* envData = new SystemEnvironmentData;
	envData->mSystemName = md.name;
	envData->mStartPath = path;
	envData->mSearchExtensions = extensions;
	envData->mLaunchCommand = cmd;
	envData->mPlatformIds = platformIds;
	envData->mEmulators = emulatorList;
	envData->mGroup = system.child("group").text().get();

	SystemData* newSys = new SystemData(md, envData, false, false, !md.themeFolder.empty(), true);
	if (newSys->getRootFolder()->getChildren().size() == 0)
	{
		LOG(LogWarning) << "SystemData::loadSystem() - System \"" << md.name << "\" has no games! Ignoring it.";
		delete newSys;

		return nullptr;
	}

	return newSys;
}

void SystemData::createGroupedSystems()
{
	auto hiddenSystems = Utils::String::split(Settings::getInstance()->getString("HiddenSystems"), ';');

	std::map<std::string, std::vector<SystemData*>> map;

	for (auto sys : sSystemVector)
	{
		if (sys->isCollection() || sys->getSystemEnvData()->mGroup.empty())
			continue;

		if (Settings::getInstance()->getBool(sys->getSystemEnvData()->mGroup + ".ungroup") || Settings::getInstance()->getBool(sys->getName() + ".ungroup"))
			continue;

		if (sys->getName() == sys->getSystemEnvData()->mGroup)
		{
			sys->getSystemEnvData()->mGroup = "";
			continue;
		}
		else if (std::find(hiddenSystems.cbegin(), hiddenSystems.cend(), sys->getName()) != hiddenSystems.cend())
			continue;

		map[sys->getSystemEnvData()->mGroup].push_back(sys);
	}

	for (auto item : map)
	{
		SystemEnvironmentData* envData = new SystemEnvironmentData;
		envData->mStartPath = "";
		envData->mLaunchCommand = "";

		SystemMetadata md;
		md.name = item.first;
		md.fullName = item.first;
		md.themeFolder = item.first;

		SystemData* system = new SystemData(md, envData, false, true);
		system->mIsGameSystem = false;

		FolderData* root = system->getRootFolder();

		for (auto childSystem : item.second)
		{			
			auto children = childSystem->getRootFolder()->getChildren();
			if (children.size() > 0)
			{
				auto folder = new FolderData(childSystem->getRootFolder()->getPath(), childSystem, false);
				root->addChild(folder);

				auto theme = childSystem->getTheme();
				if (theme)
				{
					const ThemeData::ThemeElement* logoElem = theme->getElement("system", "logo", "image");
					if (logoElem && logoElem->has("path"))
					{
						std::string path = logoElem->get<std::string>("path");
						folder->setMetadata(MetaDataId::Image, path);
						folder->setMetadata(MetaDataId::Thumbnail, path);

						folder->enableVirtualFolderDisplay(true);
					}
				}

				for (auto child : children)
					folder->addChild(child, false);
			}
		}

		if (root->getChildren().size() > 0)
		{
			system->loadTheme();
			sSystemVector.push_back(system);
		}

		root->getMetadata().resetChangedFlag();
	}
}

//creates systems from information located in a config file
bool SystemData::loadConfig(Window* window)
{
	deleteSystems();
	ThemeData::setDefaultTheme(nullptr);

	std::string path = getConfigPath(false);

	LOG(LogInfo) << "SystemData::loadConfig() - Loading system config file '" << path << "'...";

	if(!Utils::FileSystem::exists(path))
	{
		LOG(LogError) << "SystemData::loadConfig() - '" << path << "' file does not exist!";
		Log::flush();
		writeExampleConfig(getConfigPath(true));
		return false;
	}

	pugi::xml_document doc;
	pugi::xml_parse_result res = doc.load_file(path.c_str());

	if(!res)
	{
		LOG(LogError) << "SystemData::loadConfig() - Could not parse '" << path << "' file!";
		LOG(LogError) << "SystemData::loadConfig() - " << res.description();
		return false;
	}

	//actually read the file
	pugi::xml_node systemList = doc.child("systemList");

	if(!systemList)
	{
		LOG(LogError) << "SystemData::loadConfig() - '" << path << "' is missing the <systemList> tag!";
		return false;
	}

	std::vector<std::string> systemsNames;
	
	int systemCount = 0;
	for (pugi::xml_node system = systemList.child("system"); system; system = system.next_sibling("system"))
	{
		systemsNames.push_back(system.child("fullname").text().get());
		systemCount++;
	}

	Utils::FileSystem::FileSystemCacheActivator fsc;

	int currentSystem = 0;

	typedef SystemData* SystemDataPtr;

	ThreadPool* pThreadPool = NULL;
	SystemDataPtr* systems = NULL;
	
	if (Utils::Async::isCanRunAsync())
	{
		LOG(LogInfo) << "SystemData::loadConfig() - Thread Loading Collection Systems!";
		pThreadPool = new ThreadPool();

		systems = new SystemDataPtr[systemCount];
		for (int i = 0; i < systemCount; i++)
			systems[i] = nullptr;

		pThreadPool->queueWorkItem([] { CollectionSystemManager::get()->loadCollectionSystems(true); });
	}

	int processedSystem = 0;
	
	for (pugi::xml_node system = systemList.child("system"); system; system = system.next_sibling("system"))
	{		
		if (pThreadPool != NULL)
		{
			pThreadPool->queueWorkItem([system, currentSystem, systems, &processedSystem]
			{
				systems[currentSystem] = loadSystem(system);
				processedSystem++;
			});
		}
		else
		{
			std::string fullname = system.child("fullname").text().get();

			if (window != NULL)
				window->renderLoadingScreen(fullname, systemCount == 0 ? 0 : (float)currentSystem / (float)(systemCount + 1));

			std::string nm = system.child("name").text().get();
			StopWatch watch("SystemData " + nm);

			SystemData* pSystem = loadSystem(system);
			if (pSystem != nullptr)
				sSystemVector.push_back(pSystem);
		}

		currentSystem++;
	}

	if (pThreadPool != NULL)
	{
		if (window != NULL)
		{
			pThreadPool->wait([window, &processedSystem, systemCount, &systemsNames]
			{
				int px = processedSystem - 1;
				if (px >= 0 && px < systemsNames.size())
					window->renderLoadingScreen(systemsNames.at(px), (float)px / (float)(systemCount + 1));
			}, 10);
		}
		else
			pThreadPool->wait();

		for (int i = 0; i < systemCount; i++)
		{
			SystemData* pSystem = systems[i];
			if (pSystem != nullptr)
				sSystemVector.push_back(pSystem);
		}
		
		delete[] systems;
		delete pThreadPool;

		if (window != NULL)
			window->renderLoadingScreen(_("Favorites"), systemCount == 0 ? 0 : currentSystem / systemCount);

		createGroupedSystems();
		CollectionSystemManager::get()->updateSystemsList();
	}
	else
	{
		if (window != NULL)
			window->renderLoadingScreen(_("Favorites"), systemCount == 0 ? 0 : currentSystem / systemCount);

		createGroupedSystems();
		CollectionSystemManager::get()->loadCollectionSystems();
	}

	if (SystemData::sSystemVector.size() > 0)
	{
		auto theme = SystemData::sSystemVector.at(0)->getTheme();
		if (theme != nullptr)
			ViewController::get()->onThemeChanged(theme);
	}

	return true;
}

void SystemData::writeExampleConfig(const std::string& path)
{
	std::ofstream file(path.c_str());

	file << "<!-- This is the EmulationStation Systems configuration file.\n"
			"All systems must be contained within the <systemList> tag.-->\n"
			"\n"
			"<systemList>\n"
			"	<!-- Here's an example system to get you started. -->\n"
			"	<system>\n"
			"\n"
			"		<!-- A short name, used internally. Traditionally lower-case. -->\n"
			"		<name>nes</name>\n"
			"\n"
			"		<!-- A \"pretty\" name, displayed in menus and such. -->\n"
			"		<fullname>Nintendo Entertainment System</fullname>\n"
			"\n"
			"		<!-- The path to start searching for ROMs in. '~' will be expanded to $HOME on Linux or %HOMEPATH% on Windows. -->\n"
			"		<path>~/roms/nes</path>\n"
			"\n"
			"		<!-- A list of extensions to search for, delimited by any of the whitespace characters (\", \\r\\n\\t\").\n"
			"		You MUST include the period at the start of the extension! It's also case sensitive. -->\n"
			"		<extension>.nes .NES</extension>\n"
			"\n"
			"		<!-- The shell command executed when a game is selected. A few special tags are replaced if found in a command:\n"
			"		%ROM% is replaced by a bash-special-character-escaped absolute path to the ROM.\n"
			"		%BASENAME% is replaced by the \"base\" name of the ROM.  For example, \"/foo/bar.rom\" would have a basename of \"bar\". Useful for MAME.\n"
			"		%ROM_RAW% is the raw, unescaped path to the ROM. -->\n"
			"		<command>retroarch -L ~/cores/libretro-fceumm.so %ROM%</command>\n"
			"\n"
			"		<!-- The platform to use when scraping. You can see the full list of accepted platforms in src/PlatformIds.cpp.\n"
			"		It's case sensitive, but everything is lowercase. This tag is optional.\n"
			"		You can use multiple platforms too, delimited with any of the whitespace characters (\", \\r\\n\\t\"), eg: \"genesis, megadrive\" -->\n"
			"		<platform>nes</platform>\n"
			"\n"
			"		<!-- The theme to load from the current theme set.  See THEMES.md for more information.\n"
			"		This tag is optional. If not set, it will default to the value of <name>. -->\n"
			"		<theme>nes</theme>\n"
			"	</system>\n"
			"</systemList>\n";

	file.close();

	LOG(LogError) << "SystemData::writeExampleConfig() - Example config written!  Go read it at \"" << path << "\"!";
	Log::flush();
}

bool SystemData::isManufacturerSupported()
{
	for (auto sys : sSystemVector)
	{
		if (!sys->isGameSystem() || sys->isCollection())
			continue;

		if (!sys->getSystemMetadata().manufacturer.empty())
			return true;
	}

	return false;
}


bool SystemData::hasDirtySystems()
{
	bool saveOnExit = !Settings::getInstance()->getBool("IgnoreGamelist") && Settings::getInstance()->getBool("SaveGamelistsOnExit");
	if (!saveOnExit)
		return false;

	for (unsigned int i = 0; i < sSystemVector.size(); i++)
	{
		SystemData* pData = sSystemVector.at(i);
		if (pData->mIsCollectionSystem)
			continue;

		
		if (hasDirtyFile(pData))
			return true;
	}

	return false;
}

void SystemData::deleteSystems()
{
	bool saveOnExit = !Settings::getInstance()->getBool("IgnoreGamelist") && Settings::getInstance()->getBool("SaveGamelistsOnExit");

	for(unsigned int i = 0; i < sSystemVector.size(); i++)
	{
		SystemData* pData = sSystemVector.at(i);

		if (saveOnExit && !pData->mIsCollectionSystem)
			updateGamelist(pData);

		delete pData;
	}

	sSystemVector.clear();
}

std::string SystemData::getConfigPath(bool forWrite)
{
	std::string path = Utils::FileSystem::getEsConfigPath() + "/es_systems.cfg";

	if(forWrite || Utils::FileSystem::exists(path))
		return path;

	return "/etc/emulationstation/es_systems.cfg";
}

bool SystemData::isVisible()
{
	if (isGroupChildSystem())
		return false;

	if ((getGameCountInfo()->totalGames > 0 || (UIModeController::getInstance()->isUIModeFull() && mIsCollectionSystem) || (mIsCollectionSystem && mMetadata.name == "favorites")))
	{
		if (!mIsCollectionSystem)
		{
			return !mHidden;
		}

		return true;
	}

	return false;
}

SystemData* SystemData::getNext() const
{
	std::vector<SystemData*>::const_iterator it = getIterator();

	do {
		it++;
		if (it == sSystemVector.cend())
			it = sSystemVector.cbegin();
	} while (!(*it)->isVisible());
	// as we are starting in a valid gamelistview, this will always succeed, even if we have to come full circle.

	return *it;
}

SystemData* SystemData::getPrev() const
{
	std::vector<SystemData*>::const_reverse_iterator it = getRevIterator();

	do {
		it++;
		if (it == sSystemVector.crend())
			it = sSystemVector.crbegin();
	} while (!(*it)->isVisible());
	// as we are starting in a valid gamelistview, this will always succeed, even if we have to come full circle.

	return *it;
}

std::string SystemData::getGamelistPath(bool forWrite) const
{
	std::string fileRomPath = mRootFolder->getPath() + "/gamelist.xml";
	if(Utils::FileSystem::exists(fileRomPath))
		return fileRomPath;

	std::string filePath = Utils::FileSystem::getEsConfigPath() + "/gamelists/" + mMetadata.name + "/gamelist.xml";

	// Default to system rom folder
	if (forWrite && !Utils::FileSystem::exists(filePath) && Utils::FileSystem::isDirectory(mRootFolder->getPath()))
		return fileRomPath;

	if(forWrite) // make sure the directory exists if we're going to write to it, or crashes will happen
		Utils::FileSystem::createDirectory(Utils::FileSystem::getParent(filePath));
	
	if (forWrite || Utils::FileSystem::exists(filePath))
		return filePath;

	return "/etc/emulationstation/gamelists/" + mMetadata.name + "/gamelist.xml";
}

std::string SystemData::getThemePath() const
{
	// where we check for themes, in order:
	// 1. [SYSTEM_PATH]/theme.xml
	// 2. system theme from currently selected theme set [CURRENT_THEME_PATH]/[SYSTEM]/theme.xml
	// 3. default system theme from currently selected theme set [CURRENT_THEME_PATH]/theme.xml

	// first, check game folder
	std::string localThemePath = mRootFolder->getPath() + "/theme.xml";
	if(Utils::FileSystem::exists(localThemePath))
		return localThemePath;

	// not in game folder, try system theme in theme sets
	localThemePath = ThemeData::getThemeFromCurrentSet(mMetadata.themeFolder);

	if (Utils::FileSystem::exists(localThemePath))
		return localThemePath;

	// not system theme, try default system theme in theme set
	localThemePath = Utils::FileSystem::getParent(Utils::FileSystem::getParent(localThemePath)) + "/theme.xml";

	return localThemePath;
}

bool SystemData::hasGamelist() const
{
	return (Utils::FileSystem::exists(getGamelistPath(false)));
}

unsigned int SystemData::getGameCount() const
{
	return (unsigned int)mRootFolder->getFilesRecursive(GAME).size();
}

SystemData* SystemData::getRandomSystem()
{
	//  this is a bit brute force. It might be more efficient to just to a while (!gameSystem) do random again...
	unsigned int total = 0;
	for(auto it = sSystemVector.cbegin(); it != sSystemVector.cend(); it++)
	{
		if ((*it)->isGameSystem())
			total ++;
	}

	// get random number in range
	int target = Randomizer::random(total);
	for (auto it = sSystemVector.cbegin(); it != sSystemVector.cend(); it++)
	{
		if ((*it)->isGameSystem())
		{
			if (target > 0)
			{
				target--;
			}
			else
			{
				return (*it);
			}
		}
	}

	// if we end up here, there is no valid system
	return NULL;
}

FileData* SystemData::getRandomGame()
{
	std::vector<FileData*> list = mRootFolder->getFilesRecursive(GAME, true);
	unsigned int total = (int)list.size();

	// get random number in range
	if (total == 0)
		return NULL;

	int target = Randomizer::random(total);
	return list.at(target);
}

GameCountInfo* SystemData::getGameCountInfo()
{
	if (mGameCountInfo != nullptr)
		return mGameCountInfo;

	std::vector<FileData*> games = mRootFolder->getFilesRecursive(GAME, true);

	int realTotal = games.size();
	if (mFilterIndex != nullptr)
	{
		auto savedFilter = mFilterIndex;
		mFilterIndex = nullptr;
		realTotal = mRootFolder->getFilesRecursive(GAME, true).size();
		mFilterIndex = savedFilter;
	}


	mGameCountInfo = new GameCountInfo();
	mGameCountInfo->visibleGames = games.size();
	mGameCountInfo->totalGames = realTotal;
	mGameCountInfo->favoriteCount = 0;
	mGameCountInfo->hiddenCount = 0;
	mGameCountInfo->playCount = 0;
	mGameCountInfo->gamesPlayed = 0;

	int mostPlayCount = 0;

	for (auto game : games)
	{
		if (game->getFavorite())
			mGameCountInfo->favoriteCount++;

		if (game->getHidden())
			mGameCountInfo->hiddenCount++;

		int playCount = Utils::String::toInteger(game->getMetadata(MetaDataId::PlayCount));
		if (playCount > 0)
		{
			mGameCountInfo->gamesPlayed++;
			mGameCountInfo->playCount += playCount;

			if (playCount > mostPlayCount)
			{
				mGameCountInfo->mostPlayed = game->getName();
				mostPlayCount = playCount;
			}
		}

		auto lastPlayed = game->getMetadata(MetaDataId::LastPlayed);
		if (!lastPlayed.empty() && lastPlayed > mGameCountInfo->lastPlayedDate)
			mGameCountInfo->lastPlayedDate = lastPlayed;
	}

	return mGameCountInfo;
}

void SystemData::updateDisplayedGameCount()
{
	if (mGameCountInfo != nullptr)
		delete mGameCountInfo;

	mGameCountInfo = nullptr;
}

void SystemData::loadTheme()
{
	//StopWatch watch("SystemData::loadTheme " + getName());

	mTheme = std::make_shared<ThemeData>();

	std::string path = getThemePath();
	if (!Utils::FileSystem::exists(path)) // no theme available for this platform
		return;

	try
	{
		// build map with system variables for theme to use,
		std::map<std::string, std::string> sysData;
		sysData.insert(std::pair<std::string, std::string>("system.name", getName()));
		sysData.insert(std::pair<std::string, std::string>("system.theme", getThemeFolder()));
		sysData.insert(std::pair<std::string, std::string>("system.fullName", Utils::String::proper(getFullName())));

		sysData.insert(std::pair<std::string, std::string>("system.manufacturer", getSystemMetadata().manufacturer));
		sysData.insert(std::pair<std::string, std::string>("system.hardwareType", Utils::String::proper(getSystemMetadata().hardwareType)));

		if (Settings::getInstance()->getString("SortSystems") == "hardware")
			sysData.insert(std::pair<std::string, std::string>("system.sortedBy", Utils::String::proper(getSystemMetadata().hardwareType)));
		else
			sysData.insert(std::pair<std::string, std::string>("system.sortedBy", getSystemMetadata().manufacturer));

		if (getSystemMetadata().releaseYear > 0)
		{
			sysData.insert(std::pair<std::string, std::string>("system.releaseYearOrNull", std::to_string(getSystemMetadata().releaseYear)));
			sysData.insert(std::pair<std::string, std::string>("system.releaseYear", std::to_string(getSystemMetadata().releaseYear)));
		}
		else
			sysData.insert(std::pair<std::string, std::string>("system.releaseYear", _("Unknown")));

		mTheme->loadFile(getThemeFolder(), sysData, path);
	}
	catch(ThemeException& e)
	{
		LOG(LogError) << "SystemData::loadTheme() - Error: " << e.what();
		mTheme = std::make_shared<ThemeData>(); // reset to empty
	}
}

void SystemData::setSortId(const unsigned int sortId)
{
	mSortId = sortId;
	Settings::getInstance()->setInt(getName() + ".sort", mSortId);
}

bool SystemData::isGroupChildSystem()
{
	if (mEnvData != nullptr && !mEnvData->mGroup.empty())
		return !Settings::getInstance()->getBool(mEnvData->mGroup + ".ungroup") &&
						!Settings::getInstance()->getBool(getName() + ".ungroup");

	return false;
}

std::unordered_set<std::string> SystemData::getAllGroupNames()
{
	auto hiddenSystems = Utils::String::split(Settings::getInstance()->getString("HiddenSystems"), ';');

	std::unordered_set<std::string> names;

	for (auto sys : SystemData::sSystemVector)
	{
		std::string name;
		if (sys->isGroupSystem())
			name = sys->getName();
		else if (sys->mEnvData != nullptr && !sys->mEnvData->mGroup.empty())
			name = sys->mEnvData->mGroup;

		if (!name.empty() && std::find(hiddenSystems.cbegin(), hiddenSystems.cend(), name) == hiddenSystems.cend())
			names.insert(name);
	}

	return names;
}

std::unordered_set<std::string> SystemData::getGroupChildSystemNames(const std::string groupName)
{
	std::unordered_set<std::string> names;

	for (auto sys : SystemData::sSystemVector)
		if (sys->mEnvData != nullptr && sys->mEnvData->mGroup == groupName)
			names.insert(sys->getFullName());

	return names;
}

SystemData* SystemData::getParentGroupSystem()
{
	if (!isGroupChildSystem() || isGroupSystem())
		return this;

	for (auto sys : SystemData::sSystemVector)
		if (sys->isGroupSystem() && sys->getName() == mEnvData->mGroup)
			return sys;

	return this;
}

SystemData* SystemData::getSystem(const std::string name)
{
	for (auto sys : SystemData::sSystemVector)
		if (sys->getName() == name && sys->mTheme != nullptr && sys->isVisible())
			return sys;

	return nullptr;
}

SystemData* SystemData::getFirstVisibleSystem()
{
	for (auto sys : SystemData::sSystemVector)
		if (sys->mTheme != nullptr && sys->isVisible())
			return sys;

	return nullptr;
}

bool SystemData::shouldExtractHashesFromArchives()
{
	return
		!hasPlatformId(PlatformIds::ARCADE) &&
		!hasPlatformId(PlatformIds::NEOGEO) &&
		!hasPlatformId(PlatformIds::DAPHNE) &&
		!hasPlatformId(PlatformIds::LUTRO) &&
		!hasPlatformId(PlatformIds::SEGA_DREAMCAST) &&
		!hasPlatformId(PlatformIds::ATOMISWAVE) &&
		!hasPlatformId(PlatformIds::NAOMI);
}
