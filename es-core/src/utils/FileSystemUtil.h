#pragma once
#ifndef ES_CORE_UTILS_FILE_SYSTEM_UTIL_H
#define ES_CORE_UTILS_FILE_SYSTEM_UTIL_H

#include <list>
#include <string>
#include "utils/TimeUtil.h"

namespace Utils
{
	namespace FileSystem
	{
		typedef std::list<std::string> stringList;

		stringList  getDirContent      (const std::string& _path, const bool _recursive = false, const bool includeHidden = true);
		stringList  getPathList        (const std::string& _path);
		void        setHomePath        (const std::string& _path);
		std::string getHomePath        ();
		std::string getCWDPath         ();
		void        setEsConfigPath    (const std::string& _path);
		std::string getEsConfigPath    ();
		std::string getSharedConfigPath();
		void        setUserDataPath    (const std::string& _path);
		std::string getUserDataPath    ();
		void        setExePath         (const std::string& _path);
		std::string getExePath         ();
		std::string getPreferredPath   (const std::string& _path);
		std::string getGenericPath     (const std::string& _path);
		std::string getEscapedPath     (const std::string& _path);
		std::string getCanonicalPath   (const std::string& _path);
		std::string getAbsolutePath    (const std::string& _path, const std::string& _base = getCWDPath());
		std::string getParent          (const std::string& _path);
		std::string getFileName        (const std::string& _path);
		std::string getStem            (const std::string& _path);
		std::string getExtension       (const std::string& _path);
		std::string resolveRelativePath(const std::string& _path, const std::string& _relativeTo, const bool _allowHome);
		std::string createRelativePath (const std::string& _path, const std::string& _relativeTo, const bool _allowHome);
		std::string removeCommonPath   (const std::string& _path, const std::string& _common, bool& _contains);
		std::string resolveSymlink     (const std::string& _path);
		std::string combine(const std::string& _path, const std::string& filename);
		bool        removeFile         (const std::string& _path);
		bool        createDirectory    (const std::string& _path);
		bool        removeDirectory    (const std::string& _path);
		bool        exists             (const std::string& _path);
		size_t      getFileSize        (const std::string& _path);
		Utils::Time::DateTime getFileCreationDate(const std::string& _path);
		Utils::Time::DateTime getFileModificationDate(const std::string& _path);
		bool        isAbsolute         (const std::string& _path);
		bool        isRegularFile      (const std::string& _path);
		bool        isDirectory        (const std::string& _path);
		bool        isSymlink          (const std::string& _path);
		bool        isHidden           (const std::string& _path);
		bool        isExecutable       (const std::string& _path);
		std::string megaBytesToString  (unsigned long size);

		void		deleteDirectoryFiles(const std::string path, bool deleteDirectory = false);
		bool		renameFile(const std::string src, const std::string dst, bool overWrite = true);

		std::string getTempPath();

		std::string getFileCrc32(const std::string& filename);
		std::string getFileMd5(const std::string& filename);

		// FCA
		struct FileInfo
		{
		public:
			std::string path;
			bool hidden;
			bool directory;
		};

		typedef std::list<FileInfo> fileList;

		fileList        getDirectoryFiles(const std::string& _path);
		fileList        getDirInfo       (const std::string& _path/*, const bool _recursive = false*/);

		std::string readAllText          (const std::string fileName);
		void        writeAllText         (const std::string fileName, const std::string text);
		bool        copyFile             (const std::string src, const std::string dst);

		class FileSystemCacheActivator
		{
		public:
			FileSystemCacheActivator();
			~FileSystemCacheActivator();

		private:
			static int mReferenceCount;
		};
		
	} // FileSystem::



} // Utils::

#endif // ES_CORE_UTILS_FILE_SYSTEM_UTIL_H
