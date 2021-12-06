#define _FILE_OFFSET_BITS 64

#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"
#include "utils/TimeUtil.h"
#include "utils/ZipFile.h"
#include "utils/md5.h"

#include "Settings.h"
#include <sys/stat.h>
#include <string.h>
#include "platform.h"

#include <dirent.h>
#include <unistd.h>
#include <mutex>

#include <fstream>
#include <sstream>
#include <algorithm>
#include "Log.h"

namespace Utils
{
	namespace FileSystem
	{

		static std::string configPath;
		static std::string userDataPath;
		static std::string exePath;
		static std::string homePath;

		void setUserDataPath(const std::string& _path)
		{
			LOG(LogInfo) << "Utils::FileSystem::setUserDataPath() - Setting path '" << _path << "'...";
			if (_path.empty())
				return;

			userDataPath = Utils::FileSystem::getGenericPath(_path);

			if (isRegularFile(userDataPath)) {
				userDataPath = getParent(userDataPath);
			}
			LOG(LogInfo) << "Utils::FileSystem::setUserDataPath() - final path '" << userDataPath << "'...";
		} // setUserDataPath

		std::string getUserDataPath()
		{
			if (userDataPath.empty())
				userDataPath = "/userdata";

			return userDataPath;
		}

		void setEsConfigPath(const std::string& _path)
		{
			LOG(LogInfo) << "Utils::FileSystem::setEsConfigPath() - Setting path '" << _path << "'...";
			if (_path.empty())
				return;

			configPath = Utils::FileSystem::getGenericPath(_path);

			if (isRegularFile(configPath))
				configPath = getParent(configPath);

			LOG(LogInfo) << "Utils::FileSystem::setEsConfigPath() - final path '" << configPath << "'...";
		} // setEsConfigPath

		std::string getEsConfigPath()
		{
			if (configPath.empty())
				configPath = Utils::FileSystem::getCanonicalPath(Utils::FileSystem::getHomePath() + "/.emulationstation");

			return configPath;
		}

		std::string getSharedConfigPath()
		{
			return "/usr/share/emulationstation"; // batocera
		}
		
		struct FileCache
		{
			FileCache() {}

			FileCache(bool _exists, bool _dir) 
			{
				directory = _dir;
				exists = _exists;
				hidden = false;
				isSymLink = false;
			}

			FileCache(const std::string& name, dirent* entry)
			{
				exists = true;
				hidden = (getFileName(name)[0] == '.');
				directory = (entry->d_type == 4); // DT_DIR;
				isSymLink = (entry->d_type == 10); // DT_LNK;
			}

			FileCache(dirent* entry, bool _hidden)
			{
				exists = true;
				hidden = _hidden;
				directory = (entry->d_type == 4); // DT_DIR;
				isSymLink = (entry->d_type == 10); // DT_LNK;
			}

			FileCache(const std::string& name, dirent* entry, bool _hidden)
			{
				exists = true;
				hidden = _hidden;

				if (entry->d_type == 10)
				{
					struct stat64 info;
					if (stat64(resolveSymlink(name).c_str(), &info) == 0)
						directory = S_ISDIR(info.st_mode);
					else
						directory = false;
				}
				else
					directory = (entry->d_type == 4);

				isSymLink = (entry->d_type == 10); // DT_LNK;
			}

			bool exists;
			bool directory;
			bool hidden;
			bool isSymLink;

			static int fromStat64(const std::string& key, struct stat64* info)
			{
				int ret = stat64(key.c_str(), info);

				std::unique_lock<std::mutex> lock(mFileCacheMutex);

				FileCache cache(ret == 0, false);
				if (cache.exists)
				{
					cache.directory = S_ISDIR(info->st_mode);
					cache.isSymLink = S_ISLNK(info->st_mode);
				}

				mFileCache[key] = cache;

				return ret;
			}

			static void add(const std::string& key, FileCache cache)
			{
				if (!mEnabled)
					return;

				std::unique_lock<std::mutex> lock(mFileCacheMutex);
				mFileCache[key] = cache;
			}

