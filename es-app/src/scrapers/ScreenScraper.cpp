#include "scrapers/ScreenScraper.h"

#include "utils/TimeUtil.h"
#include "utils/StringUtil.h"
#include "FileData.h"
#include "Log.h"
#include "PlatformId.h"
#include "Settings.h"
#include "SystemData.h"
#include <pugixml/src/pugixml.hpp>
#include <cstring>
#include "EsLocale.h"
#include <thread>
#include "ApiSystem.h"

using namespace PlatformIds;

#if defined(SCREENSCRAPER_DEV_LOGIN)

std::string ScreenScraperRequest::ensureUrl(const std::string url)
{
	return Utils::String::replace(
		Utils::String::replace(url, " ", "%20") ,
		"#screenscraperserveur#", "https://www.screenscraper.fr/");
}


/**
	List of systems and their IDs from
	https://www.screenscraper.fr/api/systemesListe.php?devid=xxx&devpassword=yyy&softname=zzz&output=XML
**/
const std::map<PlatformId, unsigned short> screenscraper_platformid_map{
	{ THREEDO, 29 },
	{ AMIGA, 64 },
	{ AMSTRAD_CPC, 65 },
	{ APPLE_II, 86 },
	{ ARCADE, 75 },
	{ ATARI_800, 43 },
	{ ATARI_2600, 26 },
	{ ATARI_5200, 40 },
	{ ATARI_7800, 41 },
	{ ATARI_JAGUAR, 27 },
	{ ATARI_JAGUAR_CD, 171 },
	{ ATARI_LYNX, 28 },
	{ ATARI_ST, 42},
	// missing Atari XE ?
	{ COLECOVISION, 48 },
	{ COMMODORE_64, 66 },
	{ INTELLIVISION, 115 },
	{ MAC_OS, 146 },
	{ XBOX, 32 },
	{ XBOX_360, 33 },
	{ MSX, 113 },
	{ NEOGEO, 142 },
	{ NEOGEO_POCKET, 25},
	{ NEOGEO_POCKET_COLOR, 82 },
	{ NINTENDO_3DS, 17 },
	{ NINTENDO_64, 14 },
	{ NINTENDO_64_DISK_DRIVE, 122 },
	{ NINTENDO_DS, 15 },
	{ FAMICOM_DISK_SYSTEM, 106 },
	{ NINTENDO_ENTERTAINMENT_SYSTEM, 3 },
	{ GAME_BOY, 9 },
	{ GAME_BOY_ADVANCE, 12 },
	{ GAME_BOY_COLOR, 10 },
	{ NINTENDO_GAMECUBE, 13 },
	{ NINTENDO_WII, 16 },
	{ NINTENDO_WII_U, 18 },
	{ NINTENDO_SWITCH, 225 },
	{ NINTENDO_VIRTUAL_BOY, 11 },
	{ NINTENDO_GAME_AND_WATCH, 52 },
	{ PC, 135 },
	{ PC_88, 221},
	{ PC_98, 208},
	{ SCUMMVM, 123},
	{ SEGA_32X, 19 },
	{ SEGA_CD, 20 },
	{ SEGA_DREAMCAST, 23 },
	{ SEGA_GAME_GEAR, 21 },
	{ SEGA_GENESIS, 1 },
	{ SEGA_MASTER_SYSTEM, 2 },
	{ SEGA_MEGA_DRIVE, 1 },
	{ SEGA_SATURN, 22 },
	{ SEGA_SG1000, 109 },
	{ SHARP_X1, 220 },
	{ SHARP_X6800, 79},
	{ PLAYSTATION, 57 },
	{ PLAYSTATION_2, 58 },
	{ PLAYSTATION_3, 59 },
	// missing Sony Playstation 4 ?
	{ PLAYSTATION_VITA, 62 },
	{ PLAYSTATION_PORTABLE, 61 },
	{ SUPER_NINTENDO, 4 },
	{ TURBOGRAFX_16, 31 },
	{ TURBOGRAFX_CD, 114 },
	{ WONDERSWAN, 45 },
	{ WONDERSWAN_COLOR, 46 },
	{ ZX_SPECTRUM, 76 },
	{ VIDEOPAC_ODYSSEY2, 104 },
	{ VECTREX, 102 },
	{ TRS80_COLOR_COMPUTER, 144 },
	{ TANDY, 144 },
	{ SUPERGRAFX, 105 },

	{ AMIGACD32, 130 },
	{ AMIGACDTV, 129 },
	{ ATOMISWAVE, 53 },
	{ CAVESTORY, 135 },
	{ GX4000, 87 },
	{ LUTRO, 206 },
	{ NAOMI, 56 },
	{ NEOGEO_CD, 70 },
	{ PCFX, 72 },
	{ POKEMINI, 211 },
	{ PRBOOM, 135 },
	{ SATELLAVIEW, 107 },
	{ SUFAMITURBO, 108 },
	{ ZX81, 77 },
	{ TIC80, 222 },
	{ MOONLIGHT, 138 }, // "PC Windows"
	{ MODEL3, 55 },
	{ TI99, 205 },
	{ WATARA_SUPERVISION, 207 },

	// Windows
	{ VISUALPINBALL, 198 },
	{ FUTUREPINBALL, 199 },

	// Misc
	{ COMMODORE_VIC20, 73 },
	{ ORICATMOS, 131 },
	{ CHANNELF, 80 },
	{ THOMSON_TO_MO, 141 },
	{ SAMCOUPE, 213 },
	{ OPENBOR, 214 },
	{ UZEBOX, 216 },
	{ APPLE2GS, 217 },
	{ SPECTRAVIDEO, 218 },
	{ PALMOS, 219 },
	{ DAPHNE, 49 },
	{ SOLARUS, 223 },
	{ PICO8, 234 },
	{ SUPER_CASSETTE_VISION, 67 },
	{ EASYRPG, 231 },
	{ SONIC, 0 }, // All systems
	{ QUAKE, 135 }, // DOS PC
	{ SUPER_GAME_BOY, 127 },
	{ COMMODORE_PET, 240 },
	{ ACORN_ATOM, 36 },
	{ NOKIA_NGAGE, 30 },
	{ ACORN_BBC_MICRO, 37 },
	{ ASTROCADE, 44 },
	{ ARCHIMEDES, 84 },
	{ ACORN_ELECTRON, 85 },
	{ ADAM, 89 },
	{ PHILIPS_CDI, 133 },
	{ SUPER_NINTENDO_MSU1, 210 },
	{ FUJITSU_FM7, 97 },
	{ CASIO_PV1000, 74 },
	{ TIGER_GAMECOM, 121 },
	{ ENTEX_ADVENTURE_VISION, 78 },
	{ EMERSON_ARCADIA_2001, 94 },
	{ VTECH_CREATIVISION, 241 },
	{ VTECH_VSMILE, 120 }
};

