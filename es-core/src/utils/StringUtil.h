#pragma once
#ifndef ES_CORE_UTILS_STRING_UTIL_H
#define ES_CORE_UTILS_STRING_UTIL_H

#include <string>
#include <vector>

namespace Utils
{
	namespace String
	{
		typedef std::vector<std::string> stringVector;

		unsigned int chars2Unicode      (const std::string& _string, size_t& _cursor);
		std::string  unicode2Chars      (const unsigned int _unicode);
		size_t       nextCursor         (const std::string& _string, const size_t _cursor);
		size_t       prevCursor         (const std::string& _string, const size_t _cursor);
		size_t       moveCursor         (const std::string& _string, const size_t _cursor, const int _amount);
		std::string  toLower            (const std::string& _string);
		std::string  toUpper            (const std::string& _string);
		std::string  startWithUpper(const std::string& _string);
		std::string  trim               (const std::string& _string);
		std::string  replace            (const std::string& _string, const std::string& _replace, const std::string& _with);
		bool         startsWith         (const std::string& _string, const std::string& _start);
		bool         endsWith           (const std::string& _string, const std::string& _end);
		std::string  removeParenthesis  (const std::string& _string);
		stringVector commaStringToVector(const std::string& _string);
		std::string  vectorToCommaString(stringVector _vector);
		std::string  format             (const char* _string, ...);
		std::string  scramble           (const std::string& _input, const std::string& key);

		std::vector<std::string> split  (const std::string& s, char seperator, bool removeEmptyEntries = false);
		std::vector<std::string> splitAny(const std::string& s, const std::string& separator, bool removeEmptyEntries = false);
		std::vector<std::string> extractStrings(const std::string& _string, const std::string& startDelimiter, const std::string& endDelimiter);

		std::string join(const std::vector<std::string>& items, std::string separator);
		int			compareIgnoreCase(const std::string& name1, const std::string& name2);
		std::string proper(const std::string& _string);
		std::string removeHtmlTags(const std::string& html);
		bool		equalsIgnoreCase(const std::string & _string, const std::string & _what);
		bool		containsIgnoreCase(const std::string & _string, const std::string & _what);
		bool		startsWithIgnoreCase(const std::string& name1, const std::string& name2);

		int			toInteger(const std::string& string);
		float		toFloat(const std::string& string);

		std::string decodeXmlString(const std::string& string);
		std::string toHexString(unsigned int color);
		unsigned int fromHexString(const std::string& string);

		const std::string hiddenSpecialCharacters(const std::string msg);
		const std::string showSpecialCharacters(const std::string msg);
		const std::string boolToString(bool value, bool uppercase = false);
		bool toBool(const std::string value);
		bool isNumber(const std::string &str);
	} // String::

} // Utils::

#define WINSTRINGW(x) x

#endif // ES_CORE_UTILS_STRING_UTIL_H