			static FileCache* get(const std::string& key)
			{
				if (!mEnabled)
					return nullptr;

				std::unique_lock<std::mutex> lock(mFileCacheMutex);

				auto it = mFileCache.find(key);
				if (it != mFileCache.cend())
					return &it->second;

				it = mFileCache.find(Utils::FileSystem::getParent(key)+"/*");
				if (it != mFileCache.cend())
				{
					mFileCache[key] = FileCache(false, false);
					return &mFileCache[key];
				}				

				return nullptr;
			}

			static void resetCache()
			{
				std::unique_lock<std::mutex> lock(mFileCacheMutex);
				mFileCache.clear();
			}

			static void setEnabled(bool value) { mEnabled = value; }

		private:
			static std::map<std::string, FileCache> mFileCache;
			static std::mutex mFileCacheMutex;
			static bool mEnabled;
		};

		std::map<std::string, FileCache> FileCache::mFileCache;
		std::mutex FileCache::mFileCacheMutex;
		bool FileCache::mEnabled = false;

		FileSystemCacheActivator::FileSystemCacheActivator()
		{
			if (mReferenceCount == 0)
			{
				FileCache::setEnabled(true);
				FileCache::resetCache();
			}

			mReferenceCount++;
		}

		FileSystemCacheActivator::~FileSystemCacheActivator()
		{
			mReferenceCount--;

			if (mReferenceCount <= 0)
			{
				FileCache::setEnabled(false);
				FileCache::resetCache();
			}
		}

		int FileSystemCacheActivator::mReferenceCount = 0;

		fileList getDirInfo(const std::string& _path/*, const bool _recursive*/)
		{
			std::string path = getGenericPath(_path);
			fileList  contentList;

			// tell filecache we enumerated the folder
			FileCache::add(path + "/*", FileCache(true, true));

			// only parse the directory, if it's a directory
			if (isDirectory(path))
			{
				DIR* dir = opendir(path.c_str());

				if (dir != NULL)
				{
					struct dirent* entry;

					// loop over all files in the directory
					while ((entry = readdir(dir)) != NULL)
					{
						std::string name(entry->d_name);

						// ignore "." and ".."
						if ((name != ".") && (name != ".."))
						{
							std::string fullName(getGenericPath(path + "/" + name));

							FileInfo fi;
							fi.path = fullName;
							fi.hidden = Utils::FileSystem::isHidden(fullName);

							if (entry->d_type == 10) // DT_LNK
							{
								struct stat64 si;
								if (stat64(resolveSymlink(fullName).c_str(), &si) == 0)
									fi.directory = S_ISDIR(si.st_mode);
								else
									fi.directory = false;
							}
							else
								fi.directory = (entry->d_type == 4); // DT_DIR;

							FileCache::add(fullName, FileCache(entry, fi.hidden));

							//DT_LNK
							contentList.push_back(fi);
						}
					}

					closedir(dir);
				}

			}

			// return the content list
			return contentList;

		} // getDirContent
		
		stringList getDirContent(const std::string& _path, const bool _recursive, const bool includeHidden)
		{
			std::string path = getGenericPath(_path);
			stringList  contentList;

			// only parse the directory, if it's a directory
			if(isDirectory(path))
			{
				FileCache::add(path + "/*", FileCache(true, true));


				DIR* dir = opendir(path.c_str());

				if(dir != NULL)
				{
					struct dirent* entry;

					// loop over all files in the directory
					while((entry = readdir(dir)) != NULL)
					{
						std::string name(entry->d_name);

						// ignore "." and ".."
						if((name != ".") && (name != ".."))
						{
							std::string fullName(getGenericPath(path + "/" + name));
							FileCache::add(fullName, FileCache(fullName, entry));

							if (!includeHidden && Utils::FileSystem::isHidden(fullName))
								continue;

							contentList.push_back(fullName);
							
							if(_recursive && isDirectory(fullName))
								contentList.merge(getDirContent(fullName, true));
						}
					}

					closedir(dir);
				}

			}

			// return the content list
			return contentList;

		} // getDirContent

