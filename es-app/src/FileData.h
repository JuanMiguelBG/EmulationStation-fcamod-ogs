#pragma once
#ifndef ES_APP_FILE_DATA_H
#define ES_APP_FILE_DATA_H

#include "utils/FileSystemUtil.h"
#include "MetaData.h"
#include <unordered_map>
#include <set>

class SystemData;
class Window;
struct SystemEnvironmentData;

enum FileType
{
	GAME = 1,   // Cannot have children.
	FOLDER = 2,
	PLACEHOLDER = 3
};

enum FileChangeType
{
	FILE_ADDED,
	FILE_METADATA_CHANGED,
	FILE_REMOVED,
	FILE_SORTED
};

class FolderData;

// A tree node that holds information for a file.
class FileData
{
public:
	FileData(FileType type, const std::string& path, SystemData* system);
	virtual ~FileData();

	virtual const std::string getName();

	inline FileType getType() const { return mType; }
	
	inline FolderData* getParent() const { return mParent; }
	void setParent(FolderData* parent) { mParent = parent; }

	inline SystemData* getSystem() const { return mSystem; }

	virtual const std::string getPath() const;

	virtual SystemEnvironmentData* getSystemEnvData() const;

	virtual const std::string getThumbnailPath();
	virtual const std::string getVideoPath();
	virtual const std::string getMarqueePath();
	virtual const std::string getImagePath();

	virtual const std::string getCore();
	virtual const std::string getEmulator();

	virtual const bool getHidden();
	virtual const bool getFavorite();
	virtual const bool getKidGame();

	inline bool isPlaceHolder() { return mType == PLACEHOLDER; };

	virtual inline void refreshMetadata() { return; };

	virtual std::string getKey();
	const bool isArcadeAsset();
	const bool isVerticalArcadeGame();
	inline std::string getFullPath() { return getPath(); };
	inline std::string getFileName() { return Utils::FileSystem::getFileName(getPath()); };
	virtual FileData* getSourceFileData();
	virtual std::string getSystemName() const;

	// Returns our best guess at the "real" name for this file (will attempt to perform MAME name translation)
	std::string getDisplayName() const;

	// As above, but also remove parenthesis
	std::string getCleanName() const;

	void launchGame(Window* window);

	static void resetSettings();
	
	virtual const MetaDataList& getMetadata() const { return mMetadata; }
	virtual MetaDataList& getMetadata() { return mMetadata; }

	void setMetadata(MetaDataList value) { getMetadata() = value; }
	
	std::string getMetadata(MetaDataId key) { return getMetadata().get(key); }
	void setMetadata(MetaDataId key, const std::string& value) { return getMetadata().set(key, value); }

	void deleteGameFiles();

	bool hasContentFiles();
	std::set<std::string> getContentFiles();

private:
	std::string getMessageFromExitCode(int exitCode);

	MetaDataList mMetadata;

protected:
	FolderData* mParent;
	std::string mPath;
	FileType mType;
	SystemData* mSystem;
};

class CollectionFileData : public FileData
{
public:
	CollectionFileData(FileData* file, SystemData* system);
	~CollectionFileData();
	const std::string getName();
	void refreshMetadata();
	FileData* getSourceFileData();
	std::string getKey();
	virtual const std::string getPath() const;

	virtual std::string getSystemName() const;
	virtual SystemEnvironmentData* getSystemEnvData() const;

	virtual const MetaDataList& getMetadata() const { return mSourceFileData->getMetadata(); }
	virtual MetaDataList& getMetadata() { return mSourceFileData->getMetadata(); }

private:
	// needs to be updated when metadata changes
	std::string mCollectionFileName;
	FileData* mSourceFileData;

	bool mDirty;
};

class FolderData : public FileData
{
	friend class FileData;
	friend class SystemData;

public:
	FolderData(const std::string& startpath, SystemData* system, bool ownsChildrens=true) : FileData(FOLDER, startpath, system)
	{
		mIsDisplayableAsVirtualFolder = false;
		mOwnsChildrens = ownsChildrens;
	}

	~FolderData()
	{
		if (mOwnsChildrens)
		{
			for (int i = mChildren.size() - 1; i >= 0; i--)
				delete mChildren.at(i);
		}

		mChildren.clear();
	}

	inline bool isVirtualFolderDisplay() { return mIsDisplayableAsVirtualFolder && !mOwnsChildrens; }
	void enableVirtualFolderDisplay(bool value) { mIsDisplayableAsVirtualFolder = value; };
	bool isVirtualFolderDisplayEnabled() { return mIsDisplayableAsVirtualFolder; };

	FileData* FindByPath(const std::string& path);

	inline const std::vector<FileData*>& getChildren() const { return mChildren; }
	const std::vector<FileData*> getChildrenListToDisplay();
	std::vector<FileData*> getFilesRecursive(unsigned int typeMask, bool displayedOnly = false, SystemData* system = nullptr) const;

	void addChild(FileData* file, bool assignParent = true); // Error if mType != FOLDER
	void removeChild(FileData* file); //Error if mType != FOLDER

	void createChildrenByFilenameMap(std::unordered_map<std::string, FileData*>& map);

	FileData* findUniqueGameForFolder();

private:
	std::vector<FileData*> getFlatGameList(bool displayedOnly, SystemData* system) const;
	std::vector<FileData*> mChildren;

	bool	mOwnsChildrens;
	bool	mIsDisplayableAsVirtualFolder;
};

#endif // ES_APP_FILE_DATA_H
