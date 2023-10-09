#include "utils/AsyncUtil.h"

#include <thread>
#include "Settings.h"
#include <unistd.h>
#include <future>
#include "Log.h"
#include "make_unique.hh"


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
			runScheduled(0, asyncFunction);
		}

		void runScheduled(int seconds, const std::function<void()>& asyncFunction)
		{
			if (asyncFunction == nullptr)
				return;

			std::string threadId = std::to_string(getThreadId());
			if (Utils::Async::isCanRunAsync())
			{
				LOG(LogDebug) << "Utils::Async::runScheduled() - Asynchronous execution, thread: '" << threadId << "', launch after " << seconds << " seconds!";
				// trick to run asynchronous and forget
				std::make_unique<std::future<void>*>(new auto(std::async(std::launch::async, [seconds, asyncFunction]
					{
						LOG(LogDebug) << "Utils::Async::runScheduled() - INSIDE Asynchronous execution, thread: '" << std::to_string(Utils::Async::getThreadId()) << "', launch after " << seconds << " seconds!";
						if (seconds > 0)
							sleep(seconds * 1000);

						asyncFunction();

						LOG(LogDebug) << "Utils::Async::runScheduled() - INSIDE Asynchronous execution, thread: '" << std::to_string(Utils::Async::getThreadId()) << "', launched after " << seconds << " seconds!";
					}))).reset();
				LOG(LogDebug) << "Utils::Async::runScheduled() - exit Asynchronous execution, thread: '" << threadId << "', launch after " << seconds << " seconds!";
			}
			else
			{
				LOG(LogDebug) << "Utils::Async::runScheduled() - synchronous execution, thread: '" << threadId << "', launch after " << seconds << " seconds!";
				if (seconds > 0)
					sleep(seconds * 1000);

				asyncFunction();
				LOG(LogDebug) << "Utils::Async::runScheduled() - synchronous execution, thread: '" << threadId << "', launched after " << seconds << " seconds!";
			}

		}

		unsigned int getThreadId()
		{
			std::thread::id threadId = std::this_thread::get_id();
			return *static_cast<unsigned int*>(static_cast<void*>(&threadId));
		}

	} // Async::

} // Utils::
