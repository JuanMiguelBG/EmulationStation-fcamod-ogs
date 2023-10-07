#include "EsLocale.h"
#include "resources/ResourceManager.h"
#include "Settings.h"
#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"

#include <fstream>

std::map<std::string, std::string> EsLocale::mItems;
std::string EsLocale::mCurrentLanguage = "en";
std::string EsLocale::mDateFormat = "%d/%m/%Y"; // ISO format
bool EsLocale::mCurrentLanguageLoaded = true; // By default, 'en' is considered loaded

// List of all possible plural forms here
// https://github.com/translate/l10n-guide/blob/master/docs/l10n/pluralforms.rst
// Test rules without spaces & without parenthesis - there are 19 distinct rules

PluralRule rules[] = {
	{ "en", "n!=1", [](int n) { return n != 1 ? 1 : 0; } },
	{ "fr", "n>1", [](int n) { return n>1 ? 1 : 0; } },
	{ "jp", "0", [](int n) { return 0; } },
	{ "ru", "n%10==1&&n%100!=11?0:n%10>=2&&n%10<=4&&n%100<10||n%100>=20?1:2",  [](int n) { return n % 10 == 1 && n % 100 != 11 ? 0 : n % 10 >= 2 && n % 10 <= 4 && (n % 100 < 10 || n % 100 >= 20) ? 1 : 2; } },
	{ "ar", "n==0?0:n==1?1:n==2?2:n%100>=3&&n%100<=10?3:n%100>=11?4:5", [](int n) { return n == 0 ? 0 : n == 1 ? 1 : n == 2 ? 2 : n % 100 >= 3 && n % 100 <= 10 ? 3 : n % 100 >= 11 ? 4 : 5; } },
	{ "pl", "n==1?0:n%10>=2&&n%10<=4&&n%100<10||n%100>=20?1:2", [](int n) { return n == 1 ? 0 : n % 10 >= 2 && n % 10 <= 4 && (n % 100<10 || n % 100 >= 20) ? 1 : 2; } },
	{ "ga", "n==1?0:n==2?1:n>2&&n<7?2:n>6&&n<11?3:4", [](int n) { return n == 1 ? 0 : n == 2 ? 1 : (n>2 && n<7) ? 2 : (n>6 && n<11) ? 3 : 4; } },
	{ "gd", "n==1||n==11?0:n==2||n==12?1:n>2&&n<20?2:3", [](int n) { return (n == 1 || n == 11) ? 0 : (n == 2 || n == 12) ? 1 : (n>2 && n<20) ? 2 : 3; } },
	{ "mk", "n==1||n%10==1?0:1", [](int n) { return n == 1 || n % 10 == 1 ? 0 : 1; } },
	{ "is", "n%10!=1||n%100==11", [](int n) { return (n % 10 != 1 || n % 100 == 11) ? 1 : 0; } },
	{ "lv", "n%10==1&&n%100!=11?0:n!=0?1:2", [](int n) { return n % 10 == 1 && n % 100 != 11 ? 0 : n != 0 ? 1 : 2; } },
	{ "lt", "n%10==1&&n%100!=11?0:n%10>=2&&n%100<10||n%100>=20?1:2", [](int n) { return n % 10 == 1 && n % 100 != 11 ? 0 : n % 10 >= 2 && (n % 100<10 || n % 100 >= 20) ? 1 : 2; } },
	{ "mn", "n==0?0:n==1?1:2", [](int n) { return n == 0 ? 0 : n == 1 ? 1 : 2; } },
	{ "ro", "n==1?0:n==0||n%100>0&&n%100<20?1:2", [](int n) { return n == 1 ? 0 : (n == 0 || (n % 100>0 && n % 100<20)) ? 1 : 2; } },
	{ "cs", "n==1?0:n>=2&&n<=4?1:2", [](int n) { return (n == 1) ? 0 : (n >= 2 && n <= 4) ? 1 : 2; } },
	{ "sl", "n%100==1?0:n%100==2?1:n%100==3||n%100==4?2:3", [](int n) { return n % 100 == 1 ? 0 : n % 100 == 2 ? 1 : n % 100 == 3 || n % 100 == 4 ? 2 : 3; } },
	{ "mt", "n==1?0:n==0||n%100>1&&n%100<11?1:n%100>10&&n%100<20?2:3", [](int n) { return n == 1 ? 0 : n == 0 || (n % 100>1 && n % 100<11) ? 1 : (n % 100>10 && n % 100<20) ? 2 : 3; } },
	{ "cy", "n==1?0:n==2?1:n!=8&&n!=11?2:3", [](int n) { return (n == 1) ? 0 : (n == 2) ? 1 : (n != 8 && n != 11) ? 2 : 3; } },
	{ "kw", "n==1?0:n==2?1:n==3?2:3", [](int n) { return (n == 1) ? 0 : (n == 2) ? 1 : (n == 3) ? 2 : 3; } }
};