const std::map<unsigned short, std::string> screenscraper_arcadesystemid_map{
		{ 6, "cps1" },
		{ 7, "cps2" },
		{ 8, "cps3" },
		{ 47, "cave" },
		{ 68, "neogeo" },
		{ 142, "neogeo" },
		{ 147, "sega" },
		{ 148, "irem" },
		{ 150, "midway" },
		{ 151, "capcom" },
		{ 153, "tecmo" },
		{ 154, "snk" },
		{ 155, "namco" },
		{ 156, "namco" },
		{ 157, "taito" },
		{ 158, "konami" },
		{ 159, "jaleco" },
		{ 160, "atari" },
		{ 161, "nintendo" },
		{ 162, "dataeast" },
		{ 164, "sammy" },
		{ 166, "acclaim" },
		{ 167, "psikyo" },
		{ 174, "kaneko" },
		{ 183, "coleco" },
		{ 185, "atlus" },
		{ 186, "banpresto" }
};

// Helper XML parsing method, finding a node-by-name recursively.
pugi::xml_node find_node_by_name_re(const pugi::xml_node& node, const std::vector<std::string> node_names) {

	for (const std::string& _val : node_names)
	{
		pugi::xpath_query query_node_name((static_cast<std::string>("//") + _val).c_str());
		pugi::xpath_node_set results = node.select_nodes(query_node_name);

		if (results.size() > 0)
			return results.first().node();
	}

	return pugi::xml_node();
}

