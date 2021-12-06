#include "FileData.h"

#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"
#include "utils/TimeUtil.h"
#include "AudioManager.h"
#include "CollectionSystemManager.h"
#include "FileFilterIndex.h"
#include "FileSorts.h"
#include "Log.h"
#include "MameNames.h"
#include "platform.h"
#include "Scripting.h"
#include "SystemData.h"
#include "VolumeControl.h"
#include "Window.h"
#include "views/UIModeController.h"
#include <assert.h>
#include "Gamelist.h"
#include "MetaData.h"
#include <fstream>

FileData::FileData(FileType type, const std::string& path, SystemData* system)
	: mType(type), mSystem(system), mParent(NULL), mMetadata(type == GAME ? GAME_METADATA : FOLDER_METADATA) // metadata is REALLY set in the constructor!
{
	mPath = Utils::FileSystem::createRelativePath(path, getSystemEnvData()->mStartPath, false);
	
//	TRACE("FileData : " << mPath);

	// metadata needs at least a name field (since that's what getName() will return)
	if (mMetadata.get(MetaDataId::Name).empty())
		mMetadata.set(MetaDataId::Name, getDisplayName());
	
	mMetadata.resetChangedFlag();
}

const std::string FileData::getPath() const
{ 	
	if (mPath.empty())
		return getSystemEnvData()->mStartPath;

	return Utils::FileSystem::resolveRelativePath(mPath, getSystemEnvData()->mStartPath, true);	
}

inline SystemEnvironmentData* FileData::getSystemEnvData() const
{ 
	return mSystem->getSystemEnvData(); 
}

std::string FileData::getSystemName() const
{
	return mSystem->getName();
}

FileData::~FileData()
{
	if(mParent)
		mParent->removeChild(this);

	if(mType == GAME)
		mSystem->removeFromIndex(this);	
}

std::string FileData::getDisplayName() const
{
	std::string stem = Utils::FileSystem::getStem(getPath());
	if(mSystem && mSystem->hasPlatformId(PlatformIds::ARCADE) || mSystem->hasPlatformId(PlatformIds::NEOGEO))
		stem = MameNames::getInstance()->getRealName(stem);

	return stem;
}

std::string FileData::getCleanName() const
{
	return Utils::String::removeParenthesis(this->getDisplayName());
}

const std::string FileData::getThumbnailPath()
{
	std::string thumbnail = getMetadata(MetaDataId::Thumbnail);

	// no thumbnail, try image
	if(thumbnail.empty())
	{
		thumbnail = getMetadata(MetaDataId::Image);
		
		// no image, try to use local image
		if(thumbnail.empty() && Settings::getInstance()->getBool("LocalArt"))
		{
			const char* extList[2] = { ".png", ".jpg" };
			for(int i = 0; i < 2; i++)
			{
				if(thumbnail.empty())
				{
					std::string path = getSystemEnvData()->mStartPath + "/images/" + getDisplayName() + "-thumb" + extList[i];
					if (Utils::FileSystem::exists(path))
					{
						setMetadata(MetaDataId::Thumbnail, path);
						thumbnail = path;
					}
				}
			}
		}

		if (thumbnail.empty())
			thumbnail = getMetadata(MetaDataId::Image);

		// no image, try to use local image
		if (thumbnail.empty() && Settings::getInstance()->getBool("LocalArt"))
		{
			const char* extList[2] = { ".png", ".jpg" };
			for (int i = 0; i < 2; i++)
			{
				if (thumbnail.empty())
				{
					std::string path = getSystemEnvData()->mStartPath + "/images/" + getDisplayName() + "-image" + extList[i];					
					if (!Utils::FileSystem::exists(path))
						path = getSystemEnvData()->mStartPath + "/images/" + getDisplayName() + extList[i];

					if (Utils::FileSystem::exists(path))
						thumbnail = path;
				}
			}
		}
	}

	return thumbnail;
}

const bool FileData::getFavorite()
{
	auto data = getMetadata(MetaDataId::Favorite);
	return !data.empty() && data == "true";
}

const bool FileData::getHidden()
{
	auto data = getMetadata(MetaDataId::Hidden);
	return !data.empty() && data == "true";
}

const bool FileData::getKidGame()
{
	auto data = getMetadata(MetaDataId::KidGame);
	return !data.empty() && data == "true";
}

static std::shared_ptr<bool> showFilenames;

void FileData::resetSettings()
{
	showFilenames = nullptr;
}