PluralRule EsLocale::mPluralRule = rules[0];

const std::string EsLocale::getText(const std::string msgid)
{
	std::string hascode = std::to_string(std::hash<std::string>{}(msgid));

	checkLocalisationLoaded();

	auto item = mItems.find(hascode);
	if (item != mItems.cend())
		return item->second;

	return msgid;
}

const std::string EsLocale::nGetText(const std::string msgid, const std::string msgid_plural, int n)
{
	if (mCurrentLanguage.empty() || mCurrentLanguage == "en") // English default
		return n != 1 ? msgid_plural : msgid;

	if (mPluralRule.rule.empty())
		return n != 1 ? getText(msgid_plural) : getText(msgid);

	checkLocalisationLoaded();

	int pluralId = mPluralRule.evaluate(n);
	if (pluralId == 0)
		return getText(msgid);

	std::string hascode = std::to_string(std::hash<std::string>{}(msgid_plural));

	auto item = mItems.find(std::to_string(pluralId) + "@" + hascode);
	if (item != mItems.cend())
		return item->second;

	item = mItems.find(hascode);
	if (item != mItems.cend())
		return item->second;

	return msgid_plural;
}

const bool EsLocale::isRTL()
{
	std::string language = Settings::getInstance()->getString("Language");
	return language.find("ar") == 0 || language.find("he") == 0;
}

const std::vector<PluralRule> pluralRules(rules, rules + sizeof(rules) / sizeof(rules[0]));

