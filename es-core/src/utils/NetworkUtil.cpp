#include "utils/NetworkUtil.h"

#include "utils/StringUtil.h"

namespace Utils
{
	namespace Network
	{

		bool validateIPv4(const std::string &ip)
		{
			// split the string into tokens
			std::vector<std::string> list = Utils::String::split(ip, '.', true);

			// if the token size is not equal to four
			if (list.size() != 4) {
				return false;
			}

			// validate each token
			for (std::string str: list)
			{
				// verify that the string is a number or not, and the numbers are in the valid range
				if (!Utils::String::isNumber(str) || std::stoi(str) > 255 || std::stoi(str) < 0)
				{
					return false;
				}
			}

			return true;
		}

	} // Network::

} // Utils::