// Help XML parsing method, finding an direct child XML node starting from the parent and filtering by an attribute value list.
pugi::xml_node find_child_by_attribute_list(const pugi::xml_node& node_parent, const std::string& node_name, const std::string& attribute_name, const std::vector<std::string> attribute_values)
{
	for (auto _val : attribute_values)
	{
		for (pugi::xml_node node : node_parent.children(node_name.c_str()))
		{

			if (strcmp(node.attribute(attribute_name.c_str()).value(), _val.c_str()) == 0)
				return node;
		}
	}

	return pugi::xml_node(NULL);

}

void screenscraper_generate_scraper_requests(const ScraperSearchParams& params,
	std::queue< std::unique_ptr<ScraperRequest> >& requests,
	std::vector<ScraperSearchResult>& results)
{
	std::string path;

	ScreenScraperRequest::ScreenScraperConfig ssConfig;

	// FCA Fix for names override not working on Retropie
	if (params.nameOverride.length() == 0)
	{
		if (Utils::FileSystem::isDirectory(params.game->getPath()))
			path = ssConfig.getGameSearchUrl(params.game->getDisplayName());
		else
			path = ssConfig.getGameSearchUrl(params.game->getFileName());

		path += "&romtype=rom";

		// Use md5 to search scrapped game
		std::string fileNameToHash = params.game->getFullPath();
		auto length = Utils::FileSystem::getFileSize(fileNameToHash);

		if (length > 1024 * 1024 && !params.game->getMetadata(MetaDataId::Md5).empty()) // 1Mb
			path += "&md5=" + params.game->getMetadata(MetaDataId::Md5);
		else
		{

			if (params.game->hasContentFiles() && Utils::String::toLower(Utils::FileSystem::getExtension(fileNameToHash)) == ".m3u")
			{
				auto content = params.game->getContentFiles();
				if (content.size())
				{
					fileNameToHash = (*content.begin());
					length = Utils::FileSystem::getFileSize(fileNameToHash);
				}
			}

			// Use md5 to search scrapped game
			if (length > 0 && length <= 131072 * 1024) // 128 Mb max
			{
				std::string val = ApiSystem::getInstance()->getMD5(fileNameToHash, params.system->shouldExtractHashesFromArchives());
				if (!val.empty())
				{
					params.game->setMetadata(MetaDataId::Md5, val);
					path += "&md5=" + val;
				}
				else
					path += "&romtaille=" + std::to_string(length);
			}
			else
				path += "&romtaille=" + std::to_string(length);
		}
	}
	else
	{
		std::string name = Utils::String::replace(params.nameOverride, "_", " ");
		name = Utils::String::replace(name, "-", " ");

		path = ssConfig.getGameSearchUrl(name, true);
	}

	auto& platforms = params.system->getPlatformIds();
	std::vector<unsigned short> p_ids;

	// Get the IDs of each platform from the ScreenScraper list
	for (auto platformIt = platforms.cbegin(); platformIt != platforms.cend(); platformIt++)
	{
		auto mapIt = screenscraper_platformid_map.find(*platformIt);

		if (mapIt != screenscraper_platformid_map.cend())
		{
			p_ids.push_back(mapIt->second);
		}else{
			LOG(LogWarning) << "ScreenScraper: no support for platform " << getPlatformName(*platformIt);
			// Add the scrape request without a platform/system ID
			requests.push(std::unique_ptr<ScraperRequest>(new ScreenScraperRequest(requests, results, path)));
		}
	}

	// Sort the platform IDs and remove duplicates
	std::sort(p_ids.begin(), p_ids.end());
	auto last = std::unique(p_ids.begin(), p_ids.end());
	p_ids.erase(last, p_ids.end());

	for (auto platform = p_ids.cbegin(); platform != p_ids.cend(); platform++)
	{
		path += "&systemeid=";
		path += HttpReq::urlEncode(std::to_string(*platform));
		requests.push(std::unique_ptr<ScraperRequest>(new ScreenScraperRequest(requests, results, path)));
	}
}

