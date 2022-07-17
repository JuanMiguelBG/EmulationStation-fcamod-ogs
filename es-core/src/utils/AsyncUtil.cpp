#include "utils/AsyncUtil.h"

#include <thread>
#include "Settings.h"
#include <unistd.h>


namespace Utils
{
	namespace Async
	{

		bool isCanRunAsync()
		{
			return (std::thread::hardware_concurrency() > 2) && Settings::getInstance()->getBool("ThreadedLoading");
		}

		void sleep(int milliseconds)
		{
			usleep(milliseconds * 1000);
		}

	} // Async::

} // Utils::
