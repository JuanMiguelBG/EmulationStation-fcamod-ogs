#include "Gamelist.h"

#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"
#include "FileData.h"
#include "FileFilterIndex.h"
#include "Log.h"
#include "Settings.h"
#include "SystemData.h"
#include <pugixml/src/pugixml.hpp>

#include <unistd.h>

#include <future>
#include "utils/AsyncUtil.h"

FileData* findOrCreateFile(SystemData* system, const std::string& path, FileType type, std::unordered_map<std::string, FileData*>& fileMap)
{
	auto pGame = fileMap.find(path);
	if (pGame != fileMap.end())
		return pGame->second;

	// first, verify that path is within the system's root folder
	FolderData* root = system->getRootFolder();

	bool contains = false;
	std::string relative = Utils::FileSystem::removeCommonPath(path, root->getPath(), contains);

	if(!contains)
	{
		LOG(LogError) << "Gamelist::findOrCreateFile() - File path \"" << path << "\" is outside system path \"" << system->getStartPath() << "\"";
		return NULL;
	}

	Utils::FileSystem::stringList pathList = Utils::FileSystem::getPathList(relative);
	auto path_it = pathList.begin();
	FolderData* treeNode = root;

	//	bool found = false;
	while(path_it != pathList.end())
	{
		std::string key = Utils::FileSystem::combine(treeNode->getPath(), *path_it);	
		FileData* item = (fileMap.find(key) != fileMap.end()) ? fileMap[key] : nullptr;
		if (item != nullptr)
		{
			if (item->getType() == FOLDER)
				treeNode = (FolderData*) item;
			else
				return item;
		}
		
		// this is the end
		if(path_it == --pathList.end())
		{
			if (type == FOLDER)
			{
				LOG(LogWarning) << "Gamelist::findOrCreateFile() - gameList: folder doesn't already exist, won't create";
				return NULL;
			}

			if (type == GAME) // Final file
			{
				// Skip if the extension in the gamelist is unknown
				if (!system->getSystemEnvData()->isValidExtension(Utils::String::toLower(Utils::FileSystem::getExtension(path))))
				{
					LOG(LogWarning) << "Gamelist::findOrCreateFile() - gameList: file extension is not known by systemlist";
					return NULL;
				}

				// Add final game
				item = new FileData(GAME, path, system);
				if (!item->isArcadeAsset())
				{
					fileMap[key] = item;
					treeNode->addChild(item);
				}

				return item;
			}
		}
		
		if (item == nullptr)
		{
			// don't create folders unless it's leading up to a game
			// if type is a folder it's gonna be empty, so don't bother
			if(type == FOLDER)
			{
				LOG(LogWarning) << "Gamelist::findOrCreateFile() - gameList: folder doesn't already exist, won't create";
				return NULL;
			}

			// create missing folder
			FolderData* folder = new FolderData(Utils::FileSystem::getStem(treeNode->getPath()) + "/" + *path_it, system);
			fileMap[key] = folder;
			treeNode->addChild(folder);
			treeNode = folder;
		}
		
		path_it++;
	}

	return NULL;
}