// Process should return false only when we reached a maximum scrap by minute, to retry
bool ScreenScraperRequest::process(HttpReq* request, std::vector<ScraperSearchResult>& results)
{
	assert(request->status() == HttpReq::REQ_SUCCESS);

	auto content = request->getContent();

	pugi::xml_document doc;
	pugi::xml_parse_result parseResult = doc.load(content.c_str());

	if (!parseResult)
	{
		std::stringstream ss;
		ss << "ScreenScraperRequest - Error parsing XML." << std::endl << parseResult.description() << "";
		std::string err = ss.str();
		//setError(err); Don't consider it an error -> Request is a success. Simply : Game is not found		
		LOG(LogWarning) << err;
				
		if (Utils::String::toLower(content).find("maximum threads per minute reached") != std::string::npos)
			return false;
		
		return true;
	}

	processGame(doc, results);
	return true;
}

pugi::xml_node ScreenScraperRequest::findMedia(pugi::xml_node media_list, std::vector<std::string> mediaNames, std::string region)
{
	for (std::string media : mediaNames)
	{
		pugi::xml_node art = findMedia(media_list, media, region);
		if (art)
			return art;
	}

	return pugi::xml_node(NULL);
}

pugi::xml_node ScreenScraperRequest::findMedia(pugi::xml_node media_list, std::string mediaName, std::string region)
{
	pugi::xml_node art = pugi::xml_node(NULL);

	// Do an XPath query for media[type='$media_type'], then filter by region
	// We need to do this because any child of 'medias' has the form
	// <media type="..." region="..." format="..."> 
	// and we need to find the right media for the region.

	pugi::xpath_node_set results = media_list.select_nodes((static_cast<std::string>("media[@type='") + mediaName + "']").c_str());

	if (!results.size())
		return art;

	// Region fallback: WOR(LD), US, CUS(TOM?), JP, EU
	for (auto _region : std::vector<std::string>{ region, "wor", "us", "eu", "jp", "ss", "cus", "" })
	{
		if (art)
			break;

		for (auto node : results)
		{
			if (node.node().attribute("region").value() == _region)
			{
				art = node.node();
				break;
			}
		}
	}

	return art;
}

std::vector<std::string> ScreenScraperRequest::getRipList(std::string imageSource)
{
	if (imageSource == "ss")
		return { "ss", "sstitle" };
	if (imageSource == "sstitle")
		return { "sstitle", "ss" };
	if (imageSource == "mixrbv1" || imageSource == "mixrbv")
		return { "mixrbv1", "mixrbv2" };
	if (imageSource == "mixrbv2")
		return { "mixrbv2", "mixrbv1" };
	if (imageSource == "box-2D")
		return { "box-2D", "box-3D" };
	if (imageSource == "box-3D")
		return { "box-3D", "box-2D" };
	if (imageSource == "wheel")
		return { "wheel", "wheel-hd", "wheel-steel", "wheel-carbon", "screenmarqueesmall", "screenmarquee" };
	if (imageSource == "marquee")
		return { "screenmarqueesmall", "screenmarquee", "wheel", "wheel-hd", "wheel-steel", "wheel-carbon" };
	if (imageSource == "video")
		return { "video-normalized", "video" };

	return { imageSource };
}