		fileList getDirectoryFiles(const std::string& _path)
		{
			std::string path = getGenericPath(_path);
			fileList  contentList;

			// tell filecache we enumerated the folder
			FileCache::add(path + "/*", FileCache(true, true));

			// only parse the directory, if it's a directory
			// if (isDirectory(path))
			{
				DIR* dir = opendir(path.c_str());

				if (dir != NULL)
				{
					struct dirent* entry;

					// loop over all files in the directory
					while ((entry = readdir(dir)) != NULL)
					{
						std::string name(entry->d_name);

						// ignore "." and ".."
						if ((name != ".") && (name != ".."))
						{
							std::string fullName(getGenericPath(path + "/" + name));

							FileInfo fi;
							fi.path = fullName;
							fi.hidden = Utils::FileSystem::isHidden(fullName);

							if (entry->d_type == 10) // DT_LNK
							{
								struct stat64 si;
								if (stat64(resolveSymlink(fullName).c_str(), &si) == 0)
									fi.directory = S_ISDIR(si.st_mode);
								else
									fi.directory = false;
							}
							else
								fi.directory = (entry->d_type == 4); // DT_DIR;

							FileCache::add(fullName, FileCache(fullName, entry, fi.hidden));

							//DT_LNK
							contentList.push_back(fi);
						}
					}

					closedir(dir);
				}

			}

			// return the content list
			return contentList;

		} // getDirectoryFiles

		stringList getPathList(const std::string& _path)
		{
			stringList  pathList;
			std::string path  = getGenericPath(_path);
			size_t      start = 0;
			size_t      end   = 0;

			// split at '/'
			while((end = path.find("/", start)) != std::string::npos)
			{
				if(end != start)
					pathList.push_back(std::string(path, start, end - start));

				start = end + 1;
			}

			// add last folder / file to pathList
			if(start != path.size())
				pathList.push_back(std::string(path, start, path.size() - start));

			// return the path list
			return pathList;

		} // getPathList

		void setHomePath(const std::string& _path)
		{
			homePath = Utils::FileSystem::getGenericPath(_path);
		}

		std::string getHomePath()
		{
			// only construct the homepath once
			if (homePath.length())
				return homePath;

			// check if "getExePath()/.emulationstation/es_systems.cfg" exists
			if (Utils::FileSystem::exists(getExePath() + "/.emulationstation/es_systems.cfg"))
				homePath = getExePath();

			// check for HOME environment variable
			if (!homePath.length())
			{
				char* envHome = getenv("HOME");
				if (envHome)
					homePath = getGenericPath(envHome);
			}

			// no homepath found, fall back to current working directory
			if (!homePath.length())
				homePath = getCWDPath();

			homePath = getGenericPath(homePath);

			// return constructed homepath
			return homePath;
		} // getHomePath

		std::string getCWDPath()
		{
			char temp[512];
			return (getcwd(temp, 512) ? getGenericPath(temp) : "");
		} // getCWDPath

		void setExePath(const std::string& _path)
		{
			LOG(LogInfo) << "Utils::FileSystem::setExePath() - Setting path '" << _path << "'...";
			constexpr int path_max = 32767;
			std::string result(path_max, 0);
			if (readlink("/proc/self/exe", &result[0], path_max) != -1) {
				exePath = getCanonicalPath(result);
			}

			// If the native implementations fail, fallback to argv[0]
			if (exePath.empty()) {
				exePath = getCanonicalPath(_path);
			}
			if (isRegularFile(exePath))
				exePath = getParent(exePath);

			LOG(LogInfo) << "Utils::FileSystem::setExePath() - final path '" << exePath << "'...";
		} // setExePath

		std::string getExePath()
		{
			// return constructed exepath
			return exePath;
		} // getExePath

		std::string getPreferredPath(const std::string& _path)
		{
			std::string path   = _path;
			size_t      offset = std::string::npos;
			return path;
		}

		std::string getGenericPath(const std::string& _path)
		{
			if (_path.empty())
				return _path;

			std::string path   = _path;
			size_t      offset = std::string::npos;

			// remove "\\\\?\\"
			if(path[0] == '\\' && (path.find("\\\\?\\")) == 0)
				path.erase(0, 4);

			// convert '\\' to '/'
			while ((offset = path.find('\\')) != std::string::npos)
				path[offset] = '/';// .replace(offset, 1, "/");

			// remove double '/'
			while((offset = path.find("//")) != std::string::npos)
				path.erase(offset, 1);

			// remove trailing '/'
			while(path.length() && ((offset = path.find_last_of('/')) == (path.length() - 1)))
				path.erase(offset, 1);

			// return generic path
			return path;
		} // getGenericPath