void loadGamelistFile (const std::string xmlpath, SystemData* system, std::unordered_map<std::string, FileData*>& fileMap, size_t checkSize = SIZE_MAX)
{
	LOG(LogInfo) << "Gamelist::loadGamelistFile() - Parsing XML file \"" << xmlpath << "\"...";

	bool trustGamelist = Settings::getInstance()->getBool("ParseGamelistOnly");

	if ( !Utils::String::endsWith(xmlpath, ".xml") )
	{
		LOG(LogWarning) << "Gamelist::loadGamelistFile() - file \"" << xmlpath << "\" isn't a XML file, skipped!";
		return;
	}

	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(xmlpath.c_str());

	if (!result)
	{
		LOG(LogError) << "Gamelist::loadGamelistFile() - Error parsing XML file \"" << xmlpath << "\"!\n	" << result.description();
		return;
	}

	pugi::xml_node root = doc.child("gameList");
	if (!root)
	{
		LOG(LogError) << "Gamelist::loadGamelistFile() - Could not find <gameList> node in gamelist \"" << xmlpath << "\"!";
		return;
	}

	if (checkSize != SIZE_MAX)
	{
		auto parentSize = root.attribute("parentHash").as_uint();
		if (parentSize != checkSize)
		{
			LOG(LogWarning) << "Gamelist::loadGamelistFile() - gamelist size don't match !";
			return;
		}
	}

	std::string relativeTo = system->getStartPath();

	for (pugi::xml_node fileNode : root.children())
	{
		FileType type = GAME;

		std::string tag = fileNode.name();
		if (tag == "folder")
			type = FOLDER;
		else if (tag != "game")
			continue;

		const std::string path = Utils::FileSystem::resolveRelativePath(fileNode.child("path").text().get(), relativeTo, false);
		if (!trustGamelist && !Utils::FileSystem::exists(path))
		{
			LOG(LogWarning) << "Gamelist::loadGamelistFile() - File \"" << path << "\" does not exist! Ignoring.";
			continue;
		}

		FileData* file = findOrCreateFile(system, path, type, fileMap);
		if (!file)
		{
			LOG(LogError) << "Gamelist::loadGamelistFile() - Error finding/creating FileData for \"" << path << "\", skipping.";
			continue;
		}
		else if (!file->isArcadeAsset())
		{
			std::string defaultName = file->getMetadata(MetaDataId::Name);

			file->setMetadata(MetaDataList::createFromXML(type == FOLDER ? FOLDER_METADATA : GAME_METADATA, fileNode, system));
			file->getMetadata().migrate(file, fileNode);

			//make sure name gets set if one didn't exist
			if (file->getMetadata(MetaDataId::Name).empty())
				file->setMetadata(MetaDataId::Name, defaultName);

			if (!trustGamelist && !file->getHidden() && Utils::FileSystem::isHidden(path))
				file->setMetadata(MetaDataId::Hidden, "true");

			if (checkSize != SIZE_MAX)
				file->getMetadata().setDirty();
			else
				file->getMetadata().resetChangedFlag();
		}
	}
}

std::string getTemporaryGamelistRecovery(SystemData* system)
{
	return Utils::FileSystem::getEsConfigPath() + "/recovery/" + system->getName();
}

void clearTemporaryGamelistRecovery(SystemData* system)
{	
	auto path = getTemporaryGamelistRecovery(system);

	auto files = Utils::FileSystem::getDirContent(path, true, false);
	if (files.size() > 0)
	{
		for (auto file : files)
			if (!Utils::FileSystem::isDirectory(file))
				Utils::FileSystem::removeFile(file);

		std::reverse(std::begin(files), std::end(files));

		for (auto file : files)
			if (Utils::FileSystem::isDirectory(file))
				rmdir(file.c_str());
	}

	rmdir(path.c_str());
}

void parseGamelist(SystemData* system, std::unordered_map<std::string, FileData*>& fileMap)
{
	std::string xmlpath = system->getGamelistPath(false);

	LOG(LogInfo) << "GameList::parseGamelist() - system: " << system->getName() << ", path: " << xmlpath;

	auto size = Utils::FileSystem::getFileSize(xmlpath);
	if (size != 0)
		loadGamelistFile(xmlpath, system, fileMap);

	auto files = Utils::FileSystem::getDirContent(getTemporaryGamelistRecovery(system), true, false);
	for (auto file : files)
		loadGamelistFile(file, system, fileMap, size);

	if (size != SIZE_MAX)
		system->setGamelistHash(size);
}

bool addFileDataNode(pugi::xml_node& parent, const FileData* file, const char* tag, SystemData* system)
{
	//create game and add to parent node
	pugi::xml_node newNode = parent.append_child(tag);

	//write metadata
	file->getMetadata().appendToXML(newNode, true, system->getStartPath());

	if(newNode.children().begin() == newNode.child("name") //first element is name
		&& ++newNode.children().begin() == newNode.children().end() //theres only one element
		&& newNode.child("name").text().get() == file->getDisplayName()) //the name is the default
	{
		//if the only info is the default name, don't bother with this node
		//delete it and ultimately do nothing
		parent.remove_child(newNode);
		return false;
	}

	//there's something useful in there so we'll keep the node, add the path
	// try and make the path relative if we can so things still work if we change the rom folder location in the future
	newNode.prepend_child("path").text().set(Utils::FileSystem::createRelativePath(file->getPath(), system->getStartPath(), false).c_str());	
	return true;
}

