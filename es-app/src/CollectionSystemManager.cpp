#include "CollectionSystemManager.h"

#include "guis/GuiInfoPopup.h"
#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"
#include "views/gamelist/IGameListView.h"
#include "views/ViewController.h"
#include "FileData.h"
#include "FileFilterIndex.h"
#include "Log.h"
#include "Settings.h"
#include "SystemData.h"
#include "ThemeData.h"
#include "EsLocale.h"
#include <pugixml/src/pugixml.hpp>
#include <fstream>
#include "Gamelist.h"
#include "FileSorts.h"
#include "utils/ThreadPool.h"
#include "utils/AsyncUtil.h"

std::string myCollectionsName = "collections";

#define LAST_PLAYED_MAX	50

/* Handling the getting, initialization, deinitialization, saving and deletion of
 * a CollectionSystemManager Instance */
CollectionSystemManager* CollectionSystemManager::sInstance = NULL;

std::vector<CollectionSystemDecl> CollectionSystemManager::getSystemDecls()
{
	CollectionSystemDecl systemDecls[] = {
		//type                  name            long name            //default sort              // theme folder            // isCustom
		{ AUTO_ALL_GAMES,       "all",          "all games",         "filename, ascending",      "auto-allgames",           false,       true },
		{ AUTO_LAST_PLAYED,     "recent",       "last played",       "last played, descending",  "auto-lastplayed",         false,       true },
		{ AUTO_FAVORITES,       "favorites",    "favorites",         "filename, ascending",      "auto-favorites",          false,       true },
		{ AUTO_AT2PLAYERS,      "2players",     "2 players",         "filename, ascending",      "auto-at2players",         false,       true },
		{ AUTO_AT4PLAYERS,      "4players",     "4 players",         "filename, ascending",      "auto-at4players",         false,       true },
		{ AUTO_NEVER_PLAYED,    "neverplayed",  "never played",      "filename, ascending",      "auto-neverplayed",        false,       true },
		{ AUTO_VERTICALARCADE,  "vertical",     "vertical arcade",   "filename, ascending",      "auto-verticalarcade",     false,       true }, // batocera

		// Arcade meta 
		{ AUTO_ARCADE,          "arcade",       "arcade",            "filename, ascending",      "arcade",                  false,       true },

		// Arcade systems
		{ CPS1_COLLECTION,      "zcps1",       "cps1",                 "filename, ascending",    "cps1",                    false,       false },
		{ CPS2_COLLECTION,      "zcps2",       "cps2",                 "filename, ascending",    "cps2",                    false,       false },
		{ CPS3_COLLECTION,      "zcps3",       "cps3",                 "filename, ascending",    "cps3",                    false,       false },
		{ CAVE_COLLECTION,      "zcave",       "cave",                 "filename, ascending",    "cave",                    false,       false },
		{ NEOGEO_COLLECTION,    "zneogeo",     "neogeo",               "filename, ascending",    "neogeo",                  false,       false },
		{ SEGA_COLLECTION,      "zsega",       "sega",                 "filename, ascending",    "sega",                    false,       false },
		{ IREM_COLLECTION,      "zirem",       "irem",                 "filename, ascending",    "irem",                    false,       false },
		{ MIDWAY_COLLECTION,    "zmidway",     "midway",               "filename, ascending",    "midway",                  false,       false },
		{ CAPCOM_COLLECTION,    "zcapcom",     "capcom",               "filename, ascending",    "capcom",                  false,       false },
		{ TECMO_COLLECTION,     "ztecmo",      "tecmo",                "filename, ascending",    "tecmo",                   false,       false },
		{ SNK_COLLECTION,       "zsnk",        "snk",                  "filename, ascending",    "snk",                     false,       false },
		{ NAMCO_COLLECTION,     "znamco",      "namco",                "filename, ascending",    "namco",                   false,       false },
		{ TAITO_COLLECTION,     "ztaito",      "taito",                "filename, ascending",    "taito",                   false,       false },
		{ KONAMI_COLLECTION,    "zkonami",     "konami",               "filename, ascending",    "konami",                  false,       false },
		{ JALECO_COLLECTION,    "zjaleco",     "jaleco",               "filename, ascending",    "jaleco",                  false,       false },
		{ ATARI_COLLECTION,     "zatari",      "atari",                "filename, ascending",    "atari",                   false,       false },
		{ NINTENDO_COLLECTION,  "znintendo",   "nintendo",             "filename, ascending",    "nintendo",                false,       false },
		{ SAMMY_COLLECTION,     "zsammy",      "sammy",                "filename, ascending",    "sammy",                   false,       false },
		{ ACCLAIM_COLLECTION,   "zacclaim",    "acclaim",              "filename, ascending",    "acclaim",                 false,       false },
		{ PSIKYO_COLLECTION,    "zpsiko",      "psiko",                "filename, ascending",    "psiko",                   false,       false },
		{ KANEKO_COLLECTION,    "zkaneko",     "kaneko",               "filename, ascending",    "kaneko",                  false,       false },
		{ COLECO_COLLECTION,    "zcoleco",     "coleco",               "filename, ascending",    "coleco",                  false,       false },
		{ ATLUS_COLLECTION,     "zatlus",      "atlus",                "filename, ascending",    "atlus",                   false,       false },
		{ BANPRESTO_COLLECTION, "zbanpresto",  "banpresto",            "filename, ascending",    "banpresto",               false,       false },

		{ CUSTOM_COLLECTION,    myCollectionsName,  "collections",     "filename, ascending",    "custom-collections",      true,        true }
	};

	return std::vector<CollectionSystemDecl>(systemDecls, systemDecls + sizeof(systemDecls) / sizeof(systemDecls[0]));
}

CollectionSystemManager::CollectionSystemManager(Window* window) : mWindow(window)
{
	// create a map
	std::vector<CollectionSystemDecl> tempSystemDecl = getSystemDecls();

	for (std::vector<CollectionSystemDecl>::const_iterator it = tempSystemDecl.cbegin(); it != tempSystemDecl.cend(); ++it )
		mCollectionSystemDeclsIndex[(*it).name] = (*it);

	// creating standard environment data
	mCollectionEnvData = new SystemEnvironmentData;
	mCollectionEnvData->mStartPath = "";
	mCollectionEnvData->mLaunchCommand = "";
	std::vector<PlatformIds::PlatformId> allPlatformIds;
	allPlatformIds.push_back(PlatformIds::PLATFORM_IGNORE);
	mCollectionEnvData->mPlatformIds = allPlatformIds;

	std::string path = getCollectionsFolder();
	if(!Utils::FileSystem::exists(path))
		Utils::FileSystem::createDirectory(path);

	mIsEditingCustom = false;
	mEditingCollection = "Favorites";
	mEditingCollectionSystemData = NULL;
	mCustomCollectionsBundle = NULL;
}

