#include "utils/AsyncUtil.h"

#include <thread>
#include "Settings.h"
#include <unistd.h>
#include <future>
#include "Log.h"


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
			std::string threadId = std::to_string(getThreadId());
			LOG(LogDebug) << "Utils::Async::sleep() - sleeping thread '" << threadId << "' since '" << std::to_string(milliseconds) << "' milliseconds!";
			usleep(milliseconds * 1000);
			LOG(LogDebug) << "Utils::Async::sleep() - waking up thread '" << threadId << "'!";
		}

		void run(const std::function<void()>& asyncFunction)
		{
			if (asyncFunction == nullptr)
				return;

			std::string threadId = std::to_string(getThreadId());
			if (Utils::Async::isCanRunAsync())
			{
				LOG(LogDebug) << "Utils::Async::run() - Asynchronous execution, thread: '" << threadId << "'!";
				auto dummy = std::async(std::launch::async, [asyncFunction]
					{
						LOG(LogDebug) << "Utils::Async::run() - INSIDE Asynchronous execution, thread: '" << std::to_string(Utils::Async::getThreadId()) << "'!";
						asyncFunction();
					});
				LOG(LogDebug) << "Utils::Async::run() - exit Asynchronous execution, thread: '" << threadId << "'!";
			}
			else
			{
				LOG(LogDebug) << "Utils::Async::run() - normal execution, thread: '" << threadId << "'!";
				asyncFunction();
			}
		}

		unsigned int getThreadId()
		{
			std::thread::id threadId = std::this_thread::get_id();
			return *static_cast<unsigned int*>(static_cast<void*>(&threadId));
		}

	} // Async::

} // Utils::