		std::string getEscapedPath(const std::string& _path)
		{
			std::string path = getGenericPath(_path);

			// insert a backslash before most characters that would mess up a bash path
			const char* invalidChars = "\\ '\"!$^&*(){}[]?;<>";
			const char* invalidChar  = invalidChars;

			while(*invalidChar)
			{
				size_t start  = 0;
				size_t offset = 0;

				while((offset = path.find(*invalidChar, start)) != std::string::npos)
				{
					start = offset + 1;

					if((offset == 0) || (path[offset - 1] != '\\'))
					{
						path.insert(offset, 1, '\\');
						++start;
					}
				}

				++invalidChar;
			}

			// return escaped path
			return path;

		} // getEscapedPath

		std::string getCanonicalPath(const std::string& _path)
		{
			// temporary hack for builtin resources
			if(_path.size() >= 2 && _path[0] == ':' && _path[1] == '/')
				return _path;

			std::string path = exists(_path) ? getAbsolutePath(_path) : getGenericPath(_path);

			// cleanup path
			bool scan = true;
			while(scan)
			{
				stringList pathList = getPathList(path);

				path.clear();
				scan = false;

				for(stringList::const_iterator it = pathList.cbegin(); it != pathList.cend(); ++it)
				{
					// ignore empty
					if((*it).empty())
						continue;

					// remove "/./"
					if((*it) == ".")
						continue;

					// resolve "/../"
					if((*it) == "..")
					{
						path = getParent(path);
						continue;
					}

					// append folder to path
					path += ("/" + (*it));


					// resolve symlink
					if(isSymlink(path))
					{
						std::string resolved = resolveSymlink(path);

						if(resolved.empty())
							return "";

						if(isAbsolute(resolved))
							path = resolved;
						else
							path = getParent(path) + "/" + resolved;

						for(++it; it != pathList.cend(); ++it)
							path += (path.size() == 0) ? (*it) : ("/" + (*it));

						scan = true;
						break;
					}
				}
			}

			// return canonical path
			return path;

		} // getCanonicalPath

		std::string getAbsolutePath(const std::string& _path, const std::string& _base)
		{
			if (_path.empty() || isAbsolute(_path))
				return getGenericPath(_path);

			return getCanonicalPath(_base + "/" + _path);
		} // getAbsolutePath

		std::string getParent(const std::string& _path)
		{
			std::string path   = getGenericPath(_path);
			size_t      offset = std::string::npos;

			// find last '/' and erase it
			if((offset = path.find_last_of('/')) != std::string::npos)
				return path.erase(offset);

			// no parent found
			return path;

		} // getParent

		std::string getFileName(const std::string& _path)
		{
			std::string path   = getGenericPath(_path);
			size_t      offset = std::string::npos;

			// find last '/' and return the filename
			if((offset = path.find_last_of('/')) != std::string::npos)
				return ((path[offset + 1] == 0) ? "." : std::string(path, offset + 1));

			// no '/' found, entire path is a filename
			return path;

		} // getFileName

		std::string getStem(const std::string& _path)
		{
			std::string fileName = getFileName(_path);
			size_t      offset   = std::string::npos;

			// empty fileName
			if(fileName == ".")
				return fileName;

			// find last '.' and erase the extension
			if((offset = fileName.find_last_of('.')) != std::string::npos)
				return fileName.erase(offset);

			// no '.' found, filename has no extension
			return fileName;

		} // getStem

		std::string getExtension(const std::string& _path)
		{
			const char *str = _path.c_str();

			const char *ext;
			if (str && *str != '\0' && ((ext = strrchr(str, '.'))) && strpbrk(ext, "/\\") == nullptr)
				return ext;

			return std::string();
		} // getExtension