void ScreenScraperRequest::processGame(const pugi::xml_document& xmldoc, std::vector<ScraperSearchResult>& out_results)
{
	LOG(LogDebug) << "ScreenScraperRequest::processGame >>";

	pugi::xml_node data = xmldoc.child("Data");
	if (data.child("jeux"))
		data = data.child("jeux");

	for (pugi::xml_node game = data.child("jeu"); game; game = game.next_sibling("jeu"))
	{
		ScraperSearchResult result;
		ScreenScraperRequest::ScreenScraperConfig ssConfig;

		std::string region = Utils::String::toLower(ssConfig.region);

		std::string language = Utils::String::toLower(EsLocale::getLanguage());
		if (language.empty())
			language = "en";
		else
		{
			auto shortNameDivider = language.find("_");
			if (shortNameDivider != std::string::npos)
			{
				region = Utils::String::toLower(language.substr(shortNameDivider + 1));
				language = Utils::String::toLower(language.substr(0, shortNameDivider));
			}
		}

		if (game.attribute("id"))
			result.mdl.set(MetaDataId::ScraperId, game.attribute("id").value());
		else
			result.mdl.set(MetaDataId::ScraperId, "");

		// Name fallback: US, WOR(LD). ( Xpath: Data/jeu[0]/noms/nom[*] ).
		result.mdl.set(MetaDataId::Name, find_child_by_attribute_list(game.child("noms"), "nom", "region", { region, "wor", "us" , "ss", "eu", "jp" }).text().get());

		// Description fallback language: EN, WOR(LD)
		std::string description = find_child_by_attribute_list(game.child("synopsis"), "synopsis", "langue", { language, "en", "wor" }).text().get();

		if (!description.empty()) {
			result.mdl.set(MetaDataId::Desc, Utils::String::replace(description, "&nbsp;", " "));
		}

		// Genre fallback language: EN. ( Xpath: Data/jeu[0]/genres/genre[*] )
		result.mdl.set(MetaDataId::Genre, find_child_by_attribute_list(game.child("genres"), "genre", "langue", { language, "en" }).text().get());
		//LOG(LogDebug) << "Genre: " << result.mdl.get(MetaDataId::Genre);

		// Get the date proper. The API returns multiple 'date' children nodes to the 'dates' main child of 'jeu'.
		// Date fallback: WOR(LD), US, SS, JP, EU
		std::string _date = find_child_by_attribute_list(game.child("dates"), "date", "region", { region, "wor", "us", "ss", "jp", "eu" }).text().get();
		//LOG(LogDebug) << "Release Date (unparsed): " << _date;

		// Date can be YYYY-MM-DD or just YYYY.
		if (_date.length() > 4)
		{
			result.mdl.set(MetaDataId::ReleaseDate, Utils::Time::DateTime(Utils::Time::stringToTime(_date, "%Y-%m-%d")));
		} else if (_date.length() > 0)
		{
			result.mdl.set(MetaDataId::ReleaseDate, Utils::Time::DateTime(Utils::Time::stringToTime(_date, "%Y")));
		}

		//LOG(LogDebug) << "Release Date (parsed): " << result.mdl.get(MetaDataId::ReleaseDate);

		/// Developer for the game( Xpath: Data/jeu[0]/developpeur )
		std::string developer = game.child("developpeur").text().get();
		if (!developer.empty())
			result.mdl.set(MetaDataId::Developer, Utils::String::replace(developer, "&nbsp;", " "));

		// Publisher for the game ( Xpath: Data/jeu[0]/editeur )
		std::string publisher = game.child("editeur").text().get();
		if (!publisher.empty())
			result.mdl.set(MetaDataId::Publisher, Utils::String::replace(publisher, "&nbsp;", " "));

		// Players
		result.mdl.set(MetaDataId::Players, game.child("joueurs").text().get());

        if(game.child("systeme").attribute("id"))
        {
            int systemId = game.child("systeme").attribute("id").as_int();

            if(screenscraper_arcadesystemid_map.find(systemId) != screenscraper_arcadesystemid_map.cend())
            {
                std::string systemName = screenscraper_arcadesystemid_map.at(game.child("systeme").attribute("id").as_int(0));
                result.mdl.set(MetaDataId::ArcadeSystemName, systemName);
            }
            //else
            //    LOG(LogDebug) << "System " << systemId << " not found";
        }

        // TODO: Validate rating
		if (Settings::getInstance()->getBool("ScrapeRatings") && game.child("note"))
		{
			float ratingVal = (game.child("note").text().as_int() / 20.0f);
			std::stringstream ss;
			ss << ratingVal;
			result.mdl.set(MetaDataId::Rating, ss.str());
		}
		else
			result.mdl.set(MetaDataId::Rating, "-1");

		// Media super-node
		pugi::xml_node media_list = game.child("medias");

		if (media_list)
		{
			std::vector<std::string> ripList = getRipList(Settings::getInstance()->getString("ScrapperImageSrc"));
			if (!ripList.empty())
			{
				pugi::xml_node art = findMedia(media_list, ripList, region);
				if (art)
				{
					// Sending a 'softname' containing space will make the image URLs returned by the API also contain the space. 
					//  Escape any spaces in the URL here
					result.imageUrl = ensureUrl(art.text().get());

					// Get the media type returned by ScreenScraper
					std::string media_type = art.attribute("format").value();
					if (!media_type.empty())
						result.imageType = "." + media_type;

					// Ask for the same image, but with a smaller size, for the thumbnail displayed during scraping
					result.thumbnailUrl = result.imageUrl + "&maxheight=250";
				}
				else
					LOG(LogDebug) << "Failed to find media XML node for image";
			}

			if (!Settings::getInstance()->getString("ScrapperThumbSrc").empty() &&
				Settings::getInstance()->getString("ScrapperThumbSrc") != Settings::getInstance()->getString("ScrapperImageSrc"))
			{
				ripList = getRipList(Settings::getInstance()->getString("ScrapperThumbSrc"));
				if (!ripList.empty())
				{
					pugi::xml_node art = findMedia(media_list, ripList, region);
					if (art)
					{
						// Ask for the same image, but with a smaller size, for the thumbnail displayed during scraping
						result.thumbnailUrl = ensureUrl(art.text().get());
					}
					else
						LOG(LogDebug) << "Failed to find media XML node for thumbnail";
				}
			}

			if (!Settings::getInstance()->getString("ScrapperLogoSrc").empty())
			{
				ripList = getRipList(Settings::getInstance()->getString("ScrapperLogoSrc"));
				if (!ripList.empty())
				{
					pugi::xml_node art = findMedia(media_list, ripList, region);
					if (art)
						result.marqueeUrl = ensureUrl(art.text().get());
					else
						LOG(LogDebug) << "Failed to find media XML node for video";
				}
			}

			if (Settings::getInstance()->getBool("ScrapeVideos"))
			{
				ripList = getRipList("video");
				if (!ripList.empty())
				{
					pugi::xml_node art = findMedia(media_list, ripList, region);
					if (art)
						result.videoUrl = ensureUrl(art.text().get());
					else
						LOG(LogDebug) << "Failed to find media XML node for video";
				}
			}
		}

		out_results.push_back(result);
	} // game

	LOG(LogDebug) << "ScreenScraperRequest::processGame <<";
}

