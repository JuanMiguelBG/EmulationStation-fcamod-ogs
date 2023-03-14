#pragma once
#ifndef ES_CORE_UTILS_NETWORK_UTIL_H
#define ES_CORE_UTILS_NETWORK_UTIL_H

#include <string>

namespace Utils
{
	namespace Network
	{

		// Function to validate an IP address
		bool validateIPv4(const std::string &ip);

	} // Network::

} // Utils::

#endif // ES_CORE_UTILS_NETWORK_UTIL_H