		std::string resolveRelativePath(const std::string& _path, const std::string& _relativeTo, const bool _allowHome)
		{
			// nothing to resolve
			if(!_path.length())
				return _path;

			if (_path.length() == 1 && _path[0] == '.')
				return getGenericPath(_relativeTo);

			if ((_path[0] == '.') && (_path[1] == '/' || _path[1] == '\\'))
			{
				if (_path[2] == '.' && (_path[3] == '.' || _path[3] == '/' || _path[3] == '\\')) // ./.. or ././ ?
					return getCanonicalPath(_relativeTo + &(_path[1]));

				return getGenericPath(_relativeTo + &(_path[1]));
			}

			// replace '~' with homePath
			if(_allowHome && (_path[0] == '~') && (_path[1] == '/' || _path[1] == '\\'))
				return getCanonicalPath(getHomePath() + &(_path[1]));

			// nothing to resolve
			return getGenericPath(_path);

		} // resolveRelativePath

		std::string createRelativePath(const std::string& _path, const std::string& _relativeTo, const bool _allowHome)
		{
			if (_relativeTo.empty())
				return _path;

			if (_path == _relativeTo)
				return "";

			bool        contains = false;
			std::string path     = removeCommonPath(_path, _relativeTo, contains);

			if(contains)
			{
				// success
				return ("./" + path);
			}

			if(_allowHome)
			{
				path = removeCommonPath(_path, getHomePath(), contains);

				if(contains)
				{
					// success
					return ("~/" + path);
				}
			}

			// nothing to resolve
			return path;

		} // createRelativePath

		std::string removeCommonPath(const std::string& _path, const std::string& _common, bool& _contains)
		{
			std::string path = _path; // getGenericPath(_path);
			//std::string common = isDirectory(_common) ? getGenericPath(_common) : getParent(_common);

			// check if path contains common
			if(path.find(_common) == 0 && path != _common)
			{
				_contains = true;
				int trailingSlash = _common.find_last_of('/') == (_common.length() - 1) ? 0 : 1;
				return path.substr(_common.length() + trailingSlash);
			}

			// it didn't
			_contains = false;
			return path;

		} // removeCommonPath

		std::string resolveSymlink(const std::string& _path)
		{
			std::string path = getGenericPath(_path);
			std::string resolved;

			struct stat info;

			// check if lstat succeeded
			if(lstat(path.c_str(), &info) == 0)
			{
				resolved.resize(info.st_size);
				if(readlink(path.c_str(), (char*)resolved.data(), resolved.size()) > 0)
					resolved = getGenericPath(resolved);
			}

			// return resolved path
			return resolved;

		} // resolveSymlink

		bool removeFile(const std::string& _path)
		{
			std::string path = getGenericPath(_path);

			// don't remove if it doesn't exists
			if(!exists(path))
				return true;

			// try to remove file
			return (unlink(path.c_str()) == 0);

		} // removeFile

		bool copyFile(const std::string src, const std::string dst)
		{
			std::string path = getGenericPath(src);
			std::string pathD = getGenericPath(dst);

			// don't remove if it doesn't exists
			if (!exists(path))
				return true;

			char buf[512];
			size_t size;

			FILE* source = fopen(path.c_str(), "rb");
			if (source == nullptr)
				return false;

			FILE* dest = fopen(pathD.c_str(), "wb");
			if (dest == nullptr)
			{
				fclose(source);
				return false;
			}

			while (size = fread(buf, 1, 512, source))
				fwrite(buf, 1, size, dest);			

			fclose(dest);
			fclose(source);

			return true;
		} // removeFile

		bool createDirectory(const std::string& _path)
		{
			FileCache::resetCache();

			std::string path = getGenericPath(_path);

			// don't create if it already exists
			if(exists(path))
				return true;

			// try to create directory
			if(mkdir(path.c_str(), 0755) == 0)
				return true;

			// failed to create directory, try to create the parent
			std::string parent = getParent(path);

			// only try to create parent if it's not identical to path
			if(parent != path)
				createDirectory(parent);

			// try to create directory again now that the parent should exist
			return (mkdir(path.c_str(), 0755) == 0);

		} // createDirectory

		bool removeDirectory(const std::string& _path)
		{
			std::string path = getGenericPath(_path);

			// don't remove if it doesn't exists
			if (!exists(path))
				return true;

			return (rmdir(path.c_str()) == 0);
		} // removeDirectory