CollectionSystemManager::~CollectionSystemManager()
{
	assert(sInstance == this);
	removeCollectionsFromDisplayedSystems();

	// iterate the map
	for(auto it = mCustomCollectionSystemsData.cbegin() ; it != mCustomCollectionSystemsData.cend() ; it++ )
	{
		if (it->second.isPopulated)
		{
			saveCustomCollection(it->second.system);
		}
		delete it->second.system;
	}

	for (auto it = mAutoCollectionSystemsData.cbegin(); it != mAutoCollectionSystemsData.cend(); it++)
		delete it->second.system;

	if (mCustomCollectionsBundle != nullptr)
	{
		delete mCustomCollectionsBundle;
		mCustomCollectionsBundle = nullptr;
	}

	if (mCollectionEnvData != nullptr)
	{
		delete mCollectionEnvData;
		mCollectionEnvData = nullptr;
	}

	sInstance = NULL;
}

bool systemByAlphaSort(SystemData* sys1, SystemData* sys2)
{
	std::string name1 = Utils::String::toUpper(sys1->getFullName());
	std::string name2 = Utils::String::toUpper(sys2->getFullName());
	return name1.compare(name2) < 0;
}

bool systemBySpecialAlphaSort(SystemData* sys1, SystemData* sys2)
{
	// Move system hardware at End
	std::string hw1 = sys1->getSystemMetadata().hardwareType;
	std::string hw2 = sys2->getSystemMetadata().hardwareType;

	if (hw1 != hw2)
	{
		if (hw1 == "system")
			return false;
		else if (hw2 == "system")
			return true;
	}

	// Move collection at End
	if (sys1->isCollection() != sys2->isCollection())
		return sys2->isCollection();

	// Then by name
	std::string name1 = sys1->getFullName();
	std::string name2 = sys2->getFullName();

	// Move collections bundle at first collection
	if (sys1->isCollection() && sys2->isCollection())
	{
		if (name1 == "collections")
			return true;
		else if (name2 == "collections")
			return false;
	}

	if (hw1 == "auto collection")
		name1 = _(name1);
	if (hw2 == "auto collection")
		name2 = _(name2);

	name1 = Utils::String::toUpper(name1);
	name2 = Utils::String::toUpper(name2);

	return name1.compare(name2) < 0;
}

bool systemByManufacurerSort(SystemData* sys1, SystemData* sys2)
{
	// Move collection at End
	if (sys1->isCollection() != sys2->isCollection())
		return sys2->isCollection();

	// Move custom collections before auto collections
	if (sys1->isCollection() && sys2->isCollection())
	{
		std::string hw1 = Utils::String::toUpper(sys1->getSystemMetadata().hardwareType);
		std::string hw2 = Utils::String::toUpper(sys2->getSystemMetadata().hardwareType);

		if (hw1 != hw2)
			return hw1.compare(hw2) >= 0;
	}

	// Order by manufacturer
	std::string mf1 = Utils::String::toUpper(sys1->getSystemMetadata().manufacturer);
	std::string mf2 = Utils::String::toUpper(sys2->getSystemMetadata().manufacturer);

	if (mf1 != mf2)
		return mf1.compare(mf2) < 0;

	// Then by release date
	if (sys1->getSystemMetadata().releaseYear < sys2->getSystemMetadata().releaseYear)
		return true;
	else if (sys1->getSystemMetadata().releaseYear > sys2->getSystemMetadata().releaseYear)
		return false;

	// Then by name
	std::string name1 = Utils::String::toUpper(sys1->getName());
	std::string name2 = Utils::String::toUpper(sys2->getName());
	if (Settings::getInstance()->getBool("ForceFullNameSortSystems"))
	{
		name1 = Utils::String::toUpper(sys1->getFullName());
		name2 = Utils::String::toUpper(sys2->getFullName());
	}
	return name1.compare(name2) < 0;
}

bool systemByHardwareSort(SystemData* sys1, SystemData* sys2)
{
	// Move collection at End
	if (sys1->isCollection() != sys2->isCollection())
		return sys2->isCollection();

	// Order by hardware
	std::string mf1 = Utils::String::toUpper(sys1->getSystemMetadata().hardwareType);
	std::string mf2 = Utils::String::toUpper(sys2->getSystemMetadata().hardwareType);
	if (mf1 != mf2)
		return mf1.compare(mf2) < 0;

	// Then by name
	std::string name1 = Utils::String::toUpper(sys1->getName());
	std::string name2 = Utils::String::toUpper(sys2->getName());
	if (Settings::getInstance()->getBool("ForceFullNameSortSystems"))
	{
		name1 = Utils::String::toUpper(sys1->getFullName());
		name2 = Utils::String::toUpper(sys2->getFullName());
	}
	return name1.compare(name2) < 0;
}

bool systemByReleaseDate(SystemData* sys1, SystemData* sys2)
{
	// Order by hardware
	int mf1 = sys1->getSystemMetadata().releaseYear;
	int mf2 = sys2->getSystemMetadata().releaseYear;
	if (mf1 != mf2)
		return mf1 < mf2;

	// Move collection at Begin
	if (sys1->isCollection() != sys2->isCollection())
		return !sys2->isCollection();

	// Then by name
	std::string name1 = Utils::String::toUpper(sys1->getName());
	std::string name2 = Utils::String::toUpper(sys2->getName());
	if (Settings::getInstance()->getBool("ForceFullNameSortSystems"))
	{
		name1 = Utils::String::toUpper(sys1->getFullName());
		name2 = Utils::String::toUpper(sys2->getFullName());
	}
	return name1.compare(name2) < 0;
}

CollectionSystemManager* CollectionSystemManager::get()
{
	assert(sInstance);
	return sInstance;
}

void CollectionSystemManager::init(Window* window)
{
	deinit();
	sInstance = new CollectionSystemManager(window);
}

void CollectionSystemManager::deinit()
{
	if (sInstance)
	{
		delete sInstance;
		sInstance = nullptr;
	}
}

void CollectionSystemManager::saveCustomCollection(SystemData* sys)
{
	std::string name = sys->getName();	
	auto games = sys->getRootFolder()->getChildren();

	bool found = mCustomCollectionSystemsData.find(name) != mCustomCollectionSystemsData.cend();
	if (found) 
	{
		CollectionSystemData sysData = mCustomCollectionSystemsData.at(name);
		if (sysData.needsSave)
		{
			auto home = Utils::FileSystem::getHomePath();

			std::ofstream configFile;
			configFile.open(getCustomCollectionConfigPath(name));
			for(auto iter = games.cbegin(); iter != games.cend(); ++iter)
			{
				std::string path = (*iter)->getKey();

				path = Utils::FileSystem::createRelativePath(path, "portnawak", true);
				
				configFile << path << std::endl;
			}
			configFile.close();
		}
	}
	else
	{
		LOG(LogError) << "CollectionSystemManager::saveCustomCollection() - Couldn't find collection to save! " << name;
	}
}

/* Methods to load all Collections into memory, and handle enabling the active ones */
// loads all Collection Systems
void CollectionSystemManager::loadCollectionSystems(bool async)
{
	initAutoCollectionSystems();
	CollectionSystemDecl decl = mCollectionSystemDeclsIndex[myCollectionsName];
	mCustomCollectionsBundle = createNewCollectionEntry(decl.name, decl, false);
	// we will also load custom systems here
	initCustomCollectionSystems();
	if(Settings::getInstance()->getString("CollectionSystemsAuto") != "" || Settings::getInstance()->getString("CollectionSystemsCustom") != "")
	{
		// Now see which ones are enabled
		loadEnabledListFromSettings();

		
		// add to the main System Vector, and create Views as needed
		if (!async)
			updateSystemsList();
	}
}