const std::string FileData::getName()
{
	if (showFilenames == nullptr)
		showFilenames = std::make_shared<bool>(Settings::getInstance()->getBool("ShowFilenames"));

	// Faster than accessing map each time
	if (*showFilenames)
	{
		if (mSystem != nullptr && !mSystem->hasPlatformId(PlatformIds::ARCADE) && !mSystem->hasPlatformId(PlatformIds::NEOGEO))
			return Utils::FileSystem::getStem(getPath());
		else
			return getDisplayName();
	}

	return getMetadata().getName();
}

const std::string FileData::getCore()
{
	return getMetadata(MetaDataId::Core);
}

const std::string FileData::getEmulator()
{
	return getMetadata(MetaDataId::Emulator);
}

const std::string FileData::getVideoPath()
{
	std::string video = getMetadata(MetaDataId::Video);
	
	// no video, try to use local video
	if(video.empty() && Settings::getInstance()->getBool("LocalArt"))
	{
		std::string path = getSystemEnvData()->mStartPath + "/images/" + getDisplayName() + "-video.mp4";
		if (Utils::FileSystem::exists(path))
		{
			setMetadata(MetaDataId::Video, path);
			video = path;
		}
	}
	
	return video;
}

const std::string FileData::getMarqueePath()
{
	std::string marquee = getMetadata(MetaDataId::Marquee);

	// no marquee, try to use local marquee
	if (marquee.empty() && Settings::getInstance()->getBool("LocalArt"))
	{
		const char* extList[2] = { ".png", ".jpg" };
		for(int i = 0; i < 2; i++)
		{
			if(marquee.empty())
			{
				std::string path = getSystemEnvData()->mStartPath + "/images/" + getDisplayName() + "-marquee" + extList[i];
				if(Utils::FileSystem::exists(path))
				{
					setMetadata(MetaDataId::Marquee, path);
					marquee = path;
				}
			}
		}
	}

	return marquee;
}

const std::string FileData::getImagePath()
{
	std::string image = getMetadata(MetaDataId::Image);

	// no image, try to use local image
	if(image.empty())
	{
		const char* extList[2] = { ".png", ".jpg" };
		for(int i = 0; i < 2; i++)
		{
			if(image.empty())
			{
				std::string path = getSystemEnvData()->mStartPath + "/images/" + getDisplayName() + "-image" + extList[i];
				if(Utils::FileSystem::exists(path))
				{
						setMetadata(MetaDataId::Image, path);
						image = path;
				}
			}
		}
	}

	return image;
}

std::string FileData::getKey() {
	return getFileName();
}

const bool FileData::isArcadeAsset()
{
	if (mSystem && (mSystem->hasPlatformId(PlatformIds::ARCADE) || mSystem->hasPlatformId(PlatformIds::NEOGEO)))
	{	
		const std::string stem = Utils::FileSystem::getStem(getPath());
		return MameNames::getInstance()->isBios(stem) || MameNames::getInstance()->isDevice(stem);		
	}

	return false;
}

const bool FileData::isVerticalArcadeGame()
{
	if (mSystem && mSystem->hasPlatformId(PlatformIds::ARCADE))
	{
		const std::string stem = Utils::FileSystem::getStem(getPath());
		return MameNames::getInstance()->isVertical(stem);
	}

	return false;
}

FileData* FileData::getSourceFileData()
{
	return this;
}