		bool exists(const std::string& _path)
		{
			if (_path.empty())
				return false;

			auto it = FileCache::get(_path);
			if (it != nullptr)
				return it->exists;

			std::string path = getGenericPath(_path);
			struct stat64 info;
			
			// check if stat64 succeeded
			return FileCache::fromStat64(path, &info) == 0;
		} // exists

		size_t getFileSize(const std::string& _path)
		{
			if (!exists(_path))
				return 0;

			std::string path = getGenericPath(_path);
			struct stat64 info;

			// check if stat64 succeeded
			if ((stat64(path.c_str(), &info) == 0))
				return (size_t) info.st_size;

			return 0;
		}

		Utils::Time::DateTime getFileCreationDate(const std::string& _path)
		{
			std::string path = getGenericPath(_path);
			struct stat64 info;

			// check if stat64 succeeded
			if ((stat64(path.c_str(), &info) == 0))
				return Utils::Time::DateTime(info.st_ctime);

			return Utils::Time::DateTime();
		}

		Utils::Time::DateTime getFileModificationDate(const std::string& _path)
		{
			std::string path = getGenericPath(_path);
			struct stat64 info;

			// check if stat64 succeeded
			if ((stat64(path.c_str(), &info) == 0))
				return Utils::Time::DateTime(info.st_mtime);

			return Utils::Time::DateTime();
		}

		bool isAbsolute(const std::string& _path)
		{
			if (_path.size() >= 2 && _path[0] == ':' && _path[1] == '/')
				return true;

			std::string path = getGenericPath(_path);
			return ((path.size() > 0) && (path[0] == '/'));

		} // isAbsolute

		bool isRegularFile(const std::string& _path)
		{
			auto it = FileCache::get(_path);
			if (it != nullptr)
				return it->exists && !it->directory && !it->isSymLink;

			std::string path = getGenericPath(_path);
			struct stat64 info;

			// check if stat64 succeeded
			if (FileCache::fromStat64(path, &info) != 0) //if(stat64(path.c_str(), &info) != 0)				
				return false;

			// check for S_IFREG attribute
			return (S_ISREG(info.st_mode));

		} // isRegularFile

		bool isDirectory(const std::string& _path)
		{
			auto it = FileCache::get(_path);
			if (it != nullptr && !it->isSymLink)
				return it->exists && it->directory;

			std::string path = getGenericPath(_path);
			struct stat64 info;

			// check if stat succeeded
			if (FileCache::fromStat64(path, &info) != 0) //if(stat64(path.c_str(), &info) != 0)
				return false;

			// check for S_IFDIR attribute
			return (S_ISDIR(info.st_mode));
		} // isDirectory

		bool isSymlink(const std::string& _path)
		{		
			auto it = FileCache::get(_path);
			if (it != nullptr)
				return it->exists && it->isSymLink;
				
			std::string path = getGenericPath(_path);

			struct stat64 info;

			// check if lstat succeeded
			if (FileCache::fromStat64(path, &info) != 0) //if(stat64(path.c_str(), &info) != 0)			
				return false;

			// check for S_IFLNK attribute
			return (S_ISLNK(info.st_mode));

		} // isSymlink

		bool isHidden(const std::string& _path)
		{
			auto it = FileCache::get(_path);
			if (it != nullptr)
				return it->exists && it->hidden;

			std::string path = getGenericPath(_path);

			// filenames starting with . are hidden in linux, we do not do this check for windows as well
			if (getFileName(path)[0] == '.')
				return true;

			// not hidden
			return false;

		} // isHidden


		std::string combine(const std::string& _path, const std::string& filename)
		{
			std::string gp = getGenericPath(_path);
			
			if (Utils::String::startsWith(filename, "/.."))
			{
				auto f = getPathList(filename);

				int count = 0;
				for (auto it = f.cbegin(); it != f.cend(); ++it)
				{
					if (*it != "..")
						break;

					count++;
				}

				if (count > 0)
				{
					auto list = getPathList(gp);
					std::vector<std::string> p(list.begin(), list.end());

					std::string result;

					for (int i = 0; i < p.size() - count; i++)
					{
						if (result.empty())
							result = p.at(i);
						else
							result = result + "/" + p.at(i);
					}

					std::vector<std::string> fn(f.begin(), f.end());
					for (int i = count; i < fn.size(); i++)
					{
						if (result.empty())
							result = fn.at(i);
						else
							result = result + "/" + fn.at(i);
					}

					return result;
				}
			} // combine


			if (!Utils::String::endsWith(gp, "/") && !Utils::String::endsWith(gp, "\\"))
				if (!Utils::String::startsWith(filename, "/") && !Utils::String::startsWith(filename, "\\"))
					gp += "/";

			return gp + filename;
		}

