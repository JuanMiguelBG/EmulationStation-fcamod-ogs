#include <exception>
#include <map>

#include "scrapers/GamesDBJSONScraper.h"
#include "scrapers/GamesDBJSONScraperResources.h"

#ifdef GAMESDB_APIKEY

#include "FileData.h"
#include "Log.h"
#include "PlatformId.h"
#include "Settings.h"
#include "SystemData.h"
#include "utils/TimeUtil.h"
#include <pugixml/src/pugixml.hpp>

/* When raspbian will get an up to date version of rapidjson we'll be
   able to have it throw in case of error with the following:
#ifndef RAPIDJSON_ASSERT
#define RAPIDJSON_ASSERT(x)                                                    \
  if (!(x)) {                                                                  \
	throw std::runtime_error("rapidjson internal assertion failure: " #x);     \
  }
#endif // RAPIDJSON_ASSERT
*/

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

using namespace PlatformIds;
using namespace rapidjson;

namespace
{
TheGamesDBJSONRequestResources resources;
}

const std::map<PlatformId, std::string> gamesdb_new_platformid_map{
	{ THREEDO, "25" },
	{ AMIGA, "4911" },
	{ AMSTRAD_CPC, "4914" },
	{ APPLE_II, "4942" },
	{ ATARI_800, "4943" },
	{ ATARI_2600, "22" },
	{ ATARI_5200, "26" },
	{ ATARI_7800, "27" },
	{ ATARI_JAGUAR, "28" },
	{ ATARI_JAGUAR_CD, "29" },
	{ ATARI_LYNX, "4924" },
	{ ATARI_ST, "4937" },
	{ ATARI_XE, "30" },
	{ COLECOVISION, "31" },
	{ COMMODORE_64, "40" },
	{ INTELLIVISION, "32" },
	{ MAC_OS, "37" },
	{ XBOX, "14" },
	{ XBOX_360, "15" },
	{ MSX, "4929" },
	{ NEOGEO, "24" },
	{ NEOGEO_POCKET, "4922" },
	{ NEOGEO_POCKET_COLOR, "4923" },
	{ NINTENDO_3DS, "4912" },
	{ NINTENDO_64, "3" },
	{ NINTENDO_DS, "8" },
	{ FAMICOM_DISK_SYSTEM, "4936" },
	{ NINTENDO_ENTERTAINMENT_SYSTEM, "7" },
	{ GAME_BOY, "4" },
	{ GAME_BOY_ADVANCE, "5" },
	{ GAME_BOY_COLOR, "41" },
	{ NINTENDO_GAMECUBE, "2" },
	{ NINTENDO_WII, "9" },
	{ NINTENDO_WII_U, "38" },
	{ NINTENDO_VIRTUAL_BOY, "4918" },
	{ NINTENDO_GAME_AND_WATCH, "4950" },
	{ SEGA_32X, "33" },
	{ SEGA_CD, "21" },
	{ SEGA_DREAMCAST, "16" },
	{ SEGA_GAME_GEAR, "20" },
	{ SEGA_GENESIS, "18" },
	{ SEGA_MASTER_SYSTEM, "35" },
	{ SEGA_MEGA_DRIVE, "36" },
	{ SEGA_SATURN, "17" },
	{ SEGA_SG1000, "4949" },
	{ PLAYSTATION, "10" },
	{ PLAYSTATION_2, "11" },
	{ PLAYSTATION_3, "12" },
	{ PLAYSTATION_4, "4919" },
	{ PLAYSTATION_VITA, "39" },
	{ PLAYSTATION_PORTABLE, "13" },
	{ SUPER_NINTENDO, "6" },
	{ TURBOGRAFX_16, "34" },   // HuCards only
	{ TURBOGRAFX_CD, "4955" }, // CD-ROMs only
	{ WONDERSWAN, "4925" },
	{ WONDERSWAN_COLOR, "4926" },
	{ ZX_SPECTRUM, "4913" },
	{ VIDEOPAC_ODYSSEY2, "4927" },
	{ VECTREX, "4939" },
	{ TRS80_COLOR_COMPUTER, "4941" },
	{ TANDY, "4941" },
	{ SUPERGRAFX, "34" }, // The code is TurboGrafx 16, but they manage SUPERGRAFX into this one....
	{ AMIGACD32, "4947" },
	{ NEOGEO_CD, "4956" },
	{ PCFX, "4930" },
	{ POKEMINI, "4957" },
	{ SATELLAVIEW, "6" },
	{ SUFAMITURBO, "6" },
	{ PC_88, "4933" },
	{ PC_98, "4934" },
	{ SHARP_X1, "4977" },
	{ SHARP_X6800, "4931" },
	{ NINTENDO_SWITCH, "4971" },
	{ TI99, "4953" },
	{ COMMODORE_VIC20, "4945" },
	{ CHANNELF, "4928" },
	{ SAMCOUPE, "4979" },
	{ SUPER_CASSETTE_VISION , "4966" },
	{ ARCHIMEDES, "4944" },
	{ ACORN_ELECTRON , "4954" },
	{ ASTROCADE , "4968" },
	{ FMTOWNS, "4932" },
	{ PHILIPS_CDI, "4917" },
	{ WATARA_SUPERVISION, "4959" },
	{ SONIC, "36,21" }, // Sega Mega Drive & Sega CD

	// 1 = PC
	{ PC, "1" },
	{ MOONLIGHT, "1" },
	{ PRBOOM, "1" },
	{ QUAKE, "1" },

	// 23 = Arcade
	{ ARCADE, "23" },
	{ NAOMI, "23" },
	{ ATOMISWAVE, "23" },
	{ DAPHNE, "23" },

	{ SUPER_NINTENDO_MSU1, "6" },
	{ LCD_GAMES, "4951" },

	{ FUJITSU_FM7, "4978" },
	{ CASIO_PV1000, "4964" },
	{ TIGER_GAMECOM, "4940" },
	{ ENTEX_ADVENTURE_VISION, "4974" },
	{ EMERSON_ARCADIA_2001, "4963" },
	{ VTECH_CREATIVISION, "5005" },
	{ VTECH_VSMILE, "4988" }

	/* Non existing systems
	{ AMIGACDTV, "129" },
	{ CAVESTORY, "135" },
	{ ATOMISWAVE, "53" },
	{ LUTRO, "206" },
	{ GX4000, "87" },
	{ ZX81, "77" },
	{ VISUALPINBALL, "198" },
	{ FUTUREPINBALL, "199" },
	{ ORICATMOS, "131" },
	{ SOLARUS, "223" },
	{ THOMSON_TO_MO, "141" },
	{ OPENBOR, "214" },
	{ UZEBOX, "216" },
	{ APPLE2GS, "217" },
	{ SPECTRAVIDEO, "218" },
	{ PALMOS, "219" },
	*/
};