void FileData::launchGame(Window* window)
{
	LOG(LogInfo) << "FileData::launchGame() - Attempting to launch game...";

	AudioManager::getInstance()->deinit();
	VolumeControl::getInstance()->deinit();

	bool hideWindow = Settings::getInstance()->getBool("HideWindow");
	window->deinit(hideWindow);

	std::string command = getSystemEnvData()->mLaunchCommand;

	const std::string rom = Utils::FileSystem::getEscapedPath(getPath());
	const std::string basename = Utils::FileSystem::getStem(getPath());
	const std::string rom_raw = Utils::FileSystem::getPreferredPath(getPath());
	
	std::string emulator = getEmulator();
	if (emulator.length() == 0)
		emulator = getSystemEnvData()->getDefaultEmulator();

	std::string core = getCore();
	if (core.length() == 0)
		core = getSystemEnvData()->getDefaultCore(emulator);

	std::string customCommandLine = getSystemEnvData()->getEmulatorCommandLine(emulator);
	if (customCommandLine.length() > 0)
		command = customCommandLine;
	


	command = Utils::String::replace(command, "%EMULATOR%", emulator);
	command = Utils::String::replace(command, "%CORE%", core);

	command = Utils::String::replace(command, "%ROM%", rom);
	command = Utils::String::replace(command, "%BASENAME%", basename);
	command = Utils::String::replace(command, "%ROM_RAW%", rom_raw);		
	command = Utils::String::replace(command, "%SYSTEM%", getSystemName());
	command = Utils::String::replace(command, "%HOME%", Utils::FileSystem::getHomePath());

	Scripting::fireEvent("game-start", rom, basename);

	time_t tstart = time(NULL);

	LOG(LogInfo) << "	" << command;

	int exitCode = runSystemCommand(command, getDisplayName(), hideWindow ? NULL : window);
	if (exitCode != 0)
	{
		LOG(LogWarning) << "FileData::launchGame() - ...launch terminated with nonzero exit code " << exitCode << "!";
	}

	Scripting::fireEvent("game-end");

	window->init(hideWindow, Settings::getInstance()->getBool("FullScreenMode"));

	VolumeControl::getInstance()->init();
	AudioManager::getInstance()->init();	
	window->normalizeNextUpdate();

	//update number of times the game has been launched
	if (exitCode == 0)
	{
		FileData* gameToUpdate = getSourceFileData();

		int timesPlayed = gameToUpdate->getMetadata().getInt(MetaDataId::PlayCount) + 1;
		gameToUpdate->setMetadata(MetaDataId::PlayCount, std::to_string(static_cast<long long>(timesPlayed)));

		//update game time played
		time_t tend = time(NULL);
		long elapsedSeconds = difftime(tend, tstart);
		long gameTime = gameToUpdate->getMetadata().getInt(MetaDataId::GameTime) + elapsedSeconds;
		if (elapsedSeconds >= 10)
			gameToUpdate->setMetadata(MetaDataId::GameTime, std::to_string(static_cast<long>(gameTime)));

		//update last played time
		gameToUpdate->setMetadata(MetaDataId::LastPlayed, Utils::Time::DateTime(Utils::Time::now()));
		CollectionSystemManager::get()->refreshCollectionSystems(gameToUpdate);
		saveToGamelistRecovery(gameToUpdate);
	}

	// music
	if (Settings::getInstance()->getBool("audio.bgmusic"))
		AudioManager::getInstance()->playRandomMusic();
}

CollectionFileData::CollectionFileData(FileData* file, SystemData* system)
	: FileData(file->getSourceFileData()->getType(), "", system)
{
	mSourceFileData = file->getSourceFileData();
	mParent = NULL;
	// metadata = mSourceFileData->metadata;	
	mDirty = true;
}

SystemEnvironmentData* CollectionFileData::getSystemEnvData() const
{ 
	return mSourceFileData->getSystemEnvData();
}

const std::string CollectionFileData::getPath() const
{
	return mSourceFileData->getPath();
}

std::string CollectionFileData::getSystemName() const
{
	return mSourceFileData->getSystem()->getName();
}

CollectionFileData::~CollectionFileData()
{
	// need to remove collection file data at the collection object destructor
	if(mParent)
		mParent->removeChild(this);
	mParent = NULL;
}

std::string CollectionFileData::getKey() {
	return getFullPath();
}

FileData* CollectionFileData::getSourceFileData()
{
	return mSourceFileData;
}

void CollectionFileData::refreshMetadata()
{
	// metadata = mSourceFileData->metadata;
	mDirty = true;
}

const std::string CollectionFileData::getName()
{
	if (mDirty) {
		mCollectionFileName = Utils::String::removeParenthesis(mSourceFileData->getMetadata(MetaDataId::Name));
		mCollectionFileName += " [" + Utils::String::toUpper(mSourceFileData->getSystem()->getName()) + "]";
		mDirty = false;
	}

	if (Settings::getInstance()->getBool("CollectionShowSystemInfo"))
		return mCollectionFileName;
		
	return Utils::String::removeParenthesis(mSourceFileData->getMetadata(MetaDataId::Name));
}