// Currently not used in this module
void ScreenScraperRequest::processList(const pugi::xml_document& xmldoc, std::vector<ScraperSearchResult>& results)
{
	assert(mRequestQueue != nullptr);

	LOG(LogDebug) << "Processing a list of results";

	pugi::xml_node data = xmldoc.child("Data");
	pugi::xml_node game = data.child("jeu");

	if (!game)
		LOG(LogDebug) << "Found nothing";

	ScreenScraperRequest::ScreenScraperConfig ssConfig;

	// limit the number of results per platform, not in total.
	// otherwise if the first platform returns >= 7 games
	// but the second platform contains the relevant game,
	// the relevant result would not be shown.
	for (int i = 0; game && i < MAX_SCRAPER_RESULTS; i++)
	{
		std::string id = game.child("id").text().get();
		std::string name = game.child("nom").text().get();
		std::string platformId = game.child("systemeid").text().get();
		std::string path = ssConfig.getGameSearchUrl(name) + "&systemeid=" + platformId + "&gameid=" + id;

		mRequestQueue->push(std::unique_ptr<ScraperRequest>(new ScreenScraperRequest(results, path)));

		game = game.next_sibling("jeu");
	}
}

std::string ScreenScraperRequest::ScreenScraperConfig::getGameSearchUrl(const std::string gameName, bool jeuRecherche) const
{
	

	std::string ret = API_URL_BASE
		+ "/jeuInfos.php?" + std::string(SCREENSCRAPER_DEV_LOGIN) +
		+ "&softname=" + HttpReq::urlEncode(VERSIONED_SOFT_NAME)
		+ "&output=xml"
		+ "&romnom=" + HttpReq::urlEncode(gameName);

	if (jeuRecherche)
	{
		ret = std::string(API_URL_BASE)
			+ "/jeuRecherche.php?" + std::string(SCREENSCRAPER_DEV_LOGIN) +
			+ "&softname=" + HttpReq::urlEncode(VERSIONED_SOFT_NAME)
			+ "&output=xml"
			+ "&recherche=" + HttpReq::urlEncode(gameName);
	}

	std::string user = Settings::getInstance()->getString("ScreenScraperUser");
	std::string pass = Settings::getInstance()->getString("ScreenScraperPass");

	if (!user.empty() && !pass.empty())
		ret = ret + "&ssid=" + HttpReq::urlEncode(user) + "&sspassword=" + HttpReq::urlEncode(pass);

	return ret;
}

#endif