void thegamesdb_generate_json_scraper_requests(const ScraperSearchParams& params,
	std::queue<std::unique_ptr<ScraperRequest>>& requests, std::vector<ScraperSearchResult>& results)
{
	resources.prepare();
	std::string path = "https://api.thegamesdb.net/v1";
	bool usingGameID = false;
	const std::string apiKey = std::string("apikey=") + resources.getApiKey();
	std::string cleanName = params.nameOverride;
	if (!cleanName.empty() && cleanName.substr(0, 3) == "id:")
	{
		std::string gameID = cleanName.substr(3);
		path += "/Games/ByGameID?" + apiKey +
				"&fields=players,publishers,genres,overview,last_updated,rating,"
				"platform,coop,youtube,os,processor,ram,hdd,video,sound,alternates&"
				"include=boxart&id=" +
				HttpReq::urlEncode(gameID);
		usingGameID = true;
	} else
	{
		if (cleanName.empty())
			cleanName = params.game->getCleanName();
		path += "/Games/ByGameName?" + apiKey +
				"&fields=players,publishers,genres,overview,last_updated,rating,"
				"platform,coop,youtube,os,processor,ram,hdd,video,sound,alternates&"
				"include=boxart&name=" +
				HttpReq::urlEncode(cleanName);
	}

	if (usingGameID)
	{
		// if we have the ID already, we don't need the GetGameList request
		requests.push(std::unique_ptr<ScraperRequest>(new TheGamesDBJSONRequest(results, path)));
	} else
	{
		std::string platformQueryParam;
		auto& platforms = params.system->getPlatformIds();
		if (!platforms.empty())
		{
			bool first = true;
			platformQueryParam += "&filter%5Bplatform%5D=";
			for (auto platformIt = platforms.cbegin(); platformIt != platforms.cend(); platformIt++)
			{
				auto mapIt = gamesdb_new_platformid_map.find(*platformIt);
				if (mapIt != gamesdb_new_platformid_map.cend())
				{
					if (!first)
					{
						platformQueryParam += ",";
					}
					platformQueryParam += HttpReq::urlEncode(mapIt->second);
					first = false;
				} else
				{
					LOG(LogWarning) << "GamesDBJSONScraper::thegamesdb_generate_json_scraper_requests() - TheGamesDB scraper warning - no support for platform "
									<< getPlatformName(*platformIt);
				}
			}
			path += platformQueryParam;
		}

		requests.push(std::unique_ptr<ScraperRequest>(new TheGamesDBJSONRequest(requests, results, path)));
	}
}