		std::string readAllText(const std::string fileName)
		{
			std::ifstream t(fileName);
			std::stringstream buffer;
			buffer << t.rdbuf();
			return buffer.str();
		} // readAllText

		void writeAllText(const std::string fileName, const std::string text)
		{
			std::fstream fs;
			fs.open(fileName.c_str(), std::fstream::out);
			fs << text;
			fs.close();
		}  // writeAllText


		std::string megaBytesToString(unsigned long size)
		{
			static const char *SIZES[] = { "MB", "GB", "TB" };
			int div = 0;
			unsigned long rem = 0;

			while (size >= 1024 && div < (sizeof SIZES / sizeof *SIZES))
			{
				rem = (size % 1024);
				div++;
				size /= 1024;
			}

			double size_d = (float)size + (float)rem / 1024.0;

			std::ostringstream out;
			out.precision(2);
			out << std::fixed << size_d << " " << SIZES[div];
			return out.str();
		} // megaBytesToString

		bool renameFile(const std::string src, const std::string dst, bool overWrite)
		{
			std::string path = getGenericPath(src);
			if (!exists(path))
				return true;

			if (overWrite && Utils::FileSystem::exists(dst))
				Utils::FileSystem::removeFile(dst);

			return std::rename(src.c_str(), dst.c_str()) == 0;
		} // renameFile

		std::string getTempPath()
		{
			static std::string path;

			if (path.empty())
			{
					path = Utils::FileSystem::getGenericPath(Utils::FileSystem::getEsConfigPath() + "/tmp");
			}

			if (!Utils::FileSystem::isDirectory(path))
				Utils::FileSystem::createDirectory(path);

			return path;
		} // getTempPath

		void deleteDirectoryFiles(const std::string path, bool deleteDirectory)
		{
			std::vector<std::string> directories;

			auto files = Utils::FileSystem::getDirContent(path, true, true);
			std::reverse(std::begin(files), std::end(files));
			for (auto file : files)
			{
				if (Utils::FileSystem::isDirectory(file))
					directories.push_back(file);
				else
					Utils::FileSystem::removeFile(file);
			}

			for (auto file : directories)
				removeDirectory(file);

			if (deleteDirectory)
				removeDirectory(path);
		} // deleteDirectoryFiles

		std::string getFileCrc32(const std::string& filename)
		{
			std::string hex;

			FILE* file = fopen(filename.c_str(), "rb");
			if (file)
			{
				#define CRCBUFFERSIZE 64 * 1024
				char* buffer = new char[CRCBUFFERSIZE];
				if (buffer)
				{
					size_t size;

					unsigned int file_crc32 = 0;

					while (size = fread(buffer, 1, CRCBUFFERSIZE, file))
						file_crc32 = Utils::Zip::ZipFile::computeCRC(file_crc32, buffer, size);

					hex = Utils::String::toHexString(file_crc32);

					delete buffer;
				}

				fclose(file);
			}

			return hex;
		} // getFileCrc32

		std::string getFileMd5(const std::string& filename)
		{
			std::string hex;

			FILE* file = fopen(filename.c_str(), "rb");
			if (file)
			{
				#define CRCBUFFERSIZE 64 * 1024
				char* buffer = new char[CRCBUFFERSIZE];

				if (buffer)
				{
					MD5 md5;

					size_t size;
					while (size = fread(buffer, 1, CRCBUFFERSIZE, file))
						md5.update(buffer, size);

					md5.finalize();
					hex = md5.hexdigest();

					delete buffer;
				}

				fclose(file);
			}

			return hex;
		} // getFileMd5

	} // FileSystem::

} // Utils::