const std::vector<FileData*> FolderData::getChildrenListToDisplay() 
{
	std::vector<FileData*> ret;

	std::string showFoldersMode = Settings::getInstance()->getString("FolderViewMode");
	
	bool showHiddenFiles = Settings::getInstance()->getBool("ShowHiddenFiles");
	bool filterKidGame = false;

	if (!Settings::getInstance()->getBool("ForceDisableFilters")) 
	{
		if (UIModeController::getInstance()->isUIModeKiosk())
			showHiddenFiles = false;

		if (UIModeController::getInstance()->isUIModeKid())
			filterKidGame = true;
	}

	auto sys = CollectionSystemManager::get()->getSystemToView(mSystem);

	FileFilterIndex* idx = sys->getIndex(false);
	if (idx != nullptr && !idx->isFiltered())
		idx = nullptr;

	std::vector<FileData*>* items = &mChildren;

	std::vector<FileData*> flatGameList;
	if (showFoldersMode == "never")
	{
		flatGameList = getFlatGameList(false, sys);
		items = &flatGameList;
	}

	bool refactorUniqueGameFolders = (showFoldersMode == "having multiple games");

	for (auto it = items->cbegin(); it != items->cend(); it++)
	{
		if (idx != nullptr && !idx->showFile((*it)))
			continue;

		if (!showHiddenFiles && (*it)->getHidden())
			continue;

		if (filterKidGame && !(*it)->getKidGame())
			continue;

		if ((*it)->getType() == FOLDER && refactorUniqueGameFolders)
		{
			FolderData* pFolder = (FolderData*)(*it);
			auto fd = pFolder->findUniqueGameForFolder();
			if (fd != nullptr)
			{
				if (idx != nullptr && !idx->showFile(fd))
					continue;

				if (!showHiddenFiles && fd->getHidden())
					continue;

				if (filterKidGame && !fd->getKidGame())
					continue;

				ret.push_back(fd);

				continue;
			}
		}

		ret.push_back(*it);
	}

	unsigned int currentSortId = sys->getSortId();
	if (currentSortId >= FileSorts::getSortTypes().size())
		currentSortId = 0;

	const FileSorts::SortType& sort = FileSorts::getSortTypes().at(currentSortId);
	std::sort(ret.begin(), ret.end(), sort.comparisonFunction);

	if (!sort.ascending)
		std::reverse(ret.begin(), ret.end());

	return ret;
}

FileData* FolderData::findUniqueGameForFolder()
{
	auto games = getFilesRecursive(GAME);
	if (games.size() == 1)
	{
		auto it = games.cbegin();
		if ((*it)->getType() == GAME)
			return (*it);
	}

	return nullptr;
}

std::vector<FileData*> FolderData::getFlatGameList(bool displayedOnly, SystemData* system) const
{
	std::vector<FileData*> ret = getFilesRecursive(GAME, displayedOnly, system);

	unsigned int currentSortId = system->getSortId();
	if (currentSortId < 0 || currentSortId >= FileSorts::getSortTypes().size())
		currentSortId = 0;

	auto sort = FileSorts::getSortTypes().at(currentSortId);

	std::stable_sort(ret.begin(), ret.end(), sort.comparisonFunction);

	if (!sort.ascending)
		std::reverse(ret.begin(), ret.end());

	return ret;
}

std::vector<FileData*> FolderData::getFilesRecursive(unsigned int typeMask, bool displayedOnly, SystemData* system) const
{
	std::vector<FileData*> out;

	FileFilterIndex* idx = (system != nullptr ? system : mSystem)->getIndex(false);

	for (auto it = mChildren.cbegin(); it != mChildren.cend(); it++)
	{
		if ((*it)->getType() & typeMask)
		{
			if (!displayedOnly || idx == nullptr || !idx->isFiltered() || idx->showFile(*it))
				out.push_back(*it);
		}

		if ((*it)->getType() != FOLDER)
			continue;

		FolderData* folder = (FolderData*)(*it);
		if (folder->getChildren().size() > 0)
		{
			std::vector<FileData*> subchildren = folder->getFilesRecursive(typeMask, displayedOnly, system);
			out.insert(out.cend(), subchildren.cbegin(), subchildren.cend());
		}
	}

	return out;
}

void FolderData::addChild(FileData* file, bool assignParent)
{
	assert(file->getParent() == nullptr || !assignParent);

	mChildren.push_back(file);

	if (assignParent)
		file->setParent(this);
}

void FolderData::removeChild(FileData* file)
{
	assert(mType == FOLDER);
	assert(file->getParent() == this);

	for (auto it = mChildren.cbegin(); it != mChildren.cend(); it++)
	{
		if (*it == file)
		{
			file->setParent(NULL);
			mChildren.erase(it);
			return;
		}
	}

	// File somehow wasn't in our children.
	assert(false);

}