namespace
{

std::string getStringOrThrow(const Value& v, const std::string& key)
{
	if (!v.HasMember(key.c_str()) || !v[key.c_str()].IsString())
	{
		throw std::runtime_error("rapidjson internal assertion failure: missing or non string key:" + key);
	}
	return v[key.c_str()].GetString();
}

int getIntOrThrow(const Value& v, const std::string& key)
{
	if (!v.HasMember(key.c_str()) || !v[key.c_str()].IsInt())
	{
		throw std::runtime_error("rapidjson internal assertion failure: missing or non int key:" + key);
	}
	return v[key.c_str()].GetInt();
}

int getIntOrThrow(const Value& v)
{
	if (!v.IsInt())
	{
		throw std::runtime_error("rapidjson internal assertion failure: not an int");
	}
	return v.GetInt();
}

std::string getBoxartImage(const Value& v)
{
	if (!v.IsArray() || v.Size() == 0)
	{
		return "";
	}
	for (int i = 0; i < (int)v.Size(); ++i)
	{
		auto& im = v[i];
		std::string type = getStringOrThrow(im, "type");
		std::string side = getStringOrThrow(im, "side");
		if (type == "boxart" && side == "front")
		{
			return getStringOrThrow(im, "filename");
		}
	}
	return getStringOrThrow(v[0], "filename");
}

std::string getDeveloperString(const Value& v)
{
	if (!v.IsArray())
	{
		return "";
	}
	std::string out = "";
	bool first = true;
	for (int i = 0; i < (int)v.Size(); ++i)
	{
		auto mapIt = resources.gamesdb_new_developers_map.find(getIntOrThrow(v[i]));
		if (mapIt == resources.gamesdb_new_developers_map.cend())
		{
			continue;
		}
		if (!first)
		{
			out += ", ";
		}
		out += mapIt->second;
		first = false;
	}
	return out;
}

std::string getPublisherString(const Value& v)
{
	if (!v.IsArray())
	{
		return "";
	}
	std::string out = "";
	bool first = true;
	for (int i = 0; i < (int)v.Size(); ++i)
	{
		auto mapIt = resources.gamesdb_new_publishers_map.find(getIntOrThrow(v[i]));
		if (mapIt == resources.gamesdb_new_publishers_map.cend())
		{
			continue;
		}
		if (!first)
		{
			out += ", ";
		}
		out += mapIt->second;
		first = false;
	}
	return out;
}

std::string getGenreString(const Value& v)
{
	if (!v.IsArray())
	{
		return "";
	}
	std::string out = "";
	bool first = true;
	for (int i = 0; i < (int)v.Size(); ++i)
	{
		auto mapIt = resources.gamesdb_new_genres_map.find(getIntOrThrow(v[i]));
		if (mapIt == resources.gamesdb_new_genres_map.cend())
		{
			continue;
		}
		if (!first)
		{
			out += ", ";
		}
		out += mapIt->second;
		first = false;
	}
	return out;
}

void processGame(const Value& game, const Value& boxart, std::vector<ScraperSearchResult>& results)
{
	std::string baseImageUrlThumb = getStringOrThrow(boxart["base_url"], "thumb");
	std::string baseImageUrlLarge = getStringOrThrow(boxart["base_url"], "large");

	ScraperSearchResult result;

	result.mdl.set(MetaDataId::Name, getStringOrThrow(game, "game_title"));

	if (game.HasMember("overview") && game["overview"].IsString())
	{
		result.mdl.set(MetaDataId::Desc, game["overview"].GetString());
	}
	if (game.HasMember("release_date") && game["release_date"].IsString())
	{
		result.mdl.set(MetaDataId::ReleaseDate, Utils::Time::DateTime(Utils::Time::stringToTime(game["release_date"].GetString(), "%Y-%m-%d")));
	}
	if (game.HasMember("developers") && game["developers"].IsArray())
	{
		result.mdl.set(MetaDataId::Developer, getDeveloperString(game["developers"]));
	}
	if (game.HasMember("publishers") && game["publishers"].IsArray())
	{
		result.mdl.set(MetaDataId::Publisher, getPublisherString(game["publishers"]));
	}
	if (game.HasMember("genres") && game["genres"].IsArray())
	{
		result.mdl.set(MetaDataId::Genre, getGenreString(game["genres"]));
	}
	if (game.HasMember("players") && game["players"].IsInt())
	{
		result.mdl.set(MetaDataId::Players, std::to_string(game["players"].GetInt()));
	}


	if (boxart.HasMember("data") && boxart["data"].IsObject())
	{
		std::string id = std::to_string(getIntOrThrow(game, "id"));
		if (boxart["data"].HasMember(id.c_str()))
		{
		    std::string image = getBoxartImage(boxart["data"][id.c_str()]);
		    result.thumbnailUrl = baseImageUrlThumb + "/" + image;
		    result.imageUrl = baseImageUrlLarge + "/" + image;
		}
	}

	results.push_back(result);
}
} // namespace

  // Process should return false only when we reached a maximum scrap by minute, to retry