// loads settings
void CollectionSystemManager::loadEnabledListFromSettings()
{
	// we parse the auto collection settings list
	std::vector<std::string> autoSelected = Utils::String::commaStringToVector(Settings::getInstance()->getString("CollectionSystemsAuto"));

	// iterate the map
	for(std::map<std::string, CollectionSystemData>::iterator it = mAutoCollectionSystemsData.begin() ; it != mAutoCollectionSystemsData.end() ; it++ )
	{
		it->second.isEnabled = (std::find(autoSelected.cbegin(), autoSelected.cend(), it->first) != autoSelected.cend());
	}

	// we parse the custom collection settings list
	std::vector<std::string> customSelected = Utils::String::commaStringToVector(Settings::getInstance()->getString("CollectionSystemsCustom"));

	// iterate the map
	for(std::map<std::string, CollectionSystemData>::iterator it = mCustomCollectionSystemsData.begin() ; it != mCustomCollectionSystemsData.end() ; it++ )
	{
		it->second.isEnabled = (std::find(customSelected.cbegin(), customSelected.cend(), it->first) != customSelected.cend());
	}
}

// updates enabled system list in System View
void CollectionSystemManager::updateSystemsList()
{
	auto sortMode = Settings::getInstance()->getString("SortSystems");
	bool sortByAlpha = SystemData::isManufacturerSupported() && sortMode == "alpha";
	bool sortByManufacturer = SystemData::isManufacturerSupported() && sortMode == "manufacturer";
	bool sortByHardware = SystemData::isManufacturerSupported() && sortMode == "hardware";
	bool sortByReleaseDate = SystemData::isManufacturerSupported() && sortMode == "releaseDate";

	// remove all Collection Systems
	removeCollectionsFromDisplayedSystems();

	std::unordered_map<std::string, FileData*> map;
	getAllGamesCollection()->getRootFolder()->createChildrenByFilenameMap(map);

	bool specailAlphSort = Settings::getInstance()->getBool("SpecialAlphaSort") && sortByAlpha;

	// add custom enabled ones
	addEnabledCollectionsToDisplayedSystems(&mCustomCollectionSystemsData, &map);

	if (sortByAlpha && !specailAlphSort)
		std::sort(SystemData::sSystemVector.begin(), SystemData::sSystemVector.end(), systemByAlphaSort);

	if (mCustomCollectionsBundle->getRootFolder()->getChildren().size() > 0)
		SystemData::sSystemVector.push_back(mCustomCollectionsBundle);

	// add auto enabled ones
	addEnabledCollectionsToDisplayedSystems(&mAutoCollectionSystemsData, &map);

	if (!sortMode.empty())
	{
		if (specailAlphSort)
			std::sort(SystemData::sSystemVector.begin(), SystemData::sSystemVector.end(), systemBySpecialAlphaSort);
		else if (sortByManufacturer)
			std::sort(SystemData::sSystemVector.begin(), SystemData::sSystemVector.end(), systemByManufacurerSort);
		else if (sortByHardware)
			std::sort(SystemData::sSystemVector.begin(), SystemData::sSystemVector.end(), systemByHardwareSort);
		else if (sortByReleaseDate)
			std::sort(SystemData::sSystemVector.begin(), SystemData::sSystemVector.end(), systemByReleaseDate);

		// Move RetroPie / Retrobat system to end
		for (auto sysIt = SystemData::sSystemVector.cbegin(); sysIt != SystemData::sSystemVector.cend(); )
		{
			if ((*sysIt)->getName() == "retropie" || (*sysIt)->getName() == "retrobat")
			{
				SystemData* retroPieSystem = (*sysIt);
				sysIt = SystemData::sSystemVector.erase(sysIt);
				SystemData::sSystemVector.push_back(retroPieSystem);
				break;
			}
			else
			{
				sysIt++;
			}
		}

	}

	// if we were editing a custom collection, and it's no longer enabled, exit edit mode
	if(mIsEditingCustom && !mEditingCollectionSystemData->isEnabled)
		exitEditMode();
}

/* Methods to manage collection files related to a source FileData */
// updates all collection files related to the source file
void CollectionSystemManager::refreshCollectionSystems(FileData* file)
{
	if (!file->getSystem()->isGameSystem() || file->getType() != GAME)
		return;

	std::map<std::string, CollectionSystemData> allCollections;
	allCollections.insert(mAutoCollectionSystemsData.cbegin(), mAutoCollectionSystemsData.cend());
	allCollections.insert(mCustomCollectionSystemsData.cbegin(), mCustomCollectionSystemsData.cend());

	for(auto sysDataIt = allCollections.cbegin(); sysDataIt != allCollections.cend(); sysDataIt++)
	{
		updateCollectionSystem(file, sysDataIt->second);
	}
}

void CollectionSystemManager::updateCollectionSystem(FileData* file, CollectionSystemData sysData)
{
	if (sysData.isPopulated)
	{
		// collection files use the full path as key, to avoid clashes
		std::string key = file->getFullPath();

		SystemData* curSys = sysData.system;	
		FileData* collectionEntry = curSys->getRootFolder()->FindByPath(key);

		FolderData* rootFolder = curSys->getRootFolder();
		
		std::string name = curSys->getName();

		if (collectionEntry != nullptr) 
		{
			// if we found it, we need to update it
			// remove from index, so we can re-index metadata after refreshing
			curSys->removeFromIndex(collectionEntry);
			collectionEntry->refreshMetadata();
			// found and we are removing
			if (name == "favorites" && !file->getFavorite()) {
				// need to check if still marked as favorite, if not remove
				ViewController::get()->getGameListView(curSys).get()->remove(collectionEntry);
				
				ViewController::get()->onFileChanged(file, FILE_METADATA_CHANGED);
				ViewController::get()->getGameListView(curSys)->onFileChanged(collectionEntry, FILE_METADATA_CHANGED);
			}
			else
			{
				// re-index with new metadata
				curSys->addToIndex(collectionEntry);
				ViewController::get()->onFileChanged(collectionEntry, FILE_METADATA_CHANGED);
			}
		}
		else
		{
			// we didn't find it here - we need to check if we should add it
			if (name == "recent" && file->getMetadata(MetaDataId::PlayCount) > "0" && includeFileInAutoCollections(file) ||
				name == "favorites" && file->getFavorite()) {
				CollectionFileData* newGame = new CollectionFileData(file, curSys);
				rootFolder->addChild(newGame);
				curSys->addToIndex(newGame);
				
				ViewController::get()->onFileChanged(file, FILE_METADATA_CHANGED);
				ViewController::get()->getGameListView(curSys)->onFileChanged(newGame, FILE_METADATA_CHANGED);
			}
		}

		curSys->updateDisplayedGameCount();

		if (name == "recent")
		{
			sortLastPlayed(curSys);
			trimCollectionCount(rootFolder, LAST_PLAYED_MAX);
			ViewController::get()->onFileChanged(rootFolder, FILE_METADATA_CHANGED);
		}
		else
			ViewController::get()->onFileChanged(rootFolder, FILE_SORTED);
	}
}

