#include "Scripting.h"
#include "Log.h"
#include "platform.h"
#include "utils/FileSystemUtil.h"
#include "utils/AsyncUtil.h"

namespace Scripting
{
	int fireEvent(const std::string& eventName, const std::string& arg1, const std::string& arg2, const std::string& arg3, const std::string& arg4, const std::string& arg5, const std::string& arg6)
	{
		LOG(LogInfo) << "Scripting::fireEvent() - name: '" << eventName << "', arg1: '" << arg1 << "', arg2: '" << arg2 << "', arg3: '" << arg3 << "', arg4: '" << arg4 << "', arg5: '" << arg5 << "', arg6: '" << arg6 << "'";
		std::list<std::string> scriptDirList;
		std::string test;

		// check in exepath
		test = Utils::FileSystem::getExePath() + "/scripts/" + eventName;
		if (Utils::FileSystem::exists(test))
			scriptDirList.push_back(test);

		// check in ES config path
		test = Utils::FileSystem::getEsConfigPath() + "/scripts/" + eventName;
		if (Utils::FileSystem::exists(test))
			scriptDirList.push_back(test);

		int ret = 0;
		// loop over found script paths per event and over scripts found in eventName folder.
		for(std::list<std::string>::const_iterator dirIt = scriptDirList.cbegin(); dirIt != scriptDirList.cend(); ++dirIt)
		{
			std::list<std::string> scripts = Utils::FileSystem::getDirContent(*dirIt);
			for (std::list<std::string>::const_iterator it = scripts.cbegin(); it != scripts.cend(); ++it)
			{
				if (!Utils::FileSystem::isExecutable(*it)) {
					LOG(LogWarning) << "Scripting::fireEvent() - executing: '" << *it << "' is not executable. Review file permissions.";
					continue;
				}

				std::string script = *it;

				for (auto arg : { arg1, arg2, arg3, arg4, arg5, arg6 })
				{
					if (arg.empty())
						break;

					script += " \"" + arg + "\"";
				}

				LOG(LogDebug) << "Scripting::fireEvent() - executing: " << script;
				ret = runSystemCommand(script, "", NULL);
				if (ret != 0)
					LOG(LogWarning) << "Scripting::fireEvent() - executing: \"" << script << "\" failed with exit code != 0. Terminating processing for this event.";
			}
		}
		return ret;
	}

	void fireAsyncEvent(const std::string& eventName, const std::string& arg1, const std::string& arg2, const std::string& arg3, const std::string& arg4, const std::string& arg5, const std::string& arg6)
	{
		LOG(LogInfo) << "Scripting::fireAsyncEvent() - name: '" << eventName << "', arg1: '" << arg1 << "', arg2: '" << arg2 << "', arg3: '" << arg3 << "', arg4: '" << arg4 << "', arg5: '" << arg5 << "', arg6: '" << arg6 << "'";
		Utils::Async::run([eventName, arg1, arg2, arg3, arg4, arg5, arg6] (void)
		{
			return fireEvent(eventName, arg1, arg2, arg3, arg4, arg5, arg6);
		});
	}

} // Scripting::