bool saveToGamelistRecoveryInternal(FileData* file)
{
	LOG(LogDebug) << "Gamelist::saveToGamelistRecoveryInternal() - Execute name: " << file->getName() << ", path: " << file->getPath();

	pugi::xml_document doc;
	pugi::xml_node root = doc.append_child("gameList");

	const char* tag = file->getType() == GAME ? "game" : "folder";
	SystemData* system = file->getSourceFileData()->getSystem();

	root.append_attribute("parentHash").set_value(system->getGamelistHash());

	if (addFileDataNode(root, file, tag, system))
	{
		std::string fp = file->getFullPath();
		fp = Utils::FileSystem::createRelativePath(file->getFullPath(), system->getRootFolder()->getFullPath(), true);
		fp = Utils::FileSystem::getParent(fp) + "/" + Utils::FileSystem::getStem(fp) + ".xml";

		std::string path = Utils::FileSystem::getAbsolutePath(fp, getTemporaryGamelistRecovery(system));
		path = Utils::FileSystem::getCanonicalPath(path);

		std::string folder = Utils::FileSystem::getParent(path);

		if (!Utils::FileSystem::exists(folder))
			Utils::FileSystem::createDirectory(folder);

		if (!doc.save_file(path.c_str()))
		{
			LOG(LogError) << "Gamelist::saveToGamelistRecoveryInternal() - Error saving gamelist.xml to \"" << path << "\" (for system " << system->getName() << ")!";
			return false;
		}

		LOG(LogInfo) << "Gamelist::saveToGamelistRecoveryInternal() - Saving gamelist.xml to \"" << path << "\" (for system " << system->getName() << ")!";
		return true;
	}

	return false;
}

bool saveToGamelistRecovery(FileData* file)
{
	if (!Settings::getInstance()->getBool("SaveGamelistsOnExit") || !file->getSourceFileData()->getSystem()->isVisible())
		return false;

	if (Utils::Async::isCanRunAsync())
	{
		LOG(LogDebug) << "Gamelist::saveToGamelistRecovery() - Asynchronous execution!";
		auto dummy= std::async(std::launch::async, saveToGamelistRecoveryInternal, file);
		LOG(LogDebug) << "Gamelist::saveToGamelistRecovery() - exit Asynchronous execution!";
		return false;
	}
	LOG(LogDebug) << "Gamelist::saveToGamelistRecovery() - normal execution!";
	return saveToGamelistRecoveryInternal(file);
}

bool hasDirtyFile(SystemData* system)
{
	if (system == nullptr || !system->isGameSystem() || (system->getName() == "imageviewer") || !system->isVisible())
		return false;

	FolderData* rootFolder = system->getRootFolder();
	if (rootFolder == nullptr)
		return false;

	for (auto file : rootFolder->getFilesRecursive(GAME | FOLDER))
		if (file->getMetadata().wasChanged())
			return true;

	return false;
}