void CollectionSystemManager::sortLastPlayed(SystemData* system)
{
	if (system->getName() != "recent")
		return;
	
	FolderData* rootFolder = system->getRootFolder();
	system->setSortId(FileSorts::LASTPLAYED_DESCENDING);

	const FileSorts::SortType& sort = FileSorts::getSortTypes().at(system->getSortId());

	std::vector<FileData*>& childs = (std::vector<FileData*>&) rootFolder->getChildren();
	std::sort(childs.begin(), childs.end(), sort.comparisonFunction);
	if (!sort.ascending)
		std::reverse(childs.begin(), childs.end());
}

void CollectionSystemManager::trimCollectionCount(FolderData* rootFolder, int limit)
{
	SystemData* curSys = rootFolder->getSystem();
	std::shared_ptr<IGameListView> listView = ViewController::get()->getGameListView(curSys, false);

	auto& childs = rootFolder->getChildren();
	while ((int)childs.size() > limit)
	{
		CollectionFileData* gameToRemove = (CollectionFileData*)childs.back();
		if (listView == nullptr)
			delete gameToRemove;
		else
			listView.get()->remove(gameToRemove);
	}
}

// deletes all collection files from collection systems related to the source file
void CollectionSystemManager::deleteCollectionFiles(FileData* file)
{
	// collection files use the full path as key, to avoid clashes
	std::string key = file->getFullPath();

	// find games in collection systems
	std::map<std::string, CollectionSystemData> allCollections;
	allCollections.insert(mAutoCollectionSystemsData.cbegin(), mAutoCollectionSystemsData.cend());
	allCollections.insert(mCustomCollectionSystemsData.cbegin(), mCustomCollectionSystemsData.cend());

	for(auto sysDataIt = allCollections.begin(); sysDataIt != allCollections.end(); sysDataIt++)
	{
		if (!sysDataIt->second.isPopulated || !sysDataIt->second.isEnabled)
			continue;

		FileData* collectionEntry = (sysDataIt->second.system)->getRootFolder()->FindByPath(key);
		if (collectionEntry == nullptr)
			continue;

		sysDataIt->second.needsSave = true;

		SystemData* systemViewToUpdate = getSystemToView(sysDataIt->second.system);
		if (systemViewToUpdate == nullptr)
			continue;

		auto view = ViewController::get()->getGameListView(systemViewToUpdate);
		if (view != nullptr)
			view.get()->remove(collectionEntry);
		else
			delete collectionEntry;
	}
}

// returns whether the current theme is compatible with Automatic or Custom Collections
bool CollectionSystemManager::isThemeGenericCollectionCompatible(bool genericCustomCollections)
{
	std::vector<std::string> cfgSys = getCollectionThemeFolders(genericCustomCollections);
	for(auto sysIt = cfgSys.cbegin(); sysIt != cfgSys.cend(); sysIt++)
	{
		if(!themeFolderExists(*sysIt))
			return false;
	}
	return true;
}

bool CollectionSystemManager::isThemeCustomCollectionCompatible(std::vector<std::string> stringVector)
{
	if (isThemeGenericCollectionCompatible(true))
		return true;

	// get theme path
	auto themeSets = ThemeData::getThemeSets();
	auto set = themeSets.find(Settings::getInstance()->getString("ThemeSet"));
	if(set != themeSets.cend())
	{
		std::string defaultThemeFilePath = set->second.path + "/theme.xml";
		if (Utils::FileSystem::exists(defaultThemeFilePath))
		{
			return true;
		}
	}

	for(auto sysIt = stringVector.cbegin(); sysIt != stringVector.cend(); sysIt++)
	{
		if(!themeFolderExists(*sysIt))
			return false;
	}
	return true;
}