void EsLocale::checkLocalisationLoaded()
{
	if (mCurrentLanguageLoaded)
	{
		if (Settings::getInstance()->getString("Language") == mCurrentLanguage)
			return;

		mCurrentLanguage = Settings::getInstance()->getString("Language");
	}

	mCurrentLanguageLoaded = true;
	mPluralRule = rules[0];
	mItems.clear();

	std::string xmlpath = ResourceManager::getInstance()->getResourcePath(":/locale/" + mCurrentLanguage + "/emulationstation2.po");
	if (!Utils::FileSystem::exists(xmlpath))
		xmlpath = ResourceManager::getInstance()->getResourcePath(":/locale/" + mCurrentLanguage + "/LC_MESSAGES/emulationstation2.po");

	if (!Utils::FileSystem::exists(xmlpath))
	{
		auto shortNameDivider = mCurrentLanguage.find("_");
		if (shortNameDivider != std::string::npos)
		{
			auto shortName = mCurrentLanguage.substr(0, shortNameDivider);

			xmlpath = ResourceManager::getInstance()->getResourcePath(":/locale/" + shortName + "/emulationstation2.po");
			if (!Utils::FileSystem::exists(xmlpath))
				xmlpath = ResourceManager::getInstance()->getResourcePath(":/locale/" + shortName + "/LC_MESSAGES/emulationstation2.po");
		}
	}

	std::string msgid;
	std::string msgid_plural;

	std::string line;
	std::string line_next;
	std::ifstream file(xmlpath);

	if (file.is_open())
	{
		while (!file.eof())
		{
			if (!line_next.empty())
			{
				line = line_next;
				line_next.clear();
			}
			else
			{
				std::getline(file, line);
				if (file.eof())
				{
					break;
				}
			}
			if (line.find("\"Plural-Forms:") == 0)
			{
				auto start = line.find("plural=");
				if (start != std::string::npos)
				{
					std::string plural;

					auto end = line.find_last_of(';');
					if (end == std::string::npos)
					{
						plural = line.substr(start + 7, line.size() - start - 7 - 1);

						std::getline(file, line);
						end = line.find_last_of(';');
						if (end != std::string::npos)
							plural += line.substr(1, end - 1);
					}
					else
						plural = line.substr(start + 7, end - start - 7);

					plural = Utils::String::replace(plural, " ", "");

					if (Utils::String::endsWith(plural, ";"))
						plural = plural.substr(0, plural.size() - 1);

				//	if (Utils::String::startsWith(plural, "(") && Utils::String::endsWith(plural, ")"))
				//		plural = plural.substr(1, plural.size() - 2);
					plural = Utils::String::replace(plural, "(", "");
					plural = Utils::String::replace(plural, ")", "");

					for (auto iter = pluralRules.cbegin(); iter != pluralRules.cend(); iter++)
					{
						if (plural == iter->rule)
						{
							mPluralRule = *iter;
							break;
						}
					}
				}
			}
			if (line.find("\"Date-Format:") == 0)
			{
				auto start = line.find_first_of(' ');
				if (start != std::string::npos && !msgid.empty())
				{
					auto end = line.find_last_of('"');
					if (end != std::string::npos)
						mDateFormat = line.substr(start + 1, end - start - 1);
				}
			}
			else if (line.find("msgid_plural") == 0)
			{
				auto start = line.find('"');
				if (start != std::string::npos && !msgid.empty())
				{
					auto end = line.find_last_of('"');
					if (end != std::string::npos)
					{
						msgid_plural = line.substr(start + 1, end - start - 1);
						readFileSplitedValues(file, msgid_plural, line_next);
					}
				}
			}
			else if (line.find("msgid") == 0)
			{
				msgid = "";
				msgid_plural = "";

				auto start = line.find('"');
				if (start != std::string::npos)
				{
					auto end = line.find_last_of('"');
					if (end != std::string::npos)
					{
						msgid = line.substr(start + 1, end - start - 1);
						readFileSplitedValues(file, msgid, line_next);
					}
				}
			}
			else if (line.find("msgstr") == 0)
			{
				std::string idx;

				if (!msgid_plural.empty())
				{
					auto idxStart = line.find('[');
					if (idxStart != std::string::npos)
					{
						auto idxEnd = line.find(']', idxStart + 1);
						if (idxEnd != std::string::npos)
							idx = line.substr(idxStart + 1, idxEnd - idxStart - 1);
					}
				}

				auto start = line.find('"');
				if (start != std::string::npos)
				{
					auto end = line.find_last_of('"');
					if (end != std::string::npos)
					{
						std::string msgstr = line.substr(start + 1, end - start - 1);
						readFileSplitedValues(file, msgstr, line_next);
						std::string msgstr_aux = Utils::String::hiddenSpecialCharacters(msgstr);
						if (!msgid.empty() && !msgstr.empty())
						{
							std::string msgid_aux = Utils::String::hiddenSpecialCharacters(msgid);
							std::string hascode = std::to_string(std::hash<std::string>{}(msgid_aux));

							if (idx.empty() || idx == "0")
								mItems[hascode] = msgstr_aux;
						}
						if (!msgid_plural.empty() && !msgstr.empty())
						{
							std::string msgid_plural_aux = Utils::String::hiddenSpecialCharacters(msgid_plural);
							std::string hascode = std::to_string(std::hash<std::string>{}(msgid_plural_aux));

							if (!idx.empty() && idx != "0")
								mItems[idx + "@" + hascode] = msgstr_aux;
							else
								mItems[hascode] = msgstr_aux;
						}
					}
				}
			}
		}

		file.close();
	}

}

const void EsLocale::readFileSplitedValues(std::ifstream &file, std::string &msg, std::string &line_next)
{
	std::string line;

	while (std::getline(file, line))
	{
		line = Utils::String::trim(line);

		if (Utils::String::startsWith(line, "\""))
		{
			auto start = 1;
			auto end = line.find_last_of('"');
			if (end != std::string::npos)
				msg.append(line.substr(start, end - 1));
		}
		else
		{
			line_next.append(line);
			line.clear();
			break;
		}
	}
}
