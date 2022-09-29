#include "Scripting.h"
#include "Log.h"
#include "platform.h"
#include "utils/FileSystemUtil.h"

namespace Scripting
{
	int fireEvent(const std::string& eventName, const std::string& arg1, const std::string& arg2, const std::string& arg3, const std::string& arg4)
	{
		LOG(LogDebug) << "fireEvent: " << eventName << " " << arg1 << " " << arg2 << "' '" << arg3 << "' '" << arg4;

		std::list<std::string> scriptDirList;
		std::string test;

		// check in exepath
		test = Utils::FileSystem::getExePath() + "/scripts/" + eventName;
		if(Utils::FileSystem::exists(test))
			scriptDirList.push_back(test);

		// check in Es config path
		test = Utils::FileSystem::getEsConfigPath() + "/scripts/" + eventName;
		if(Utils::FileSystem::exists(test))
			scriptDirList.push_back(test);

		int ret = 0;
		// loop over found script paths per event and over scripts found in eventName folder.
		for(std::list<std::string>::const_iterator dirIt = scriptDirList.cbegin(); dirIt != scriptDirList.cend(); ++dirIt)
		{
			std::list<std::string> scripts = Utils::FileSystem::getDirContent(*dirIt);
			for (std::list<std::string>::const_iterator it = scripts.cbegin(); it != scripts.cend(); ++it)
			{
				std::string script = *it;
				if (arg1.length() > 0)
				{
					script += " \"" + arg1 + "\"";
					if (arg2.length() > 0)
					{
						script += " \"" + arg2 + "\"";
						if (arg3.length() > 0)
						{
							script += " \"" + arg3 + "\"";
							if (arg4.length() > 0)
							{
								script += " \"" + arg4 + "\"";
							}
						}
					}
				}

				LOG(LogDebug) << "Scripting::fireEvent() - executing: " << script;
				ret = runSystemCommand(script, "", NULL);
				if (ret != 0)
					LOG(LogWarning) << "Scripting::fireEvent() - executing: \"" << script << "\" failed with exit code != 0. Terminating processing for this event.";
			}
		}
		return ret;
	}

} // Scripting::