std::string CollectionSystemManager::getValidNewCollectionName(std::string inName, int index)
{
	std::string name = inName;

	if(index == 0)
	{
		size_t remove = std::string::npos;

		// get valid name
		while((remove = name.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-[]() ")) != std::string::npos)
		{
			name.erase(remove, 1);
		}
	}
	else
	{
		name += " (" + std::to_string(index) + ")";
	}

	if(name == "")
	{
		name = _("New Collection");
	}

	if(name != inName)
	{
		LOG(LogInfo) << "Had to change name, from: " << inName << " to: " << name;
	}

	// get used systems in es_systems.cfg
	std::vector<std::string> systemsInUse = getSystemsFromConfig();
	// get folders assigned to custom collections
	std::vector<std::string> autoSys = getCollectionThemeFolders(false);
	// get folder assigned to custom collections
	std::vector<std::string> customSys = getCollectionThemeFolders(true);
	// get folders assigned to user collections
	std::vector<std::string> userSys = getUserCollectionThemeFolders();
	// add them all to the list of systems in use
	systemsInUse.insert(systemsInUse.cend(), autoSys.cbegin(), autoSys.cend());
	systemsInUse.insert(systemsInUse.cend(), customSys.cbegin(), customSys.cend());
	systemsInUse.insert(systemsInUse.cend(), userSys.cbegin(), userSys.cend());
	for(auto sysIt = systemsInUse.cbegin(); sysIt != systemsInUse.cend(); sysIt++)
	{
		if (*sysIt == name)
		{
			if(index > 0) {
				name = name.substr(0, name.size()-4);
			}
			return getValidNewCollectionName(name, index+1);
		}
	}
	// if it matches one of the custom collections reserved names
	if (mCollectionSystemDeclsIndex.find(name) != mCollectionSystemDeclsIndex.cend())
		return getValidNewCollectionName(name, index+1);
	return name;
}

void CollectionSystemManager::setEditMode(std::string collectionName)
{
	if (mCustomCollectionSystemsData.find(collectionName) == mCustomCollectionSystemsData.cend())
	{
		LOG(LogError) << "CollectionSystemManager::setEditMode() - Tried to edit a non-existing collection: " << collectionName;
		return;
	}
	mIsEditingCustom = true;
	mEditingCollection = collectionName;

	CollectionSystemData* sysData = &(mCustomCollectionSystemsData.at(mEditingCollection));
	if (!sysData->isPopulated)
		populateCustomCollection(sysData);

	// if it's bundled, this needs to be the bundle system
	mEditingCollectionSystemData = sysData;

	std::string col_name = collectionName;
	if (collectionName == "Favorites")
		col_name = _("Favorites");

	char strbuf[128];
	snprintf(strbuf, 128, _("Editing the '%s' Collection. Add/remove games with 'Y'.").c_str(), Utils::String::toUpper(col_name).c_str());
	mWindow->displayNotificationMessage(strbuf, 10000);
}

void CollectionSystemManager::exitEditMode()
{
	std::string col_name = mEditingCollection;
	if (mEditingCollection == "Favorites")
		col_name = _("Favorites");

	char strbuf[128];
	snprintf(strbuf, 128, _("Finished editing the '%s' Collection.").c_str(), col_name.c_str());
	mWindow->displayNotificationMessage(strbuf, 10000);
	mIsEditingCustom = false;
	mEditingCollection = "Favorites";
}

// adds or removes a game from a specific collection
bool CollectionSystemManager::toggleGameInCollection(FileData* file)
{
	if (file->getType() == GAME)
	{
		GuiInfoPopup* s;
		bool adding = true;
		std::string name = file->getName();
		std::string sysName = mEditingCollection;
		if (mIsEditingCustom)
		{
			SystemData* sysData = mEditingCollectionSystemData->system;
			mEditingCollectionSystemData->needsSave = true;
			if (!mEditingCollectionSystemData->isPopulated)
				populateCustomCollection(mEditingCollectionSystemData);

			std::string key = file->getFullPath();
			FolderData* rootFolder = sysData->getRootFolder();

			FileData* collectionEntry = rootFolder->FindByPath(key);
			
			std::string name = sysData->getName();

			SystemData* systemViewToUpdate = getSystemToView(sysData);

			if (collectionEntry != nullptr) {
				adding = false;
				// if we found it, we need to remove it				
				// remove from index
				sysData->removeFromIndex(collectionEntry);
				// remove from bundle index as well, if needed
				if (systemViewToUpdate != sysData)
					systemViewToUpdate->removeFromIndex(collectionEntry);

				ViewController::get()->getGameListView(systemViewToUpdate).get()->remove(collectionEntry);
			}
			else
			{
				// we didn't find it here, we should add it
				CollectionFileData* newGame = new CollectionFileData(file, sysData);
				rootFolder->addChild(newGame);
				sysData->addToIndex(newGame);
				ViewController::get()->getGameListView(systemViewToUpdate)->onFileChanged(newGame, FILE_METADATA_CHANGED);
				ViewController::get()->onFileChanged(systemViewToUpdate->getRootFolder(), FILE_SORTED);
				// add to bundle index as well, if needed
				if(systemViewToUpdate != sysData)
				{
					systemViewToUpdate->addToIndex(newGame);
				}
			}
			updateCollectionFolderMetadata(sysData);
		}
		else
		{
			SystemData* sysData = file->getSourceFileData()->getSystem();
			sysData->removeFromIndex(file);

			MetaDataList* md = &file->getSourceFileData()->getMetadata();

			std::string value = md->get(MetaDataId::Favorite);
			if (value == "false")
				md->set(MetaDataId::Favorite, "true");
			else
			{
				adding = false;
				md->set(MetaDataId::Favorite, "false");
			}
			sysData->addToIndex(file);
			saveToGamelistRecovery(file);

			refreshCollectionSystems(file->getSourceFileData());

			SystemData* systemViewToUpdate = getSystemToView(sysData);
			if (systemViewToUpdate != NULL)
			{
				ViewController::get()->onFileChanged(file, FILE_METADATA_CHANGED);
				ViewController::get()->getGameListView(systemViewToUpdate)->onFileChanged(file, FILE_METADATA_CHANGED);
			}

			
		}

		char trstring[512];

		std::string sys_name = sysName;
		if (sys_name == "Favorites")
			sys_name = _("Favorites");

		if (adding)
			snprintf(trstring, 512, _("Added '%s' to '%s'").c_str(), Utils::String::removeParenthesis(name).c_str(), Utils::String::toUpper(sys_name).c_str()); // batocera
		else
			snprintf(trstring, 512, _("Removed '%s' from '%s'").c_str(), Utils::String::removeParenthesis(name).c_str(), Utils::String::toUpper(sys_name).c_str()); // batocera

		mWindow->displayNotificationMessage(trstring, 4000);

		return true;
	}
	return false;
}

SystemData* CollectionSystemManager::getSystemToView(SystemData* sys)
{
	SystemData* systemToView = sys;
	FileData* rootFolder = sys->getRootFolder();

	FolderData* bundleRootFolder = mCustomCollectionsBundle->getRootFolder();

	// is the rootFolder bundled in the "My Collections" system?
	bool sysFoundInBundle = bundleRootFolder->FindByPath(rootFolder->getKey()) != nullptr;
	if (sysFoundInBundle && sys->isCollection())
		systemToView = mCustomCollectionsBundle;
	else if (sys->isGroupChildSystem())
		systemToView = sys->getParentGroupSystem();

	return systemToView;
}

/* Handles loading a collection system, creating an empty one, and populating on demand */
// loads Automatic Collection systems (All, Favorites, Last Played)
void CollectionSystemManager::initAutoCollectionSystems()
{
	for(std::map<std::string, CollectionSystemDecl>::const_iterator it = mCollectionSystemDeclsIndex.cbegin() ; it != mCollectionSystemDeclsIndex.cend() ; it++ )
	{
		CollectionSystemDecl sysDecl = it->second;
		if (!sysDecl.isCustom)
		{
			createNewCollectionEntry(sysDecl.name, sysDecl);
		}
	}
}

// this may come in handy if at any point in time in the future we want to
// automatically generate metadata for a folder
void CollectionSystemManager::updateCollectionFolderMetadata(SystemData* sys)
{
	FolderData* rootFolder = sys->getRootFolder();

	std::string desc = _("This collection is empty.");
	std::string rating = "0";
	std::string players = "1";
	std::string releasedate = "N/A";
	std::string developer = _("None");
	std::string genre = _("None");
	std::string video = "";
	std::string thumbnail = "";
	std::string image = "";

	auto games = rootFolder->getChildren();
	char trstring[512];

	if(games.size() > 0)
	{
		std::string games_list = "";
		int games_counter = 0;
		for(auto iter = games.cbegin(); iter != games.cend(); ++iter)
		{
			games_counter++;
			FileData* file = *iter;

			std::string new_rating = file->getMetadata(MetaDataId::Rating);
			std::string new_releasedate = file->getMetadata(MetaDataId::ReleaseDate);
			std::string new_developer = file->getMetadata(MetaDataId::Developer);
			std::string new_genre = file->getMetadata(MetaDataId::Genre);
			std::string new_players = file->getMetadata(MetaDataId::Players);

			rating = (new_rating > rating ? (new_rating != "" ? new_rating : rating) : rating);
			players = (new_players > players ? (new_players != "" ? new_players : players) : players);
			releasedate = (new_releasedate < releasedate ? (new_releasedate != "" ? new_releasedate : releasedate) : releasedate);
			developer = (developer == _("None") ? new_developer : (new_developer != developer ? _("Various") : new_developer));
			genre = (genre == _("None") ? new_genre : (new_genre != genre ? _("Various") : new_genre));

			switch(games_counter)
			{
				case 2:
				case 3:
					games_list += ", ";
				case 1:
					games_list += "'" + file->getName() + "'";
					break;
				case 4:
					games_list += " " + _("among other titles.");
			}
		}

			
		games_counter = games.size();

		snprintf(trstring, 512, EsLocale::nGetText(
			"This collection contains %i game, including :%s",
			"This collection contains %i games, including :%s", games_counter).c_str(), games_counter, games_list.c_str());

		desc = trstring;

		FileData* randomGame = sys->getRandomGame();
		if (randomGame != nullptr)
		{
			video = randomGame->getVideoPath();
			thumbnail = randomGame->getThumbnailPath();
			image = randomGame->getImagePath();
		}
	}


	rootFolder->setMetadata(MetaDataId::Desc, desc);
	rootFolder->setMetadata(MetaDataId::Rating, rating);
	rootFolder->setMetadata(MetaDataId::Players, players);
	rootFolder->setMetadata(MetaDataId::Genre, genre);
	rootFolder->setMetadata(MetaDataId::ReleaseDate, releasedate);
	rootFolder->setMetadata(MetaDataId::Developer, developer);
	rootFolder->setMetadata(MetaDataId::Video, video);
	rootFolder->setMetadata(MetaDataId::Thumbnail, thumbnail);
	rootFolder->setMetadata(MetaDataId::Image, image);
	rootFolder->setMetadata(MetaDataId::KidGame, "false");
	rootFolder->setMetadata(MetaDataId::Hidden, "false");
	rootFolder->setMetadata(MetaDataId::Favorite, "false");

	rootFolder->getMetadata().resetChangedFlag();
}

void CollectionSystemManager::initCustomCollectionSystems()
{
	for (auto name : getCollectionsFromConfigFolder())
		addNewCustomCollection(name, Settings::getInstance()->getString("custom-" + name + ".fullname"), false);
}

SystemData* CollectionSystemManager::getArcadeCollection()
{
	CollectionSystemData* allSysData = &mAutoCollectionSystemsData["arcade"];
	if (!allSysData->isPopulated)
		populateAutoCollection(allSysData);

	return allSysData->system;
}

SystemData* CollectionSystemManager::getAllGamesCollection()
{
	CollectionSystemData* allSysData = &mAutoCollectionSystemsData["all"];
	if (!allSysData->isPopulated)
	{
		populateAutoCollection(allSysData);
	}
	return allSysData->system;
}

SystemData* CollectionSystemManager::addNewCustomCollection(std::string name, std::string longName, bool needSave)
{
	CollectionSystemDecl decl = mCollectionSystemDeclsIndex[myCollectionsName];
	decl.themeFolder = name;
	decl.name = name;
	decl.longName = name;
	if (!longName.empty())
		decl.longName = longName;

	return createNewCollectionEntry(name, decl, true, needSave);
}

// creates a new, empty Collection system, based on the name and declaration
SystemData* CollectionSystemManager::createNewCollectionEntry(std::string name, CollectionSystemDecl sysDecl, bool index, bool needSave)
{
	SystemMetadata md;
	md.name = name;
	md.fullName = sysDecl.longName;
	md.themeFolder = sysDecl.themeFolder;
	md.manufacturer = "Collections";
	md.hardwareType = sysDecl.isCustom ? "custom collection" : "auto collection";
	md.releaseYear = 0;

	// we parse the auto collection settings list
	std::vector<std::string> selected = Utils::String::split(Settings::getInstance()->getString(sysDecl.isCustom ? "CollectionSystemsCustom" : "CollectionSystemsAuto"), ',', true);
	bool loadThemeIfEnabled = (name == myCollectionsName || (std::find(selected.cbegin(), selected.cend(), name) != selected.cend()));

	SystemData* newSys = new SystemData(md, mCollectionEnvData, true, false, loadThemeIfEnabled);

	CollectionSystemData newCollectionData;
	newCollectionData.system = newSys;
	newCollectionData.decl = sysDecl;
	newCollectionData.isEnabled = false;
	newCollectionData.isPopulated = false;
	newCollectionData.needsSave = false;

	if (index)
	{
		if (!sysDecl.isCustom)
			mAutoCollectionSystemsData[name] = newCollectionData;
		else
			mCustomCollectionSystemsData[name] = newCollectionData;
	}

	return newSys;
}

// populates an Automatic Collection System
void CollectionSystemManager::populateAutoCollection(CollectionSystemData* sysData)
{
	SystemData* newSys = sysData->system;
	CollectionSystemDecl sysDecl = sysData->decl;
	FolderData* rootFolder = newSys->getRootFolder();

	auto hiddenSystems = Utils::String::split(Settings::getInstance()->getString("HiddenSystems"), ';');

	for (auto& system : SystemData::sSystemVector)
	{
		// we won't iterate all collections
		if (!system->isGameSystem() || system->isCollection())
			continue;

		if (std::find(hiddenSystems.cbegin(), hiddenSystems.cend(), system->getName()) != hiddenSystems.cend())
			continue;

		std::vector<PlatformIds::PlatformId> platforms = system->getPlatformIds();
		bool isArcade = std::find(platforms.begin(), platforms.end(), PlatformIds::ARCADE) != platforms.end();


		std::vector<FileData*> files = system->getRootFolder()->getFilesRecursive(GAME);
		for (auto& game : files)
		{
			std::string systemarcadename;

			bool include = includeFileInAutoCollections(game);
			if (!include)
				continue;

			switch(sysDecl.type)
			{
				case AUTO_ALL_GAMES:
					break;
				case AUTO_VERTICALARCADE: // batocera
					include = game->isVerticalArcadeGame();
					break;
				case AUTO_LAST_PLAYED:
					include = game->getMetadata(MetaDataId::PlayCount) > "0";
					break;
				case AUTO_NEVER_PLAYED:
					include = !(game->getMetadata(MetaDataId::PlayCount) > "0");
					break;
				case AUTO_FAVORITES:
					// we may still want to add files we don't want in auto collections in "favorites"
					include = game->getFavorite();
					break;
				case AUTO_ARCADE:
					include = isArcade;
					break;
				case CPS1_COLLECTION:
					systemarcadename = "cps1";
					break;
				case CPS2_COLLECTION:
					systemarcadename = "cps2";
					break;
				case CPS3_COLLECTION:
					systemarcadename = "cps3";
					break;
				case CAVE_COLLECTION:
					systemarcadename = "cave";
					break;
				case NEOGEO_COLLECTION:
					systemarcadename = "neogeo";
					break;
				case SEGA_COLLECTION:
					systemarcadename = "sega";
					break;
				case IREM_COLLECTION:
					systemarcadename = "irem";
					break;
				case MIDWAY_COLLECTION:
					systemarcadename = "midway";
					break;
				case CAPCOM_COLLECTION:
					systemarcadename = "capcom";
					break;
				case TECMO_COLLECTION:
					systemarcadename = "techmo";
					break;
				case SNK_COLLECTION:
					systemarcadename = "snk";
					break;
				case NAMCO_COLLECTION:
					systemarcadename = "namco";
					break;
				case TAITO_COLLECTION:
					systemarcadename = "taito";
					break;
				case KONAMI_COLLECTION:
					systemarcadename = "konami";
					break;
				case JALECO_COLLECTION:
					systemarcadename = "jaleco";
					break;
				case ATARI_COLLECTION:
					systemarcadename = "atari";
					break;
				case NINTENDO_COLLECTION:
					systemarcadename = "nintendo";
					break;
				case SAMMY_COLLECTION:
					systemarcadename = "sammy";
					break;
				case ACCLAIM_COLLECTION:
					systemarcadename = "acclaim";
					break;
				case PSIKYO_COLLECTION:
					systemarcadename = "psikyo";
					break;
				case KANEKO_COLLECTION:
					systemarcadename = "kaneko";
					break;
				case COLECO_COLLECTION:
					systemarcadename = "coleco";
					break;
				case ATLUS_COLLECTION:
					systemarcadename = "atlus";
					break;
				case BANPRESTO_COLLECTION:
					systemarcadename = "banpresto";
					break;
				case AUTO_AT2PLAYERS:
				case AUTO_AT4PLAYERS:
				{
					std::string players = game->getMetadata(MetaDataId::Players);
					if (players.empty())
						include = false;
					else
					{
						int min = -1;
						auto split = players.rfind("+");
						if (split != std::string::npos)
							players = Utils::String::replace(players, "+", "-999");

						split = players.rfind("-");
						if (split != std::string::npos)
						{
							min = atoi(players.substr(0, split).c_str());
							players = players.substr(split + 1);
						}

						int max = atoi(players.c_str());
						int val = (sysDecl.type == AUTO_AT2PLAYERS ? 2 : 4);
						include = min <= 0 ? (val == max) : (min <= val && val <= max);
					}
				}
				break;
			}

			if (!systemarcadename.empty())
				include = isArcade && game->getMetadata(MetaDataId::ArcadeSystemName) == systemarcadename;

			if (include)
			{
				CollectionFileData* newGame = new CollectionFileData(game, newSys);
				rootFolder->addChild(newGame);
				newSys->addToIndex(newGame);
			}
		} // end FOR games

	} // end FOR systems

	if (sysDecl.type == AUTO_LAST_PLAYED)
	{
		sortLastPlayed(newSys);
		trimCollectionCount(rootFolder, LAST_PLAYED_MAX);
	}

	sysData->isPopulated = true;
}

// populates a Custom Collection System
void CollectionSystemManager::populateCustomCollection(CollectionSystemData* sysData, std::unordered_map<std::string, FileData*>* pMap)
{
	SystemData* newSys = sysData->system;
	sysData->isPopulated = true;
	CollectionSystemDecl sysDecl = sysData->decl;
	auto hiddenSystems = Utils::String::split(Settings::getInstance()->getString("HiddenSystems"), ';');

	std::string path = getCustomCollectionConfigPath(newSys->getName());

	if(!Utils::FileSystem::exists(path))
	{
		LOG(LogInfo) << "Couldn't find custom collection config file at " << path;
		return;
	}
	LOG(LogInfo) << "Loading custom collection config file at " << path;

	FolderData* rootFolder = newSys->getRootFolder();
	
	// get Configuration for this Custom System
	std::ifstream input(path);

	FolderData* folder = getAllGamesCollection()->getRootFolder();

	std::unordered_map<std::string, FileData*> map;

	if (pMap == nullptr)
	{
		folder->createChildrenByFilenameMap(map);
		pMap = &map;
	}

	// iterate list of files in config file
	for(std::string gameKey; getline(input, gameKey); )
	{
		if (gameKey.empty() || gameKey[0] == '0' || gameKey[0] == '#')
			continue;

		// if item is portable relative to homepath
		gameKey = Utils::FileSystem::resolveRelativePath(Utils::String::trim(gameKey), "portnawak", true);

		std::unordered_map<std::string, FileData*>::const_iterator it = pMap->find(gameKey);
		if (it != pMap->cend())
		{
			if (std::find(hiddenSystems.cbegin(), hiddenSystems.cend(), it->second->getName()) != hiddenSystems.cend())
				continue;

			CollectionFileData* newGame = new CollectionFileData(it->second, newSys);
			rootFolder->addChild(newGame);
			newSys->addToIndex(newGame);
		}
		else
			LOG(LogInfo) << "CollectionSystemManager::populateCustomCollection() - Couldn't find game referenced at '" << gameKey << "' for system config '" << path << "'";
	}
	updateCollectionFolderMetadata(newSys);
}

/* Handle System View removal and insertion of Collections */
void CollectionSystemManager::removeCollectionsFromDisplayedSystems()
{
	// remove all Collection Systems
	for(auto sysIt = SystemData::sSystemVector.cbegin(); sysIt != SystemData::sSystemVector.cend(); )
	{
		if ((*sysIt)->isCollection())
			sysIt = SystemData::sSystemVector.erase(sysIt);
		else
			sysIt++;
	}

	if (mCustomCollectionsBundle == nullptr)
		return;

	// remove all custom collections in bundle
	// this should not delete the objects from memory!
	FolderData* customRoot = mCustomCollectionsBundle->getRootFolder();
	std::vector<FileData*> mChildren = customRoot->getChildren();
	for(auto it = mChildren.cbegin(); it != mChildren.cend(); it++)
		customRoot->removeChild(*it);

	// clear index
	mCustomCollectionsBundle->resetIndex();
	// remove view so it's re-created as needed
	ViewController::get()->removeGameListView(mCustomCollectionsBundle);
}

void CollectionSystemManager::addEnabledCollectionsToDisplayedSystems(std::map<std::string, CollectionSystemData>* colSystemData, std::unordered_map<std::string, FileData*>* pMap)
{
	if (Utils::Async::isCanRunAsync())
	{
		LOG(LogInfo) << "CollectionSystemManager::addEnabledCollectionsToDisplayedSystems() - Collection threaded loading";
		std::vector<CollectionSystemData*> collectionsToPopulate;
		for (auto it = colSystemData->begin(); it != colSystemData->end(); it++)
			if (it->second.isEnabled && !it->second.isPopulated)
				collectionsToPopulate.push_back(&(it->second));

		if (collectionsToPopulate.size() > 1)
		{
			getAllGamesCollection();

			Utils::ThreadPool pool;

			for (auto collection : collectionsToPopulate)
			{
				if (collection->decl.isCustom)
					pool.queueWorkItem([this, collection, pMap] { populateCustomCollection(collection, pMap); });
				else
					pool.queueWorkItem([this, collection, pMap] { populateAutoCollection(collection); });
			}

			pool.wait();
		}
	}
	// add auto enabled ones
	for(std::map<std::string, CollectionSystemData>::iterator it = colSystemData->begin() ; it != colSystemData->end() ; it++ )
	{
		if(!it->second.isEnabled)
			continue;

		// check if populated, otherwise populate
		if (!it->second.isPopulated)
		{
			if(it->second.decl.isCustom)
				populateCustomCollection(&(it->second), pMap);
			else
				populateAutoCollection(&(it->second));
		}
		// check if it has its own view
		if(!it->second.decl.isCustom || themeFolderExists(it->first) || !Settings::getInstance()->getBool("UseCustomCollectionsSystem"))
		{
			if (it->second.decl.displayIfEmpty || it->second.system->getRootFolder()->getChildren().size() > 0)
			{
				// exists theme folder, or we chose not to bundle it under the custom-collections system
				// so we need to create a view
				if (it->second.isEnabled)
					SystemData::sSystemVector.push_back(it->second.system);
			}
		}
		else
		{
			FileData* newSysRootFolder = it->second.system->getRootFolder();
			mCustomCollectionsBundle->getRootFolder()->addChild(newSysRootFolder);

			//mCustomCollectionsBundle->getIndex(true)->importIndex(it->second.system->getIndex(true));
			auto idx = it->second.system->getIndex(false);
			if (idx != nullptr)
				mCustomCollectionsBundle->getIndex(true)->importIndex(idx);
		}
	}
}

/* Auxiliary methods to get available custom collection possibilities */
std::vector<std::string> CollectionSystemManager::getSystemsFromConfig()
{
	std::vector<std::string> systems;
	std::string path = SystemData::getConfigPath(false);

	if(!Utils::FileSystem::exists(path))
	{
		return systems;
	}

	pugi::xml_document doc;
	pugi::xml_parse_result res = doc.load_file(path.c_str());

	if(!res)
	{
		return systems;
	}

	//actually read the file
	pugi::xml_node systemList = doc.child("systemList");

	if(!systemList)
	{
		return systems;
	}

	for(pugi::xml_node system = systemList.child("system"); system; system = system.next_sibling("system"))
	{
		// theme folder
		std::string themeFolder = system.child("theme").text().get();
		systems.push_back(themeFolder);
	}
	std::sort(systems.begin(), systems.end());
	return systems;
}

// gets all folders from the current theme path
std::vector<std::string> CollectionSystemManager::getSystemsFromTheme()
{
	std::vector<std::string> systems;

	auto themeSets = ThemeData::getThemeSets();
	if(themeSets.empty())
	{
		// no theme sets available
		return systems;
	}

	std::map<std::string, ThemeSet>::const_iterator set = themeSets.find(Settings::getInstance()->getString("ThemeSet"));
	if(set == themeSets.cend())
	{
		// currently selected theme set is missing, so just pick the first available set
		set = themeSets.cbegin();
		Settings::getInstance()->setString("ThemeSet", set->first);
	}

	std::string themePath = set->second.path;

	if (Utils::FileSystem::exists(themePath))
	{
		Utils::FileSystem::stringList dirContent = Utils::FileSystem::getDirContent(themePath);

		for (Utils::FileSystem::stringList::const_iterator it = dirContent.cbegin(); it != dirContent.cend(); ++it)
		{
			if (Utils::FileSystem::isDirectory(*it))
			{
				//... here you have a directory
				std::string folder = *it;
				folder = folder.substr(themePath.size()+1);

				if(Utils::FileSystem::exists(set->second.getThemePath(folder)))
				{
					systems.push_back(folder);
				}
			}
		}
	}
	std::sort(systems.begin(), systems.end());
	return systems;
}

// returns the unused folders from current theme path
std::vector<std::string> CollectionSystemManager::getUnusedSystemsFromTheme()
{
	// get used systems in es_systems.cfg
	std::vector<std::string> systemsInUse = getSystemsFromConfig();
	// get available folders in theme
	std::vector<std::string> themeSys = getSystemsFromTheme();
	// get folders assigned to custom collections
	std::vector<std::string> autoSys = getCollectionThemeFolders(false);
	// get folder assigned to custom collections
	std::vector<std::string> customSys = getCollectionThemeFolders(true);
	// get folders assigned to user collections
	std::vector<std::string> userSys = getUserCollectionThemeFolders();
	// add them all to the list of systems in use
	systemsInUse.insert(systemsInUse.cend(), autoSys.cbegin(), autoSys.cend());
	systemsInUse.insert(systemsInUse.cend(), customSys.cbegin(), customSys.cend());
	systemsInUse.insert(systemsInUse.cend(), userSys.cbegin(), userSys.cend());

	for(auto sysIt = themeSys.cbegin(); sysIt != themeSys.cend(); )
	{
		if (std::find(systemsInUse.cbegin(), systemsInUse.cend(), *sysIt) != systemsInUse.cend())
			sysIt = themeSys.erase(sysIt);
		else
			sysIt++;
	}
	return themeSys;
}

// returns which collection config files exist in the user folder
std::vector<std::string> CollectionSystemManager::getCollectionsFromConfigFolder()
{
	std::vector<std::string> systems;
	std::string configPath = getCollectionsFolder();

	LOG(LogInfo) << "CollectionSystemManager::getCollectionsFromConfigFolder() - Loading collections folder '" << configPath << "'...";

	if (Utils::FileSystem::exists(configPath))
	{
		Utils::FileSystem::stringList dirContent = Utils::FileSystem::getDirContent(configPath);
		for (Utils::FileSystem::stringList::const_iterator it = dirContent.cbegin(); it != dirContent.cend(); ++it)
		{
			if (Utils::FileSystem::isRegularFile(*it))
			{
				// it's a file
				std::string filename = Utils::FileSystem::getFileName(*it);

				// need to confirm filename matches config format
				if (filename != "custom-.cfg" && Utils::String::startsWith(filename, "custom-") && Utils::String::endsWith(filename, ".cfg"))
				{
					filename = filename.substr(7, filename.size()-11);
					systems.push_back(filename);
				}
				else
				{
					LOG(LogInfo) << "CollectionSystemManager::getCollectionsFromConfigFolder() - Found non-collection config file in collections folder: " << filename;
				}
			}
		}
	}
	return systems;
}

// returns the theme folders for Automatic Collections (All, Favorites, Last Played) or generic Custom Collections folder
std::vector<std::string> CollectionSystemManager::getCollectionThemeFolders(bool custom)
{
	std::vector<std::string> systems;
	for(std::map<std::string, CollectionSystemDecl>::const_iterator it = mCollectionSystemDeclsIndex.cbegin() ; it != mCollectionSystemDeclsIndex.cend() ; it++ )
	{
		CollectionSystemDecl sysDecl = it->second;
		if (sysDecl.isCustom == custom)
		{
			systems.push_back(sysDecl.themeFolder);
		}
	}
	return systems;
}

// returns the theme folders in use for the user-defined Custom Collections
std::vector<std::string> CollectionSystemManager::getUserCollectionThemeFolders()
{
	std::vector<std::string> systems;
	for(std::map<std::string, CollectionSystemData>::const_iterator it = mCustomCollectionSystemsData.cbegin() ; it != mCustomCollectionSystemsData.cend() ; it++ )
	{
		systems.push_back(it->second.decl.themeFolder);
	}
	return systems;
}

// returns whether a specific folder exists in the theme
bool CollectionSystemManager::themeFolderExists(std::string folder)
{
	std::vector<std::string> themeSys = getSystemsFromTheme();
	return std::find(themeSys.cbegin(), themeSys.cend(), folder) != themeSys.cend();
}

bool CollectionSystemManager::includeFileInAutoCollections(FileData* file)
{
	// if/when there are more in the future, maybe this can be a more complex method, with a proper list
	// but for now a simple string comparison is more performant
	return file->getSystem()->isGameSystem() && !file->getSystem()->hasPlatformId(PlatformIds::PLATFORM_IGNORE);
}

std::string getCustomCollectionConfigPath(std::string collectionName)
{
	return getCollectionsFolder() + "/custom-" + collectionName + ".cfg";
}

std::string getCollectionsFolder()
{
	return Utils::FileSystem::getGenericPath(Utils::FileSystem::getEsConfigPath() + "/collections");
}
