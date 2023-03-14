#pragma once
#ifndef ES_CORE_UTILS_ASYNC_UTIL_H
#define ES_CORE_UTILS_ASYNC_UTIL_H

#include <functional>

namespace Utils
{
	namespace Async
	{
		bool isCanRunAsync();

		void sleep(int milliseconds);

		void run(const std::function<void()>& asyncFunction);

		unsigned int getThreadId();
	} // Async::

} // Utils::

#endif // ES_CORE_UTILS_STRING_UTIL_H