FileData* FolderData::FindByPath(const std::string& path)
{
	std::vector<FileData*> children = getChildren();

	for (std::vector<FileData*>::const_iterator it = children.cbegin(); it != children.cend(); ++it)
	{
		if ((*it)->getPath() == path)
			return (*it);

		if ((*it)->getType() != FOLDER)
			continue;
		
		auto item = ((FolderData*)(*it))->FindByPath(path);
		if (item != nullptr)
			return item;
	}

	return nullptr;
}

void FolderData::createChildrenByFilenameMap(std::unordered_map<std::string, FileData*>& map)
{
	std::vector<FileData*> children = getChildren();

	for (std::vector<FileData*>::const_iterator it = children.cbegin(); it != children.cend(); ++it)
	{
		if ((*it)->getType() == FOLDER)
			((FolderData*)(*it))->createChildrenByFilenameMap(map);			
		else 
			map[(*it)->getKey()] = (*it);
	}	
}

bool FileData::hasContentFiles()
{
	if (mPath.empty())
		return false;

	std::string ext = Utils::String::toLower(Utils::FileSystem::getExtension(mPath));
	if (ext == ".m3u" || ext == ".cue" || ext == ".ccd" || ext == ".gdi")
		return getSourceFileData()->getSystemEnvData()->isValidExtension(ext) && getSourceFileData()->getSystemEnvData()->mSearchExtensions.size() > 1;

	return false;
}

static std::vector<std::string> getTokens(const std::string& string)
{
	std::vector<std::string> tokens;

	bool inString = false;
	int startPos = 0;
	int i = 0;
	for (;;)
	{
		char c = string[i];

		switch (c)
		{
		case '\"':
			inString = !inString;
			if (inString)
				startPos = i + 1;

		case '\0':
		case ' ':
		case '\t':
		case '\r':
		case '\n':
			if (!inString)
			{
				std::string value = string.substr(startPos, i - startPos);
				if (!value.empty())
					tokens.push_back(value);

				startPos = i + 1;
			}
			break;
		}

		if (c == '\0')
			break;

		i++;
	}

	return tokens;
}

std::set<std::string> FileData::getContentFiles()
{
	std::set<std::string> files;

	if (mPath.empty())
		return files;

	if (Utils::FileSystem::isDirectory(mPath))
	{
		for (auto file : Utils::FileSystem::getDirContent(mPath, true, true))
			files.insert(file);
	}
	else if (hasContentFiles())
	{
		auto path = Utils::FileSystem::getParent(mPath);
		auto ext = Utils::String::toLower(Utils::FileSystem::getExtension(mPath));

		if (ext == ".cue")
		{
			std::string start = "FILE";

			std::ifstream cue(WINSTRINGW(mPath));
			if (cue && cue.is_open())
			{
				std::string line;
				while (std::getline(cue, line))
				{
					if (!Utils::String::startsWith(line, start))
						continue;

					auto tokens = getTokens(line);
					if (tokens.size() > 1)
						files.insert(path + "/" + tokens[1]);
				}

				cue.close();
			}
		}
		else if (ext == ".ccd")
		{
			std::string stem = Utils::FileSystem::getStem(mPath);
			files.insert(path + "/" + stem + ".cue");
			files.insert(path + "/" + stem + ".img");
			files.insert(path + "/" + stem + ".bin");
			files.insert(path + "/" + stem + ".sub");
		}
		else if (ext == ".m3u")
		{
			std::ifstream m3u(WINSTRINGW(mPath));
			if (m3u && m3u.is_open())
			{
				std::string line;
				while (std::getline(m3u, line))
				{
					auto trim = Utils::String::trim(line);
					if (trim[0] == '#' || trim[0] == '\\' || trim[0] == '/')
						continue;

					files.insert(path + "/" + trim);
				}

				m3u.close();
			}
		}
		else if (ext == ".gdi")
		{
			std::ifstream gdi(WINSTRINGW(mPath));
			if (gdi && gdi.is_open())
			{
				std::string line;
				while (std::getline(gdi, line))
				{
					auto tokens = getTokens(line);
					if (tokens.size() > 5 && tokens[4].find(".") != std::string::npos)
						files.insert(path + "/" + tokens[4]);
				}

				gdi.close();
			}
		}
	}

	return files;
}