bool TheGamesDBJSONRequest::process(HttpReq* request, std::vector<ScraperSearchResult>& results)
{
	assert(request->status() == HttpReq::REQ_SUCCESS);

	Document doc;
	doc.Parse(request->getContent().c_str());

	if (doc.HasParseError())
	{
		std::string err =
			std::string("TheGamesDBJSONRequest::process() - Error parsing JSON. \n\t") + GetParseError_En(doc.GetParseError());
		setError(err);
		LOG(LogError) << err;
		return true;
	}

	if (!doc.HasMember("data") || !doc["data"].HasMember("games") || !doc["data"]["games"].IsArray())
	{
		std::string warn = "TheGamesDBJSONRequest::process() - Response had no game data.\n";
		LOG(LogWarning) << warn;
		return true;
	}
	const Value& games = doc["data"]["games"];

	if (!doc.HasMember("include") || !doc["include"].HasMember("boxart"))
	{
		std::string warn = "TheGamesDBJSONRequest::process() - Response had no include boxart data.\n";
		LOG(LogWarning) << warn;
		return true;
	}

	const Value& boxart = doc["include"]["boxart"];

	if (!boxart.HasMember("base_url") || !boxart.HasMember("data") || !boxart.IsObject())
	{
		std::string warn = "TheGamesDBJSONRequest::process() - Response include had no usable boxart data.\n";
		LOG(LogWarning) << warn;
		return true;
	}

	resources.ensureResources();
	
	for (int i = 0; i < (int)games.Size(); ++i)
	{
		auto& v = games[i];
		try
		{
			processGame(v, boxart, results);
		}
		catch (std::runtime_error& e)
		{
			LOG(LogError) << "TheGamesDBJSONRequest::process() - ERROR: Error while processing game: " << e.what();
		}
	}

	return true;
}
#endif