void updateGamelist(SystemData* system)
{
	//We do this by reading the XML again, adding changes and then writing it back,
	//because there might be information missing in our systemdata which would then miss in the new XML.
	//We have the complete information for every game though, so we can simply remove a game
	//we already have in the system from the XML, and then add it back from its GameData information...

	if(system == nullptr || Settings::getInstance()->getBool("IgnoreGamelist") || system->getName() == "imageviewer"
			|| system->isCollection() || !system->isGameSystem() || !system->isVisible())
		return;

	FolderData* rootFolder = system->getRootFolder();
	if (rootFolder == nullptr)
	{
		LOG(LogError) << "Gamelist::updateGamelist() - Found no root folder for system \"" << system->getName() << "\"!";
		return;
	}

	std::vector<FileData*> dirtyFiles;
	std::vector<FileData*> files = rootFolder->getFilesRecursive(GAME | FOLDER);
	for (auto file : files)
		if (file->getMetadata().wasChanged())
			dirtyFiles.push_back(file);

	if (dirtyFiles.size() == 0)
	{
		clearTemporaryGamelistRecovery(system);
		return;
	}

	int numUpdated = 0;

	pugi::xml_document doc;
	pugi::xml_node root;
	std::string xmlReadPath = system->getGamelistPath(false);

	if (Utils::FileSystem::exists(xmlReadPath))
	{
		//parse an existing file first
		pugi::xml_parse_result result = doc.load_file(xmlReadPath.c_str());
		if(!result)
			LOG(LogError) << "Gamelist::updateGamelist() - Error parsing XML file \"" << xmlReadPath << "\"!\n	" << result.description();

		root = doc.child("gameList");		
		if(!root)
		{
			LOG(LogError) << "Gamelist::updateGamelist() - Could not find <gameList> node in gamelist \"" << xmlReadPath << "\"!";
			root = doc.append_child("gameList");
		}
	}else{
		//set up an empty gamelist to append to
		root = doc.append_child("gameList");
	}

	std::map<std::string, pugi::xml_node> xmlMap;

	for (pugi::xml_node fileNode : root.children())
	{
		pugi::xml_node path = fileNode.child("path");
		if (path)
		{
			std::string nodePath = Utils::FileSystem::getCanonicalPath(Utils::FileSystem::resolveRelativePath(path.text().get(), system->getStartPath(), true));
			xmlMap[nodePath] = fileNode;
		}
	}
	
	// iterate through all files, checking if they're already in the XML
	for(auto file : dirtyFiles)
	{
		bool removed = false;

		// check if the file already exists in the XML
		// if it does, remove it before adding
		auto xmf = xmlMap.find(Utils::FileSystem::getCanonicalPath(file->getPath()));
		if (xmf != xmlMap.cend())
		{
			removed = true;
			root.remove_child(xmf->second);
		}
		
		const char* tag = (file->getType() == GAME) ? "game" : "folder";

		// it was either removed or never existed to begin with; either way, we can add it now
		if (addFileDataNode(root, file, tag, system))
			++numUpdated; // Only if really added
		else if (removed)
			++numUpdated; // Only if really removed
	}

	// Now write the file
	if (numUpdated > 0) 
	{
		//make sure the folders leading up to this path exist (or the write will fail)
		std::string xmlWritePath(system->getGamelistPath(true));
		Utils::FileSystem::createDirectory(Utils::FileSystem::getParent(xmlWritePath));

		LOG(LogInfo) << "Gamelist::updateGamelist() - Added/Updated " << numUpdated << " entities in '" << xmlReadPath << "'";

		// Secure XML writing -> Write to a temporary file first
		std::string tmpFile = xmlWritePath + ".tmp";
		if (Utils::FileSystem::exists(tmpFile))
			Utils::FileSystem::removeFile(tmpFile);

		if (!doc.save_file(tmpFile.c_str())) {
			LOG(LogError) << "Gamelist::updateGamelist() - Error saving gamelist.xml to \"" << xmlWritePath << "\" (for system " << system->getName() << ")!";
		}
		else if (Utils::FileSystem::exists(tmpFile))
		{
			doc.reset();

			// Secure XML writing
			if (Utils::FileSystem::getFileSize(tmpFile) != 0)
			{
				std::string savFile = xmlWritePath + ".old";

				// remove previous gamelist.xml.old file
				if (Utils::FileSystem::exists(savFile))
					Utils::FileSystem::removeFile(savFile);

				// rename gamelist.xml to gamelist.xml.old
				if (Utils::FileSystem::exists(xmlWritePath))
					std::rename(xmlWritePath.c_str(), savFile.c_str());
				else
					LOG(LogError) << "Gamelist::updateGamelist() - Unable to rename \"" << xmlWritePath << "to " << savFile << "\"!";

				// rename gamelist.tmp.xml to gamelist.xml
				if (std::rename(tmpFile.c_str(), xmlWritePath.c_str()) != 0)
					LOG(LogError) << "Gamelist::updateGamelist() - Unable to rename \"" << tmpFile << "to " << xmlWritePath << "\"!";

				clearTemporaryGamelistRecovery(system);
			}
			else 
				Utils::FileSystem::removeFile(tmpFile);
		}
	}
	else
		clearTemporaryGamelistRecovery(system);
}
