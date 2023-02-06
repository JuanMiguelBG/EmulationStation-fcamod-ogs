#include "guis/GuiMenu.h"
#include "components/OptionListComponent.h"
#include "components/FlagOptionListComponent.h"
#include "components/SliderComponent.h"
#include "components/SwitchComponent.h"
#include "components/UpdatableTextComponent.h"
#include "components/BrightnessInfoComponent.h"
#include "components/VolumeInfoComponent.h"
#include "components/HelpComponent.h"
#include "guis/GuiCollectionSystemsOptions.h"
#include "guis/GuiDetectDevice.h"
#include "guis/GuiGeneralScreensaverOptions.h"
#include "guis/GuiMsgBox.h"
#include "guis/GuiScraperStart.h"
#include "guis/GuiSettings.h"
#include "guis/UpdatableGuiSettings.h"
#include "guis/GuiSystemInformation.h"
#include "guis/GuiQuitOptions.h"
#include "guis/GuiSystemHotkeyEventsOptions.h"
#include "guis/GuiWifi.h"
#include "guis/GuiBluetoothScan.h"
#include "guis/GuiBluetoothPaired.h"
#include "guis/GuiBluetoothConnected.h"
#include "guis/GuiAutoSuspendOptions.h"
#include "guis/GuiRetroachievementsOptions.h"
#include "guis/GuiDisplayAutoDimOptions.h"
#include "guis/GuiRemoteServicesOptions.h"
#include "guis/GuiMenusOptions.h"
#include "views/UIModeController.h"
#include "views/ViewController.h"
#include "CollectionSystemManager.h"
#include "Scripting.h"
#include "SystemData.h"
#include "VolumeControl.h"
#include <SDL_events.h>
#include <algorithm>
#include "AudioManager.h"
#include "resources/TextureData.h"
#include "animations/LambdaAnimation.h"
#include "guis/GuiThemeInstall.h"
#include "GuiGamelistOptions.h" // grid sizes
#include "platform.h"
#include "renderers/Renderer.h" // setSwapInterval()
#include "guis/GuiTextEditPopup.h"
#include "guis/GuiTextEditPopupKeyboard.h"
#include "scrapers/ThreadedScraper.h"
#include "ApiSystem.h"
#include "views/gamelist/IGameListView.h"
#include "SystemConf.h"
#include "utils/NetworkUtil.h"
#include "utils/AsyncUtil.h"
#include "guis/GuiLoading.h"


GuiMenu::GuiMenu(Window* window, bool animate) : GuiComponent(window), mMenu(window, _("MAIN MENU"), false), mVersion(window)
{
	if (ApiSystem::getInstance()->isScriptingSupported(ApiSystem::ScriptId::KODI))
	{
		addEntry(_("KODI MEDIA CENTER").c_str(), false, [this]
			{
				Window *window = mWindow;
				delete this;
				if (!ApiSystem::getInstance()->launchKodi(window))
					LOG(LogWarning) << "GuiMenu::GuiMenu() - Shutdown Kodi terminated with non-zero result!";

			}, "iconKodi");
	}

	addEntry(_("DISPLAY SETTINGS"), true, [this] { openDisplaySettings(); }, "iconDisplay");

	auto theme = ThemeData::getMenuTheme();

	bool isFullUI = UIModeController::getInstance()->isUIModeFull();

	if (isFullUI)
	{
		addEntry(_("UI SETTINGS"), true, [this] { openUISettings(); }, "iconUI");
		//addEntry(_("CONFIGURE INPUT"), true, [this] { openConfigInput(); }, "iconControllers");
		addEntry(_("CONTROLLERS SETTINGS").c_str(), true, [this] { openControllersSettings(); }, "iconControllers");
	}

	addEntry(_("SOUND SETTINGS"), true, [this] { openSoundSettings(); }, "iconSound");

	if (isFullUI)
	{
		addEntry(_("GAME COLLECTION SETTINGS"), true, [this] { openCollectionSystemSettings(); }, "iconGames");

		// Emulator settings 
		for (auto system : SystemData::sSystemVector)
		{
			if (system->isCollection() || system->getSystemEnvData()->mEmulators.size() == 0 || (system->getSystemEnvData()->mEmulators.size() == 1 && system->getSystemEnvData()->mEmulators[0].mCores.size() <= 1))
				continue;

			addEntry(_("EMULATOR SETTINGS"), true, [this] { openEmulatorSettings(); }, "iconSystem");
			break;
		}

		if (ApiSystem::getInstance()->isScriptingSupported(ApiSystem::ScriptId::WIFI))
			addEntry(_("NETWORK SETTINGS").c_str(), true, [this]  { openNetworkSettings(); }, "iconNetwork");

		if (ApiSystem::getInstance()->isScriptingSupported(ApiSystem::ScriptId::BLUETOOTH))
			addEntry(_("BLUETOOTH SETTINGS").c_str(), true, [this] { openBluetoothSettings(); }, "iconBluetooth");

		addEntry(_("SCRAPER"), true, [this] { openScraperSettings(); }, "iconScraper");

		addEntry(_("ADVANCED SETTINGS"), true, [this] { openAdvancedSettings(); }, "iconAdvanced");

		addEntry(_("SYSTEM INFORMATION"), true, [this] { openSystemInformation(); }, "iconInformation");
	}

	std::string quit_menu_label = "QUIT";
	if ( Settings::getInstance()->getBool("ShowOnlyExit") && Settings::getInstance()->getBool("ShowOnlyExitActionAsMenu") )
	{
		quit_menu_label = Settings::getInstance()->getString("OnlyExitAction");
		if ( quit_menu_label == "exit_es" )
			quit_menu_label = "QUIT EMULATIONSTATION";
	}

	addEntry(Utils::String::toUpper(_(quit_menu_label)), !Settings::getInstance()->getBool("ShowOnlyExit"), [this] { openQuitMenu(); }, "iconQuit");

	addStatusBarInfo(window);

	addChild(&mMenu);
	addVersionInfo();

	// resize & position
	float width = mSize.x(),
		  height = mSize.y();

//	setSize(mMenu.getSize());
	if (Renderer::isSmallScreen() || !Settings::getInstance()->getBool("CenterMenus"))
	{
		width = Renderer::getScreenWidth();
		height = Renderer::getScreenHeight();
	}
	setSize(width, height);

	float x_end = 0.f,
		  y_end = 0.f;

	if (!Renderer::isSmallScreen() && Settings::getInstance()->getBool("CenterMenus"))
	{
		x_end = (Renderer::getScreenWidth() - mSize.x()) / 2;  // center
		y_end = (Renderer::getScreenHeight() - mSize.y()) / 2; // center
	}

	if (animate)
	{
		float x_start = x_end,
			  y_start = Renderer::getScreenHeight() * 0.9f;
		
		animateTo(Vector2f(x_start, y_start), Vector2f(x_end, y_end));
	}
	else
		setPosition(x_end, y_end);
}

void GuiMenu::openDisplaySettings()
{
	auto pthis = this;
	Window* window = mWindow;

	auto s = new GuiSettings(window, _("DISPLAY SETTINGS"));

	// Brightness
	auto brightness = std::make_shared<SliderComponent>(window, 1.0f, 100.f, 1.0f, "%");
	brightness->setValue((float) ApiSystem::getInstance()->getBrightnessLevel());
	s->addWithLabel(_("BRIGHTNESS"), brightness);
	brightness->setOnValueChanged([window, s](const float &newVal)
		{
			int brightness_level = (int)Math::round( newVal );
			window->getBrightnessInfoComponent()->reset();
			ApiSystem::getInstance()->setBrightnessLevel(brightness_level);
			s->setVariable("reloadGuiMenu", true);
		});

	if (UIModeController::getInstance()->isUIModeFull())
	{
		// brightness overlay
		s->addSwitch(_("SHOW OVERLAY WHEN BRIGHTNESS CHANGES"), "BrightnessPopup", true);

		// restore brightness
		s->addSwitch(_("RESTORE BRIGHTNESS AFTER GAME"), "RestoreBrightnesAfterGame", true);

		if (ApiSystem::getInstance()->isScriptingSupported(ApiSystem::ScriptId::DISPLAY))
		{
			// blink with low battery
			bool blink_low_battery_value = Settings::getInstance()->getBool("DisplayBlinkLowBattery");
			auto blink_low_battery = std::make_shared<SwitchComponent>(window, blink_low_battery_value);
			s->addWithLabel(_("BLINK WITH LOW BATTERY"), blink_low_battery);
			s->addSaveFunc([blink_low_battery, blink_low_battery_value]
				{
					bool new_blink_low_battery_value = blink_low_battery->getState();
					if (blink_low_battery_value != new_blink_low_battery_value)
					{
						Settings::getInstance()->setBool("DisplayBlinkLowBattery", new_blink_low_battery_value);
						ApiSystem::getInstance()->setDisplayBlinkLowBattery(new_blink_low_battery_value);
					}
				});

			s->addEntry(_("AUTO DIM SETTINGS"), true, [this] { openDisplayAutoDimSettings(); });
		}
	}

	s->onFinalize([s, pthis, window]
	{
		if (s->getVariable("reloadGuiMenu"))
		{
			if (pthis)
				delete pthis;
			window->pushGui(new GuiMenu(window, false));
		}
	});

	window->pushGui(s);
}

void GuiMenu::openDisplayAutoDimSettings()
{
	Window *window = mWindow;
	window->pushGui(new GuiDisplayAutoDimOptions(window));
}

void GuiMenu::openControllersSettings()
{
	auto pthis = this;
	Window *window = mWindow;

	GuiSettings* s = new GuiSettings(window, _("CONTROLLERS SETTINGS"));

	auto invert_AB_buttons = std::make_shared<SwitchComponent>(window, Settings::getInstance()->getBool("InvertButtonsAB"));
	s->addWithLabel(_("SWITCH A/B BUTTONS IN EMULATIONSTATION"), invert_AB_buttons);
	s->addSaveFunc([this, s, invert_AB_buttons]
		{
			if (Settings::getInstance()->setBool("InvertButtonsAB", invert_AB_buttons->getState()))
			{
				InputConfig::AssignActionButtons();
				s->setVariable("reloadAll", true);
			}
		});

	auto invert_pu_buttons = std::make_shared<SwitchComponent>(window, Settings::getInstance()->getBool("InvertButtonsPU"));
	s->addWithLabel(_("SWITCH \"PAGE UP\" TO L1 IN EMULATIONSTATION"), invert_pu_buttons);
	s->addSaveFunc([this, s, invert_pu_buttons]
		{
			if (Settings::getInstance()->setBool("InvertButtonsPU", invert_pu_buttons->getState()))
			{
				InputConfig::AssignActionButtons();
				s->setVariable("reloadAll", true);
			}
		});

	auto invert_pd_buttons = std::make_shared<SwitchComponent>(window, Settings::getInstance()->getBool("InvertButtonsPD"));
	s->addWithLabel(_("SWITCH \"PAGE DOWN\" TO R1 IN EMULATIONSTATION"), invert_pd_buttons);
	s->addSaveFunc([this, s, invert_pd_buttons]
		{
			if (Settings::getInstance()->setBool("InvertButtonsPD", invert_pd_buttons->getState()))
			{
				InputConfig::AssignActionButtons();
				s->setVariable("reloadAll", true);
			}
		});

	s->addEntry(_("CONFIGURE INPUT"), true, [this] { openConfigInput(); } );

	s->onFinalize([s, pthis, window]
	{
		if (s->getVariable("reloadAll"))
		{
			ViewController::get()->reloadAll(window);
			window->endRenderLoadingScreen();
		}
	});

	window->pushGui(s);
}

void GuiMenu::openScraperSettings()
{
	Window *window = mWindow;

	auto s = new GuiSettings(window, _("SCRAPER"));

	std::string scraper = Settings::getInstance()->getString("Scraper");

	// scrape from
	auto scraper_list = std::make_shared< OptionListComponent< std::string > >(window, _("SCRAPE FROM"), false);
	std::vector<std::string> scrapers = getScraperList();

	// Select either the first entry of the one read from the settings, just in case the scraper from settings has vanished.
	for (auto it = scrapers.cbegin(); it != scrapers.cend(); it++)
		scraper_list->add(*it, *it, *it == scraper);

	s->addWithLabel(_("SCRAPE FROM"), scraper_list);
	s->addSaveFunc([scraper_list] { Settings::getInstance()->setString("Scraper", scraper_list->getSelected()); });

	if (!scraper_list->hasSelection())
	{
		scraper_list->selectFirstItem();
		scraper = scraper_list->getSelected();
	}

	if (scraper == "ScreenScraper")
	{
		// Image source : <image> tag
		std::string imageSourceName = Settings::getInstance()->getString("ScrapperImageSrc");
		auto imageSource = std::make_shared< OptionListComponent<std::string> >(window, _("IMAGE SOURCE"), false);
		//imageSource->add(_("NONE"), "", imageSourceName.empty());
		imageSource->add(_("SCREENSHOT"), "ss", imageSourceName == "ss");
		imageSource->add(_("TITLE SCREENSHOT"), "sstitle", imageSourceName == "sstitle");
		imageSource->add(_("MIX V1"), "mixrbv1", imageSourceName == "mixrbv1");
		imageSource->add(_("MIX V2"), "mixrbv2", imageSourceName == "mixrbv2");
		imageSource->add(_("BOX 2D"), "box-2D", imageSourceName == "box-2D");
		imageSource->add(_("BOX 3D"), "box-3D", imageSourceName == "box-3D");

		if (!imageSource->hasSelection())
			imageSource->selectFirstItem();

		s->addWithLabel(_("IMAGE SOURCE"), imageSource);
		s->addSaveFunc([imageSource] { Settings::getInstance()->setString("ScrapperImageSrc", imageSource->getSelected()); });

		// Box source : <thumbnail> tag
		std::string thumbSourceName = Settings::getInstance()->getString("ScrapperThumbSrc");
		auto thumbSource = std::make_shared< OptionListComponent<std::string> >(window, _("BOX SOURCE"), false);
		thumbSource->add(_("NONE"), "", thumbSourceName.empty());
		thumbSource->add(_("BOX 2D"), "box-2D", thumbSourceName == "box-2D");
		thumbSource->add(_("BOX 3D"), "box-3D", thumbSourceName == "box-3D");

		if (!thumbSource->hasSelection())
			thumbSource->selectFirstItem();

		s->addWithLabel(_("BOX SOURCE"), thumbSource);
		s->addSaveFunc([thumbSource] { Settings::getInstance()->setString("ScrapperThumbSrc", thumbSource->getSelected()); });

		imageSource->setSelectedChangedCallback([this, thumbSource](std::string value)
		{
			if (value == "box-2D")
				thumbSource->remove(_("BOX 2D"));
			else
				thumbSource->add(_("BOX 2D"), "box-2D", false);

			if (value == "box-3D")
				thumbSource->remove(_("BOX 3D"));
			else
				thumbSource->add(_("BOX 3D"), "box-3D", false);
		});

		// Logo source : <marquee> tag
		std::string logoSourceName = Settings::getInstance()->getString("ScrapperLogoSrc");
		auto logoSource = std::make_shared< OptionListComponent<std::string> >(window, _("LOGO SOURCE"), false);
		logoSource->add(_("NONE"), "", logoSourceName.empty());
		logoSource->add(_("WHEEL"), "wheel", logoSourceName == "wheel");
		logoSource->add(_("MARQUEE"), "marquee", logoSourceName == "marquee");

		if (!logoSource->hasSelection())
			logoSource->selectFirstItem();

		s->addWithLabel(_("LOGO SOURCE"), logoSource);
		s->addSaveFunc([logoSource] { Settings::getInstance()->setString("ScrapperLogoSrc", logoSource->getSelected()); });

		// scrape ratings
		auto scrape_ratings = std::make_shared<SwitchComponent>(window, Settings::getInstance()->getBool("ScrapeRatings"));
		s->addWithLabel(_("SCRAPE RATINGS"), scrape_ratings);
		s->addSaveFunc([scrape_ratings] { Settings::getInstance()->setBool("ScrapeRatings", scrape_ratings->getState()); });

		// scrape video
		auto scrape_video = std::make_shared<SwitchComponent>(window, Settings::getInstance()->getBool("ScrapeVideos"));
		s->addWithLabel(_("SCRAPE VIDEOS"), scrape_video);
		s->addSaveFunc([scrape_video] { Settings::getInstance()->setBool("ScrapeVideos", scrape_video->getState()); });

		// Account
		s->addInputTextRow(_("USERNAME"), "ScreenScraperUser", false);
		s->addInputTextRow(_("PASSWORD"), "ScreenScraperPass", true);
	}
	else
	{
		// scrape ratings
		auto scrape_ratings = std::make_shared<SwitchComponent>(window, Settings::getInstance()->getBool("ScrapeRatings"));
		s->addWithLabel(_("SCRAPE RATINGS"), scrape_ratings); // batocera
		s->addSaveFunc([scrape_ratings] { Settings::getInstance()->setBool("ScrapeRatings", scrape_ratings->getState()); });
	}

	// scrape now
	ComponentListRow row;
	auto openScrapeNow = [this, window] 
	{ 
		if (ThreadedScraper::isRunning())
		{
			window->pushGui(new GuiMsgBox(window, _("SCRAPING IS RUNNING. DO YOU WANT TO STOP IT ?"), _("YES"), [this, window]
			{
				ThreadedScraper::stop();
			}, _("NO"), nullptr));

			return;
		}

		window->pushGui(new GuiScraperStart(window));
	};
	std::function<void()> openAndSave = openScrapeNow;
	openAndSave = [s, openAndSave] { s->save(); openAndSave(); };
	s->addEntry(_("SCRAPE NOW"), true, openAndSave, "iconScraper");

	scraper_list->setSelectedChangedCallback([this, s, scraper, scraper_list](std::string value)
	{
		if (value != scraper && (scraper == "ScreenScraper" || value == "ScreenScraper"))
		{
			Settings::getInstance()->setString("Scraper", value);
			delete s;
			openScraperSettings();
		}
	});

	window->pushGui(s);
}

void GuiMenu::openSoundSettings()
{
	auto pthis = this;
	Window* window = mWindow;

	auto s = new GuiSettings(window, _("SOUND SETTINGS"));

	if (VolumeControl::getInstance()->isAvailable())
	{
		s->addGroup(_("VOLUME"));

		// volume
		float increment = 1.0f;
		if (SystemConf::getInstance()->getBool("bluetooth.audio.connected"))
			increment = 2.0f;

		auto volume = std::make_shared<SliderComponent>(window, 0.f, 100.f, increment, "%");
		volume->setValue( (float)ApiSystem::getInstance()->getVolume() );
		s->addWithLabel(_("SYSTEM VOLUME"), volume);
		volume->setOnValueChanged([window, s](const float &newVal)
			{
				int volume_level = (int)Math::round(newVal);
				window->getVolumeInfoComponent()->reset();
				ApiSystem::getInstance()->setVolume(volume_level);
				s->setVariable("reloadGuiMenu", true);
			});

		// Music Volume
		auto musicVolume = std::make_shared<SliderComponent>(window, 0.f, 100.f, 1.f, "%");
		musicVolume->setValue(Settings::getInstance()->getInt("MusicVolume"));
		musicVolume->setOnValueChanged([](const float &newVal) { Settings::getInstance()->setInt("MusicVolume", (int)round(newVal)); });
		s->addWithLabel(_("MUSIC VOLUME"), musicVolume);

		if (UIModeController::getInstance()->isUIModeFull())
		{
			// volume overlay
			s->addSwitch(_("SHOW OVERLAY WHEN VOLUME CHANGES"), "VolumePopup", true);

			// restore volume
			s->addSwitch(_("RESTORE VOLUME AFTER GAME"), "RestoreVolumeAfterGame", true);

			auto theme = ThemeData::getMenuTheme();
			std::shared_ptr<Font> font = theme->Text.font;
			unsigned int color = theme->Text.color;

			if (!SystemConf::getInstance()->getBool("bluetooth.audio.connected"))
			{
				// audio card
				s->addWithLabel(_("AUDIO CARD"), std::make_shared<TextComponent>(window, Utils::String::toUpper(_("default")), font, color));

				// volume control device
				s->addWithLabel(_("AUDIO DEVICE"), std::make_shared<TextComponent>(window, Utils::String::toUpper(_("Playback")), font, color));

				if (ApiSystem::getInstance()->isScriptingSupported(ApiSystem::ScriptId::SOUND))
				{
					// output device
					auto out_dev = std::make_shared< OptionListComponent<std::string> >(window, _("OUTPUT DEVICE"), false);
					std::vector<std::string> output_devices = ApiSystem::getInstance()->getOutputDevices();
					std::string out_dev_value = ApiSystem::getInstance()->getOutputDevice();
					//LOG(LogDebug) << "GuiMenu::openSoundSettings() - actual output device: " << out_dev_value;
					for(auto od = output_devices.cbegin(); od != output_devices.cend(); od++)
					{
						std::string out_dev_label;
						if (*od == "OFF")
							out_dev_label = "MUTE";
						else if (*od == "SPK")
							out_dev_label = "SPEAKER";
						else if (*od == "HP")
							out_dev_label = "HEADPHONES";
						else if (*od == "SPK_HP")
							out_dev_label = "SPEAKER AND HEADPHONES";
						else
							out_dev_label = *od;

						out_dev->add(_(out_dev_label), *od, out_dev_value == *od);
					}
					s->addWithLabel(_("OUTPUT DEVICE"), out_dev);
					out_dev->setSelectedChangedCallback([](const std::string &newVal)
						{
							ApiSystem::getInstance()->setOutputDevice(newVal);
						});
				}
			}
			else
			{
				// audio card
				s->addWithLabel(_("AUDIO CARD"), std::make_shared<TextComponent>(window, _("BLUETOOTH AUDIO"), font, color));

				// volume control device
				s->addWithLabel(_("AUDIO DEVICE"), std::make_shared<TextComponent>(window, SystemConf::getInstance()->get("bluetooth.audio.device"), font, color));
			}
		}
	}

	s->addGroup(_("MUSIC"));

	// disable sounds
	s->addSwitch(_("FRONTEND MUSIC"), "audio.bgmusic", true, []
		{
			if (Settings::getInstance()->getBool("audio.bgmusic"))
				AudioManager::getInstance()->playRandomMusic();
			else
				AudioManager::getInstance()->stopMusic();
		});

	//display music titles
	s->addSwitch(_("DISPLAY SONG TITLES"), "audio.display_titles", true);

	if (UIModeController::getInstance()->isUIModeFull())
	{
		// how long to display the song titles?
		auto titles_time = std::make_shared<SliderComponent>(window, 2.f, 120.f, 2.f, "s");
		titles_time->setValue(Settings::getInstance()->getInt("audio.display_titles_time"));
		s->addWithLabel(_("SONG TITLE DISPLAY DURATION"), titles_time);
		s->addSaveFunc([titles_time] {
				Settings::getInstance()->setInt("audio.display_titles_time", (int)Math::round(titles_time->getValue()));
			});

		// music per system
		s->addSwitch(_("ONLY PLAY SYSTEM-SPECIFIC MUSIC FOLDER"), "audio.persystem", true, [] { AudioManager::getInstance()->changePlaylist(ViewController::get()->getState().getSystem()->getTheme(), true); } );

		// batocera - music per system
		s->addSwitch(_("PLAY SYSTEM-SPECIFIC MUSIC"), "audio.thememusics", true, [] { AudioManager::getInstance()->changePlaylist(ViewController::get()->getState().getSystem()->getTheme(), true); });

		s->addSwitch(_("LOWER MUSIC WHEN PLAYING VIDEO"), "VideoLowersMusic", true);

		s->addGroup(_("SOUNDS"));

		// disable sounds
		s->addSwitch(_("ENABLE NAVIGATION SOUNDS"), "EnableSounds", true, []
			{
				if (Settings::getInstance()->getBool("EnableSounds") && PowerSaver::getMode() == PowerSaver::INSTANT)
				{
					Settings::getInstance()->setPowerSaverMode("default");
					PowerSaver::init();
				}
			});

		s->addSwitch(_("ENABLE VIDEO PREVIEW AUDIO"), "VideoAudio", true);
	}

	s->onFinalize([s, pthis, window]
	{
		if (s->getVariable("reloadGuiMenu"))
		{
			if (pthis)
				delete pthis;
			window->pushGui(new GuiMenu(window, false));
		}
	});

	window->pushGui(s);

}

struct ThemeConfigOption
{
	std::string defaultSettingName;
	std::string subset;
	std::shared_ptr<OptionListComponent<std::string>> component;
};

void GuiMenu::openThemeConfiguration(Window* mWindow, GuiComponent* s, std::shared_ptr<OptionListComponent<std::string>> theme_set,const std::string systemTheme)
{
	Window* window = mWindow;
	if (theme_set != nullptr && Settings::getInstance()->getString("ThemeSet") != theme_set->getSelected())
	{
		window->pushGui(new GuiMsgBox(window, _("YOU MUST APPLY THE THEME BEFORE EDIT CONFIGURATION"), _("OK")));
		return;
	}

	auto system = ViewController::get()->getState().getSystem();
	auto theme = system->getTheme();

	auto themeconfig = new GuiSettings(window, (systemTheme.empty() ? _("THEME CONFIGURATION") : _("VIEW CUSTOMISATION")).c_str());

	auto themeSubSets = theme->getSubSets();

	std::string viewName;
	bool showGridFeatures = true;
	if (!systemTheme.empty())
	{
		auto glv = ViewController::get()->getGameListView(system);
		viewName = glv->getName();
		std::string baseType = theme->getCustomViewBaseType(viewName);

		showGridFeatures = (viewName == "grid" || baseType == "grid");
	}

	// gamelist_style
	std::shared_ptr<OptionListComponent<std::string>> gamelist_style = nullptr;

	if (systemTheme.empty())
	{
		gamelist_style = std::make_shared< OptionListComponent<std::string> >(window, _("GAMELIST VIEW STYLE"), false);

		std::vector<std::pair<std::string, std::string>> styles;
		styles.push_back(std::pair<std::string, std::string>("automatic", _("automatic")));

		if (system != NULL)
		{
			auto mViews = theme->getViewsOfTheme();
			for (auto it = mViews.cbegin(); it != mViews.cend(); ++it)
			{
				if (it->first == "basic" || it->first == "detailed" || it->first == "grid")
					styles.push_back(std::pair<std::string, std::string>(it->first, _(it->first.c_str())));
				else
					styles.push_back(*it);
			}
		}
		else
		{
			styles.push_back(std::pair<std::string, std::string>("basic", _("basic")));
			styles.push_back(std::pair<std::string, std::string>("detailed", _("detailed")));
		}

		auto viewPreference = systemTheme.empty() ? Settings::getInstance()->getString("GamelistViewStyle") : system->getSystemViewMode();
		if (!theme->hasView(viewPreference))
			viewPreference = "automatic";

		for (auto it = styles.cbegin(); it != styles.cend(); it++)
			gamelist_style->add(_(it->second.c_str()), it->first, viewPreference == it->first);

		if (!gamelist_style->hasSelection())
			gamelist_style->selectFirstItem();

		themeconfig->addWithLabel(_("GAMELIST VIEW STYLE"), gamelist_style);
	}

	// Default grid size
	std::shared_ptr<OptionListComponent<std::string>> mGridSize = nullptr;
	if (showGridFeatures && system != NULL && theme->hasView("grid"))
	{
		Vector2f gridOverride =
			systemTheme.empty() ? Vector2f::parseString(Settings::getInstance()->getString("DefaultGridSize")) :
			system->getGridSizeOverride();

		auto ovv = std::to_string((int)gridOverride.x()) + "x" + std::to_string((int)gridOverride.y());

		mGridSize = std::make_shared<OptionListComponent<std::string>>(window, _("DEFAULT GRID SIZE"), false);

		bool found = false;
		for (auto it = GuiGamelistOptions::gridSizes.cbegin(); it != GuiGamelistOptions::gridSizes.cend(); it++)
		{
			bool sel = (gridOverride == Vector2f(0, 0) && *it == "automatic") || ovv == *it;
			if (sel)
				found = true;

			mGridSize->add(_(it->c_str()), *it, sel);
		}

		if (!found)
			mGridSize->selectFirstItem();

		themeconfig->addWithLabel(_("DEFAULT GRID SIZE"), mGridSize);
	}

	std::map<std::string, ThemeConfigOption> options;

	for (std::string subset : theme->getSubSetNames(viewName))
	{
		std::string settingName = "subset." + subset;
		std::string perSystemSettingName = systemTheme.empty() ? "" : "subset." + systemTheme + "." + subset;

		if (subset == "colorset") settingName = "ThemeColorSet";
		else if (subset == "iconset") settingName = "ThemeIconSet";
		else if (subset == "menu") settingName = "ThemeMenu";
		else if (subset == "systemview") settingName = "ThemeSystemView";
		else if (subset == "gamelistview") settingName = "ThemeGamelistView";
		else if (subset == "region") settingName = "ThemeRegionName";

		auto themeColorSets = ThemeData::getSubSet(themeSubSets, subset);
			
		if (themeColorSets.size() > 0)
		{
			auto selectedColorSet = themeColorSets.end();
			auto selectedName = !perSystemSettingName.empty() ? Settings::getInstance()->getString(perSystemSettingName) : Settings::getInstance()->getString(settingName);

			if (!perSystemSettingName.empty() && selectedName.empty())
				selectedName = Settings::getInstance()->getString(settingName);

			for (auto it = themeColorSets.begin(); it != themeColorSets.end() && selectedColorSet == themeColorSets.end(); it++)
				if (it->name == selectedName)
					selectedColorSet = it;

			std::shared_ptr<OptionListComponent<std::string>> item = std::make_shared<OptionListComponent<std::string> >(window, _("THEME") + " " + Utils::String::toUpper(subset).c_str(), false);
			item->setTag(!perSystemSettingName.empty()? perSystemSettingName : settingName);

			for (auto it = themeColorSets.begin(); it != themeColorSets.end(); it++)
			{
				std::string displayName = it->displayName;

				if (!systemTheme.empty())
				{
					std::string defaultValue = Settings::getInstance()->getString(settingName);
					if (defaultValue.empty())
						defaultValue = system->getTheme()->getDefaultSubSetValue(subset);

					if (it->name == defaultValue)
						displayName = displayName + " (" + _("DEFAULT") +")";
				}

				item->add(displayName, it->name, it == selectedColorSet);
			}

			if (selectedColorSet == themeColorSets.end())
				item->selectFirstItem();

			if (!themeColorSets.empty())
			{						
				std::string displayName = themeColorSets.cbegin()->subSetDisplayName;
				if (!displayName.empty())
				{
					std::string prefix;

					if (systemTheme.empty())
					{
						for (auto subsetName : themeColorSets.cbegin()->appliesTo)
						{
							std::string pfx = theme->getViewDisplayName(subsetName);
							if (!pfx.empty())
							{
								if (prefix.empty())
									prefix = pfx;
								else
									prefix = prefix + ", " + pfx;
							}
						}

						if (!prefix.empty())
							prefix = " ("+ prefix+")";

					}					

					themeconfig->addWithLabel(displayName + prefix, item);
				}
				else
					themeconfig->addWithLabel(_("THEME") + " " + Utils::String::toUpper(subset).c_str(), item);
			}

			ThemeConfigOption opt;
			opt.component = item;
			opt.subset = subset;			
			opt.defaultSettingName = settingName;
			options[!perSystemSettingName.empty() ? perSystemSettingName : settingName] = opt;
		}
		else
		{
			ThemeConfigOption opt;
			opt.component = nullptr;
			options[!perSystemSettingName.empty() ? perSystemSettingName : settingName] = opt;
		}
	}


	if (systemTheme.empty())
	{
		themeconfig->addEntry(_("RESET GAMELIST CUSTOMISATIONS"), false, [s, themeconfig, window]
		{
			Settings::getInstance()->setString("GamelistViewStyle", "");
			Settings::getInstance()->setString("DefaultGridSize", "");

			for (auto sysIt = SystemData::sSystemVector.cbegin(); sysIt != SystemData::sSystemVector.cend(); sysIt++)
				(*sysIt)->setSystemViewMode("automatic", Vector2f(0, 0));

			themeconfig->setVariable("reloadAll", true);
			themeconfig->close();
		});
	}

	//  theme_colorset, theme_iconset, theme_menu, theme_systemview, theme_gamelistview, theme_region,
	themeconfig->addSaveFunc([systemTheme, system, themeconfig, theme_set, options, gamelist_style, mGridSize, window]
	{
		bool reloadAll = systemTheme.empty() ? Settings::getInstance()->setString("ThemeSet", theme_set == nullptr ? "" : theme_set->getSelected()) : false;

		for (auto option : options)
		{
			ThemeConfigOption& opt = option.second;

			std::string value;

			if (opt.component != nullptr)
			{
				value = opt.component->getSelected();

				if (!systemTheme.empty() && !value.empty())
				{
					std::string defaultValue = Settings::getInstance()->getString(opt.defaultSettingName);
					if (defaultValue.empty())
						defaultValue = system->getTheme()->getDefaultSubSetValue(opt.subset);

					if (value == defaultValue)
						value = "";
				}
				else if (systemTheme.empty() && value == system->getTheme()->getDefaultSubSetValue(opt.subset))
					value = "";
			}

			if (value != Settings::getInstance()->getString(option.first))
				reloadAll |= Settings::getInstance()->setString(option.first, value);
		}
	
		Vector2f gridSizeOverride(0, 0);

		if (mGridSize != nullptr)
		{
			std::string str = mGridSize->getSelected();
			std::string value = "";

			size_t divider = str.find('x');
			if (divider != std::string::npos)
			{
				std::string first = str.substr(0, divider);
				std::string second = str.substr(divider + 1, std::string::npos);

				gridSizeOverride = Vector2f((float)atof(first.c_str()), (float)atof(second.c_str()));
				value = Utils::String::replace(Utils::String::replace(gridSizeOverride.toString(), ".000000", ""), "0 0", "");
			}

			if (systemTheme.empty())
				reloadAll |= Settings::getInstance()->setString("DefaultGridSize", value);
		}
		else if (systemTheme.empty())
			reloadAll |= Settings::getInstance()->setString("DefaultGridSize", "");
		
		if (systemTheme.empty())
			reloadAll |= Settings::getInstance()->setString("GamelistViewStyle", gamelist_style == nullptr ? "" : gamelist_style->getSelected());
		else
		{
			std::string viewMode = gamelist_style == nullptr ? system->getSystemViewMode() : gamelist_style->getSelected();
			reloadAll |= system->setSystemViewMode(viewMode, gridSizeOverride);
		}

		if (reloadAll || themeconfig->getVariable("reloadAll"))
		{
			if (systemTheme.empty())
			{
				CollectionSystemManager::getInstance()->updateSystemsList();
				ViewController::get()->reloadAll(window);
				window->endRenderLoadingScreen();

				if (theme_set != nullptr)
				{
					std::string oldTheme = Settings::getInstance()->getString("ThemeSet");
					Scripting::fireEvent("theme-changed", theme_set->getSelected(), oldTheme);
				}
			}
			else
			{
				system->loadTheme();
				system->resetFilters();
				ViewController::get()->reloadGameListView(system);
			}
		}
	});

	window->pushGui(themeconfig);
}

void GuiMenu::openUISettings()
{
	auto pthis = this;
	Window* window = mWindow;

	auto s = new GuiSettings(window, _("UI SETTINGS"));
	
	// theme set
	auto theme = ThemeData::getMenuTheme();
	auto themeSets = ThemeData::getThemeSets();
	auto system = ViewController::get()->getState().getSystem();

	if (!themeSets.empty())
	{
		std::map<std::string, ThemeSet>::const_iterator selectedSet = themeSets.find(Settings::getInstance()->getString("ThemeSet"));
		if (selectedSet == themeSets.cend())
			selectedSet = themeSets.cbegin();

		// Load theme randomly
		auto themeRandom = std::make_shared<SwitchComponent>(window, Settings::getInstance()->getBool("ThemeRandom"));

		auto theme_set = std::make_shared< OptionListComponent<std::string> >(window, _("THEMES"), false);
		for (auto it = themeSets.cbegin(); it != themeSets.cend(); it++)
			theme_set->add(it->first, it->first, it == selectedSet);

		s->addWithLabel(_("THEME"), theme_set);
		s->addSaveFunc([s, theme_set, window, themeRandom]
		{
			std::string oldTheme = Settings::getInstance()->getString("ThemeSet");
			if (oldTheme != theme_set->getSelected())
			{
				Settings::getInstance()->setString("ThemeSet", theme_set->getSelected());

				// theme changed without setting options, forcing options to avoid crash/blank theme
				Settings::getInstance()->setString("ThemeRegionName", "");
				Settings::getInstance()->setString("ThemeColorSet", "");
				Settings::getInstance()->setString("ThemeIconSet", "");
				Settings::getInstance()->setString("ThemeMenu", "");
				Settings::getInstance()->setString("ThemeSystemView", "");
				Settings::getInstance()->setString("ThemeGamelistView", "");
				Settings::getInstance()->setString("GamelistViewStyle", "");
				Settings::getInstance()->setString("DefaultGridSize", "");

				for(auto sm : Settings::getInstance()->getStringMap())
					if (Utils::String::startsWith(sm.first, "subset."))
						Settings::getInstance()->setString(sm.first, "");

				for (auto sysIt = SystemData::sSystemVector.cbegin(); sysIt != SystemData::sSystemVector.cend(); sysIt++)
					(*sysIt)->setSystemViewMode("automatic", Vector2f(0, 0));

				s->setVariable("reloadCollections", true);
				s->setVariable("reloadAll", true);
				s->setVariable("reloadGuiMenu", true);

				// if theme is manual set, disable random theme selection
				if (Settings::getInstance()->getBool("ThemeRandom"))
				{
					Settings::getInstance()->setBool("ThemeRandom", Settings::getInstance()->getBool("ThemeRandom"));
					window->pushGui(new GuiMsgBox(window, _("YOU SELECTED A THEME MANUALLY, THE RANDOM THEME SELECTION HAS BEEN DISABLED"), _("OK")));
				}

				Scripting::fireEvent("theme-changed", theme_set->getSelected(), oldTheme);
			}
		});

		bool showThemeConfiguration = (system->getTheme() != nullptr) && ( system->getTheme()->hasSubsets() || system->getTheme()->hasView("grid") );
		if (showThemeConfiguration)
		{
			s->addSubMenu(_("THEME CONFIGURATION"), [this, window, s, theme_set]() { openThemeConfiguration(window, s, theme_set); });
		}
		else // GameList view style only, acts like Retropie for simple themes
		{
			auto gamelist_style = std::make_shared< OptionListComponent<std::string> >(window, _("GAMELIST VIEW STYLE"), false);
			std::vector<std::pair<std::string, std::string>> styles;
			styles.push_back(std::pair<std::string, std::string>("automatic", _("automatic")));

			auto system = ViewController::get()->getState().getSystem();
			if ((system != NULL) && (system->getTheme() != nullptr))
			{
				auto mViews = system->getTheme()->getViewsOfTheme();
				for (auto it = mViews.cbegin(); it != mViews.cend(); ++it)
					styles.push_back(*it);
			}
			else
			{
				styles.push_back(std::pair<std::string, std::string>("basic", _("basic")));
				styles.push_back(std::pair<std::string, std::string>("detailed", _("detailed")));
				styles.push_back(std::pair<std::string, std::string>("video", _("video")));
				styles.push_back(std::pair<std::string, std::string>("grid", _("grid")));
			}

			auto viewPreference = Settings::getInstance()->getString("GamelistViewStyle");
			if ((system->getTheme() == nullptr) || !system->getTheme()->hasView(viewPreference))
				viewPreference = "automatic";

			for (auto it = styles.cbegin(); it != styles.cend(); it++)
				gamelist_style->add(_(it->second.c_str()), it->first, viewPreference == it->first);

			s->addWithLabel(_("GAMELIST VIEW STYLE"), gamelist_style);
			s->addSaveFunc([s, gamelist_style, window] 
			{
				if (Settings::getInstance()->setString("GamelistViewStyle", gamelist_style->getSelected()))
				{
					s->setVariable("reloadAll", true);
					s->setVariable("reloadGuiMenu", true);
				}
			});
		}

			// Load theme randomly
		s->addWithLabel(_("SHOW RANDOM THEME"), themeRandom);
		s->addSaveFunc([themeRandom] {
			Settings::getInstance()->setBool("ThemeRandom", themeRandom->getState());
		});
	}

	// screensaver
	ComponentListRow screensaver_row;
	screensaver_row.elements.clear();
	screensaver_row.addElement(std::make_shared<TextComponent>(window, _("SCREENSAVER SETTINGS"), theme->Text.font, theme->Text.color), true);
	screensaver_row.addElement(makeArrow(window), false);
	screensaver_row.makeAcceptInputHandler(std::bind(&GuiMenu::openScreensaverOptions, this));
	s->addRow(screensaver_row);


	//UI mode
	auto UImodeSelection = std::make_shared< OptionListComponent<std::string> >(window, _("UI MODE"), false);
	std::vector<std::string> UImodes = UIModeController::getInstance()->getUIModes();
	for (auto it = UImodes.cbegin(); it != UImodes.cend(); it++)
		UImodeSelection->add(_(*it), *it, Settings::getInstance()->getString("UIMode") == *it);
	s->addWithLabel(_("UI MODE"), UImodeSelection);

	s->addSaveFunc([UImodeSelection, window]
	{
		std::string selectedMode = UImodeSelection->getSelected();
		if (selectedMode != "Full")
		{
			std::string msg = _("You are changing the UI to a restricted mode:") + "\n" + selectedMode + "\n";
			msg += _("This will hide most menu-options to prevent changes to the system.\nTo unlock and return to the full UI, enter this code:") + "\n";
			msg += "\"" + UIModeController::getInstance()->getFormattedPassKeyStr() + "\"\n\n";
			msg += _("Do you want to proceed?");
			window->pushGui(new GuiMsgBox(window, msg,
				_("YES"), [selectedMode] {
				//LOG(LogDebug) << "GuiMenu::openUISettings() - Setting UI mode to " << selectedMode;
				Settings::getInstance()->setString("UIMode", selectedMode);
				Settings::getInstance()->saveFile();
			}, _("NO"), nullptr));
		}
	});

	// transition style
	auto transition_style = std::make_shared< OptionListComponent<std::string> >(window, _("TRANSITION STYLE"), false);
	std::vector<std::string> transitions;
	transitions.push_back("auto");
	transitions.push_back("fade");
	transitions.push_back("slide");
	transitions.push_back("instant");
	
	for (auto it = transitions.cbegin(); it != transitions.cend(); it++)
		transition_style->add(_(*it), *it, Settings::getInstance()->getString("TransitionStyle") == *it);

	if (!transition_style->hasSelection())
		transition_style->selectFirstItem();

	s->addWithLabel(_("TRANSITION STYLE"), transition_style);
	s->addSaveFunc([transition_style] {
		if (Settings::getInstance()->getString("TransitionStyle") == "instant"
			&& transition_style->getSelected() != "instant"
			&& PowerSaver::getMode() == PowerSaver::INSTANT)
		{
			Settings::getInstance()->setString("PowerSaverMode", "default");
			PowerSaver::init();
		}
		Settings::getInstance()->setString("TransitionStyle", transition_style->getSelected());
		GuiComponent::ALLOWANIMATIONS = Settings::getInstance()->getString("TransitionStyle") != "instant";
	});
	
	auto transitionOfGames_style = std::make_shared< OptionListComponent<std::string> >(window, _("GAME LAUNCH TRANSITION"), false);
	std::vector<std::string> gameTransitions;
	gameTransitions.push_back("fade");
	gameTransitions.push_back("slide");
	gameTransitions.push_back("instant");
	for (auto it = gameTransitions.cbegin(); it != gameTransitions.cend(); it++)
		transitionOfGames_style->add(_(*it), *it, Settings::getInstance()->getString("GameTransitionStyle") == *it);

	s->addWithLabel(_("GAME LAUNCH TRANSITION"), transitionOfGames_style);
	s->addSaveFunc([transitionOfGames_style] {
		if (Settings::getInstance()->getString("GameTransitionStyle") == "instant"
			&& transitionOfGames_style->getSelected() != "instant"
			&& PowerSaver::getMode() == PowerSaver::INSTANT)
		{
			Settings::getInstance()->setString("PowerSaverMode", "default");
			PowerSaver::init();
		}
		Settings::getInstance()->setString("GameTransitionStyle", transitionOfGames_style->getSelected());
	});

	// menus configurations
	s->addEntry(_("MENUS SETTINGS"), true, [this, s] { openMenusSettings(s); });

	// Hide system view
	auto hideSystemView = std::make_shared<SwitchComponent>(window, Settings::getInstance()->getBool("HideSystemView"));
	s->addWithLabel(_("HIDE SYSTEM VIEW"), hideSystemView);
	s->addSaveFunc([hideSystemView] 
	{ 
		bool hideSysView = Settings::getInstance()->getBool("HideSystemView");
		Settings::getInstance()->setBool("HideSystemView", hideSystemView->getState());

		if (!hideSysView && hideSystemView->getState())
			ViewController::get()->goToStart(true);
	});

	// quick system select (left/right in game list view)
	auto quick_sys_select = std::make_shared<SwitchComponent>(window, Settings::getInstance()->getBool("QuickSystemSelect"));
	s->addWithLabel(_("QUICK SYSTEM SELECT"), quick_sys_select);
	s->addSaveFunc([quick_sys_select] { Settings::getInstance()->setBool("QuickSystemSelect", quick_sys_select->getState()); });

	// carousel transition option
	auto move_carousel = std::make_shared<SwitchComponent>(window, Settings::getInstance()->getBool("MoveCarousel"));
	s->addWithLabel(_("CAROUSEL TRANSITIONS"), move_carousel);
	s->addSaveFunc([move_carousel] {
		if (move_carousel->getState()
			&& !Settings::getInstance()->getBool("MoveCarousel")
			&& PowerSaver::getMode() == PowerSaver::INSTANT)
		{
			Settings::getInstance()->setString("PowerSaverMode", "default");
			PowerSaver::init();
		}
		Settings::getInstance()->setBool("MoveCarousel", move_carousel->getState());
	});

	// clock
	auto clock = std::make_shared<SwitchComponent>(window, Settings::getInstance()->getBool("DrawClock"));
	s->addWithLabel(_("SHOW CLOCK"), clock);
	s->addSaveFunc(
		[clock] { Settings::getInstance()->setBool("DrawClock", clock->getState()); });

	// Clock time format (14:42 or 2:42 pm)
	auto tmFormat = std::make_shared<SwitchComponent>(window, Settings::getInstance()->getBool("ClockMode12"));
	s->addWithLabel(_("SHOW CLOCK IN 12-HOUR FORMAT"), tmFormat);
	s->addSaveFunc([tmFormat] {
			Settings::getInstance()->setBool("ClockMode12", tmFormat->getState());
	});

	// lb/rb uses full screen size paging instead of -10/+10 steps
	auto use_fullscreen_paging = std::make_shared<SwitchComponent>(window, Settings::getInstance()->getBool("UseFullscreenPaging"));
	s->addWithLabel(_("USE FULL SCREEN PAGING FOR PU/PD"), use_fullscreen_paging);
	s->addSaveFunc([use_fullscreen_paging] {
		Settings::getInstance()->setBool("UseFullscreenPaging", use_fullscreen_paging->getState());
	});

	// show help
	auto show_help = std::make_shared<SwitchComponent>(window, Settings::getInstance()->getBool("ShowHelpPrompts"));
	s->addWithLabel(_("ON-SCREEN HELP"), show_help);
	s->addSaveFunc([window, s, show_help]
	{
		if (Settings::getInstance()->setBool("ShowHelpPrompts", show_help->getState()))
		{
			if (window->getHelpComponent())
				window->getHelpComponent()->setVisible(show_help->getState());

			s->setVariable("reloadAll", true);
			s->setVariable("reloadGuiMenu", true);
		}
	});

	// Battery indicator
	if (ApiSystem::getInstance()->getBatteryInformation().hasBattery)
	{
		auto batteryStatus = std::make_shared<OptionListComponent<std::string> >(window, _("SHOW BATTERY STATUS"), false);
		batteryStatus->addRange({ { _("NO"), "" },{ _("ICON"), "icon" },{ _("ICON AND TEXT"), "text" } }, Settings::getInstance()->getString("ShowBattery"));
		s->addWithLabel(_("SHOW BATTERY STATUS"), batteryStatus);
		s->addSaveFunc([batteryStatus]
		{
			std::string old_value = Settings::getInstance()->getString("ShowBattery");
			if (old_value != batteryStatus->getSelected())
				Settings::getInstance()->setString("ShowBattery", batteryStatus->getSelected());
		});
	}

	// filenames
	auto hidden_files = std::make_shared<SwitchComponent>(window, Settings::getInstance()->getBool("ShowFilenames"));
	s->addWithLabel(_("SHOW FILENAMES IN LISTS"), hidden_files);
	s->addSaveFunc([hidden_files, s] 
	{ 
		if (Settings::getInstance()->setBool("ShowFilenames", hidden_files->getState()))
		{
			FileData::resetSettings();
			s->setVariable("reloadCollections", true);
			s->setVariable("reloadAll", true);
			s->setVariable("reloadGuiMenu", true);
		}
	});

	// enable filters (ForceDisableFilters)
	auto enable_filter = std::make_shared<SwitchComponent>(window, !Settings::getInstance()->getBool("ForceDisableFilters"));
	s->addWithLabel(_("ENABLE FILTERS"), enable_filter);
	s->addSaveFunc([enable_filter, s] { 
		bool filter_is_enabled = !Settings::getInstance()->getBool("ForceDisableFilters");
		if (Settings::getInstance()->setBool("ForceDisableFilters", !enable_filter->getState()))
		{
			s->setVariable("reloadAll", true);
			s->setVariable("reloadGuiMenu", true);
		}
	});

	// ignore articles when sorting
	auto ignoreArticles = std::make_shared<SwitchComponent>(window, Settings::getInstance()->getBool("IgnoreLeadingArticles"));
	s->addWithLabel(_("IGNORE LEADING ARTICLES WHEN SORTING"), ignoreArticles);
	s->addSaveFunc([s, ignoreArticles]
	{
		if (Settings::getInstance()->setBool("IgnoreLeadingArticles", ignoreArticles->getState()))
		{
			s->setVariable("reloadAll", true);
			s->setVariable("reloadGuiMenu", true);
		}
	});

	s->onFinalize([s, pthis, window]
	{
		if (s->getVariable("reloadCollections"))
			CollectionSystemManager::getInstance()->updateSystemsList();

		if (s->getVariable("forceReloadGames"))
			ViewController::reloadAllGames(window, false);

		if (s->getVariable("reloadAll"))
		{
			ViewController::get()->reloadAll(window);
			window->endRenderLoadingScreen();
		}

		if (s->getVariable("reloadGuiMenu"))
		{
			if (pthis)
				delete pthis;
			window->pushGui(new GuiMenu(window, false));
		}

	});

	window->pushGui(s);
}

void GuiMenu::openSystemEmulatorSettings(SystemData* system)
{
	Window* window = mWindow;
	auto theme = ThemeData::getMenuTheme();

	GuiSettings* s = new GuiSettings(window, system->getFullName().c_str());

	auto emul_choice = std::make_shared<OptionListComponent<std::string>>(window, _("SELECT EMULATOR"), false);
	auto core_choice = std::make_shared<OptionListComponent<std::string>>(window, _("SELECT CORE"), false);

	std::string currentEmul = Settings::getInstance()->getString(system->getName() + ".emulator");
	std::string defaultEmul = (system->getSystemEnvData()->mEmulators.size() == 0 ? "" : system->getSystemEnvData()->mEmulators[0].mName);

//	if (defaultEmul.length() == 0)
		emul_choice->add(_("AUTO"), "", false);
//	else
//		emul_choice->add(_("AUTO") + " (" + defaultEmul + ")", "", currentEmul.length() == 0);

	bool found = false;
	for (auto core : system->getSystemEnvData()->mEmulators)
	{
		if (core.mName == currentEmul)
			found = true;

		emul_choice->add(core.mName, core.mName, core.mName == currentEmul);
	}

	if (!found)
		emul_choice->selectFirstItem();
	
	ComponentListRow row;
	row.addElement(std::make_shared<TextComponent>(window, _("EMULATOR"), theme->Text.font, theme->Text.color), true);
	row.addElement(emul_choice, false);

	s->addRow(row);

	emul_choice->setSelectedChangedCallback([this, system, core_choice](std::string emulatorName)
	{
		std::string currentCore = Settings::getInstance()->getString(system->getName() + ".core");
		std::string defaultCore;
		
		for (auto& emulator : system->getSystemEnvData()->mEmulators)
		{
			if (emulatorName == emulator.mName)
			{
				for (auto core : emulator.mCores)
				{
					defaultCore = core;
					break;
				}
			}
		}

		core_choice->clear();

	//	if (defaultCore.length() == 0)
			core_choice->add(_("AUTO"), "", false);
	//	else
	//		core_choice->add(_("AUTO") + " (" + defaultCore + ")", "", false);

		std::vector<std::string> cores = system->getSystemEnvData()->getCores(emulatorName);

		bool found = false;

		for (auto it = cores.begin(); it != cores.end(); it++)
		{
			std::string core = *it;
			core_choice->add(core, core, currentCore == core);
			if (currentCore == core)
				found = true;
		}

		if (!found)
			core_choice->selectFirstItem();
		else
			core_choice->invalidate();
	});

	row.elements.clear();
	row.addElement(std::make_shared<TextComponent>(window, _("CORE"), theme->Text.font, theme->Text.color), true);
	row.addElement(core_choice, false);
	s->addRow(row);

	// force change event to load core list
	emul_choice->invalidate();


	s->addSaveFunc([system, emul_choice, core_choice]
	{		
		Settings::getInstance()->setString(system->getName() + ".emulator", emul_choice->getSelected());
		Settings::getInstance()->setString(system->getName() + ".core", core_choice->getSelected());
	});

	window->pushGui(s);
}

void GuiMenu::openEmulatorSettings()
{
	Window* window = mWindow;
	GuiSettings* configuration = new GuiSettings(window, _("EMULATOR SETTINGS").c_str());

	// For each activated system
	for (auto system : SystemData::sSystemVector)
	{
		if (system->isCollection())
			continue;

		if (system->getSystemEnvData()->mEmulators.size() == 0)
			continue;

		if (system->getSystemEnvData()->mEmulators.size() == 1 && system->getSystemEnvData()->mEmulators[0].mCores.size() <= 1)
			continue;
		
		configuration->addEntry(system->getFullName(), true, [this, system] { openSystemEmulatorSettings(system); });
	}

	window->pushGui(configuration);
}

void GuiMenu::updateGameLists(Window* window, bool confirm)
{
	if (ThreadedScraper::isRunning())
	{
		window->pushGui(new GuiMsgBox(window, _("SCRAPING IS RUNNING. DO YOU WANT TO STOP IT ?"),
			_("YES"), [] { ThreadedScraper::stop(); },
			_("NO"), nullptr));

		return;
	}

	if (!confirm)
	{
		ViewController::reloadAllGames(window, true);
		return;
	}

	window->pushGui(new GuiMsgBox(window, _("REALLY UPDATE GAMES LISTS ?"), _("YES"),
		[window] { ViewController::reloadAllGames(window, true); },
		_("NO"), nullptr));
}

void GuiMenu::openWifiSettings(Window* window, std::string title, std::string data, const std::function<bool(std::string)>& onsave)
{
	window->pushGui(new GuiWifi(window, title, _("\"(**)\" WIFI ALREADY CONFIGURED, NOT NEED ENTER PASSWORD TO CONNECT"), data, onsave));
}

void GuiMenu::openRemoteServicesSettings()
{
	Window* window = mWindow;
	window->pushGui(new GuiRemoteServicesOptions(window));
}

void GuiMenu::openNetworkSettings(bool selectWifiEnable, bool selectManualWifiDnsEnable)
{
	const bool baseWifiEnabled = SystemConf::getInstance()->getBool("wifi.enabled"),
			   baseManualDns = SystemConf::getInstance()->getBool("wifi.manual_dns");
	const std::string baseHostname = SystemConf::getInstance()->get("system.hostname"),
					  baseDnsOne = SystemConf::getInstance()->get("wifi.dns1"),
					  baseDnsTwo = SystemConf::getInstance()->get("wifi.dns2");

	auto theme = ThemeData::getMenuTheme();
	std::shared_ptr<Font> font = theme->Text.font;
	unsigned int color = theme->Text.color;

	Window *window = mWindow;

	auto s = new UpdatableGuiSettings(window, _("NETWORK SETTINGS").c_str());
	s->addGroup(_("INFORMATIONS"));

	auto ip = std::make_shared<UpdatableTextComponent>(window, ApiSystem::getInstance()->getIpAddress(), font, color);
	s->addWithLabel(_("IP ADDRESS"), ip);

	auto status = std::make_shared<TextComponent>(window, formatNetworkStatus( ApiSystem::getInstance()->getInternetStatus() ), font, color);
	s->addWithLabel(_("INTERNET STATUS"), status);

	// Network Indicator
	auto networkIndicator = std::make_shared<SwitchComponent>(window, Settings::getInstance()->getBool("ShowNetworkIndicator"));
	s->addWithLabel(_("SHOW NETWORK INDICATOR"), networkIndicator);
	s->addSaveFunc([networkIndicator] { Settings::getInstance()->setBool("ShowNetworkIndicator", networkIndicator->getState()); });

	s->addGroup(_("SETTINGS"));

	// Hostname
	s->addInputTextRow(_("HOSTNAME"), "system.hostname", false, false);

	if (ApiSystem::getInstance()->isScriptingSupported(ApiSystem::ScriptId::REMOTE_SERVICES))
		s->addEntry(_("REMOTE SERVICES SETTINGS"), true, [this] { openRemoteServicesSettings(); });

	// Automatically enable or disable WIFI power saving mode
	auto wifi_powersave = std::make_shared<SwitchComponent>(window, ApiSystem::getInstance()->isWifiPowerSafeEnabled());
	s->addWithLabel(_("ENABLE WIFI POWERSAVE"), wifi_powersave);
	s->addSaveFunc([wifi_powersave]
		{
			ApiSystem::getInstance()->setWifiPowerSafe( wifi_powersave->getState() );
		});

	// Wifi enable
	auto enable_wifi = std::make_shared<SwitchComponent>(window, baseWifiEnabled);
	s->addWithLabel(_("ENABLE WIFI"), enable_wifi, selectWifiEnable);

	// window, title, settingstring,
	const std::string baseSSID = SystemConf::getInstance()->get("wifi.ssid"),
										baseKEY = SystemConf::getInstance()->get("wifi.key");

	// manual dns enable
	auto manual_dns = std::make_shared<SwitchComponent>(window, baseManualDns);

	if (baseWifiEnabled)
	{
		s->addInputTextRow(_("WIFI SSID"), "wifi.ssid", false, false, &openWifiSettings);
		s->addInputTextRow(_("WIFI KEY"), "wifi.key", true, false);

		s->addEntry(_("RESET WIFI CONECTION"), false, [this, s] { resetNetworkSettings(s); });

		s->addWithLabel(_("MANUAL DNS"), manual_dns, selectManualWifiDnsEnable);

		if (baseManualDns)
		{
			s->addInputTextRow(_("DNS1"), "wifi.dns1", false, false, nullptr, &Utils::Network::validateIPv4);
			s->addInputTextRow(_("DNS2"), "wifi.dns2", false, false, nullptr, &Utils::Network::validateIPv4);
		}
	}

	s->addSaveFunc([this, s, enable_wifi, manual_dns, baseManualDns, baseDnsOne, baseDnsTwo, window]
		{
			if (enable_wifi->getState())
			{
				bool manualDns = manual_dns->getState();
				std::string ssid = SystemConf::getInstance()->get("wifi.ssid");

				if (ssid.empty())
					return;

				if (manualDns)
				{
					std::string dnsOne = SystemConf::getInstance()->get("wifi.dns1"),
											dnsTwo = SystemConf::getInstance()->get("wifi.dns2");
					if (baseDnsOne != dnsOne || baseDnsTwo != dnsTwo || !baseManualDns)
					{
						window->pushGui(new GuiLoading<bool>(window, _("CONFIGURING MANUAL DNS"),
							[this, s, ssid, dnsOne, dnsTwo]
							{
								s->setWaitingLoad(true);
								return ApiSystem::getInstance()->enableManualWifiDns(ssid, dnsOne, dnsTwo);
							},
							[this, window, s](bool success)
							{
								s->setWaitingLoad(false);
								if (success)
									window->pushGui(new GuiMsgBox(window, _("MANUAL DNS ENABLED")));
								else
									window->pushGui(new GuiMsgBox(window, _("DNS CONFIGURATION ERROR"), GuiMsgBoxIcon::ICON_ERROR));
							}));
					}
				}
				else if (baseManualDns)
				{
					window->pushGui(new GuiLoading<bool>(window, _("RESETING DNS CONFIGURATION"),
						[this, window, s, ssid]
						{
							s->setWaitingLoad(true);
							return ApiSystem::getInstance()->disableManualWifiDns(ssid);
						},
						[this, window, s](bool success)
						{
							s->setWaitingLoad(false);
							if (!success)
								window->pushGui(new GuiMsgBox(window, _("DNS CONFIGURATION ERROR"), GuiMsgBoxIcon::ICON_ERROR));
						}));
				}
			}
		});

	s->addSaveFunc([this, s, window, baseWifiEnabled, baseSSID, baseKEY, baseHostname, enable_wifi, baseManualDns, baseDnsOne, baseDnsTwo]
		{
			bool wifienabled = enable_wifi->getState();

			if (baseHostname != SystemConf::getInstance()->get("system.hostname"))
				ApiSystem::getInstance()->setHostname(SystemConf::getInstance()->get("system.hostname"));

			SystemConf::getInstance()->setBool("wifi.enabled", wifienabled);

			if (wifienabled)
			{
				std::string newSSID = SystemConf::getInstance()->get("wifi.ssid"),
										newKey = SystemConf::getInstance()->get("wifi.key");

				if (newSSID.empty())
				{
					ApiSystem::getInstance()->enableWifi();
					return;
				}

				if (baseSSID != newSSID || baseKEY != newKey || !baseWifiEnabled)
				{
					window->pushGui(new GuiLoading<bool>(window, _("CONNETING TO WIFI") + " '" + newSSID + "'",
						[this, s, newSSID, newKey]
						{
							s->setWaitingLoad(true);
							return ApiSystem::getInstance()->connectWifi(newSSID, newKey);
						},
						[this, window, s, newSSID](bool success)
						{
							s->setWaitingLoad(false);
							if (success)
							{
								window->pushGui(new GuiMsgBox(window, "'" + newSSID + "' - " + _("WIFI ENABLED")));
								s->setVariable("reloadGuiMenu", true);
							}
							else
								window->pushGui(new GuiMsgBox(window, "'" + newSSID + "' - " + _("WIFI CONFIGURATION ERROR"), GuiMsgBoxIcon::ICON_ERROR));
						}));
				}
			}
			else if (baseWifiEnabled)
			{
				window->pushGui(new GuiLoading<bool>(window, _("DISCONNETING TO WIFI") + " '" + baseSSID + "'",
					[this, s]
					{
						s->setWaitingLoad(true);
						return ApiSystem::getInstance()->disableWifi(false);
					},
					[this, window, s, baseSSID](bool success)
					{
						s->setWaitingLoad(false);
						if (success)
							s->setVariable("reloadGuiMenu", true);
						else
							window->pushGui(new GuiMsgBox(window, "'" + baseSSID + "' - " + _("WIFI CONFIGURATION ERROR"), GuiMsgBoxIcon::ICON_ERROR));
					}));
			}
		});

	enable_wifi->setOnChangedCallback([this, s, baseWifiEnabled, enable_wifi, window]()
		{
			bool wifienabled = enable_wifi->getState();
			if (baseWifiEnabled != wifienabled)
			{
				SystemConf::getInstance()->setBool("wifi.enabled", wifienabled);

				std::string ssid = SystemConf::getInstance()->get("wifi.ssid");

				if (wifienabled)
				{
					if (ssid.empty())
					{
						ApiSystem::getInstance()->enableWifi();
						delete s;
						openNetworkSettings(true, false);
						return;
					}

					window->pushGui(new GuiLoading<bool>(window, _("CONNETING TO WIFI") + " '" + ssid + "'",
						[this, s, ssid]
						{
							s->setWaitingLoad(true);
							return ApiSystem::getInstance()->connectWifi(ssid, SystemConf::getInstance()->get("wifi.key"));
						},
						[this, window, s, ssid](bool success)
						{
							s->setWaitingLoad(false);
							if (success)
							{
								window->displayNotificationMessage(_U("\uF25B  ") + ssid + " - " + _("WIFI ENABLED"), 10000);
								s->setVariable("reloadGuiMenu", true);
							}
							else
								window->pushGui(new GuiMsgBox(window, "'" + ssid + "' - " + _("WIFI CONFIGURATION ERROR"), GuiMsgBoxIcon::ICON_ERROR));

							delete s;
							openNetworkSettings(true, false);
						}));
				}
				else
				{
					if (ssid.empty())
					{
						ApiSystem::getInstance()->disableWifi();
						delete s;
						openNetworkSettings(true, false);
						return;
					}
					window->pushGui(new GuiLoading<bool>(window, _("DISCONNETING TO WIFI") + " '" + ssid + "'",
						[this, s, ssid]
						{
							s->setWaitingLoad(true);
							return ApiSystem::getInstance()->disableWifi(false);
						},
						[this, window, s, ssid](bool success)
						{
							s->setWaitingLoad(false);
							if (success)
								s->setVariable("reloadGuiMenu", true);
							else
								window->pushGui(new GuiMsgBox(window, "'" + ssid + "' - " + _("WIFI CONFIGURATION ERROR"), GuiMsgBoxIcon::ICON_ERROR));

							delete s;
							openNetworkSettings(true, false);
						}));
				}
			}
		});

	manual_dns->setOnChangedCallback([this, s, window, baseManualDns, manual_dns]()
		{
			bool manualDns = manual_dns->getState();

			if (baseManualDns != manualDns)
			{
				SystemConf::getInstance()->setBool("wifi.manual_dns", manualDns);
				std::string ssid = SystemConf::getInstance()->get("wifi.ssid");

				if (ssid.empty())
				{
					delete s;
					openNetworkSettings(false, true);
					return;
				}

				if (manualDns)
				{
					window->pushGui(new GuiLoading<bool>(window, _("CONFIGURING MANUAL DNS"),
						[this, s, ssid]
						{
							s->setWaitingLoad(true);
							return ApiSystem::getInstance()->enableManualWifiDns(ssid, SystemConf::getInstance()->get("wifi.dns1"), SystemConf::getInstance()->get("wifi.dns2"));
						},
						[this, window, s](bool success)
						{
							s->setWaitingLoad(false);
							if (success)
								window->displayNotificationMessage(_U("\uF25B  ") + _("DNS CONFIGURATION SUCCESFULLY"), 10000);
							else
								window->displayNotificationMessage(_U("\uF071  ") + _("DNS CONFIGURATION ERROR"), 10000);

							delete s;
							openNetworkSettings(false, true);
						}));
				}
				else
				{
					window->pushGui(new GuiLoading<bool>(window, _("RESETING DNS CONFIGURATION"),
						[this, s, ssid]
						{
							s->setWaitingLoad(true);
							return ApiSystem::getInstance()->disableManualWifiDns(ssid);
						},
						[this, window, s](bool success)
						{
							s->setWaitingLoad(false);
							if (!success)
								window->displayNotificationMessage(_U("\uF071  ") + _("DNS CONFIGURATION ERROR"), 10000);

							delete s;
							openNetworkSettings(false, true);
						}));
				}
			}
		});

	if (selectWifiEnable || selectManualWifiDnsEnable)
	{
		s->clear();
		ip->setUpdatableFunction([ip, status]
			{
				ip->setText(ApiSystem::getInstance()->getIpAddress());
				status->setText(formatNetworkStatus( ApiSystem::getInstance()->getInternetStatus() ));
			}, 5000);
		s->addUpdatableComponent(ip.get());
	}

	auto pthis = this;

	s->onClose([s, pthis, window]
	{
		if (s->getVariable("reloadGuiMenu"))
		{
			if (pthis)
				delete pthis;
			window->pushGui(new GuiMenu(window, false));
		}
	});

	window->pushGui(s);
}

void GuiMenu::resetNetworkSettings(GuiSettings *gui)
{
	ApiSystem::getInstance()->resetWifi(SystemConf::getInstance()->get("wifi.ssid"));
	SystemConf::getInstance()->setBool("wifi.enabled", false);
	SystemConf::getInstance()->set("wifi.ssid", "");
	SystemConf::getInstance()->set("wifi.key", "");
	SystemConf::getInstance()->setBool("wifi.manual_dns", false);
	SystemConf::getInstance()->set("wifi.dns1", "");
	SystemConf::getInstance()->set("wifi.dns2", "");

	Window* window = mWindow;
	window->peekGui();
	window->removeGui(gui);
	delete gui;
	openNetworkSettings(true, false);
}

void GuiMenu::openBluetoothSettings()
{
	const bool baseBtEnabled = SystemConf::getInstance()->getBool("bluetooth.enabled");
	const bool baseBtAudioConnected = SystemConf::getInstance()->getBool("bluetooth.audio.connected");
	const std::string baseBtAudioDevice = SystemConf::getInstance()->get("bluetooth.audio.device");

	if (baseBtEnabled)
		ApiSystem::getInstance()->startBluetoothLiveScan();

	// stop sound to prevent problems
	AudioManager::getInstance()->deinit();
	VolumeControl::getInstance()->deinit();

	Window *window = mWindow;
	auto s = new GuiSettings(window, _("BLUETOOTH SETTINGS").c_str());
	
	s->setVariable("reinitSoundComponents", true);

	// Bluetooth enable
	auto enable_bt = std::make_shared<SwitchComponent>(window, baseBtEnabled);
	s->addWithLabel(_("ENABLE BLUETOOTH"), enable_bt);
	enable_bt->setOnChangedCallback([this, window, s, baseBtEnabled, baseBtAudioConnected, enable_bt]()
		{
			bool bt_enabled = enable_bt->getState();

			if (bt_enabled)
			{
				window->pushGui(new GuiLoading<bool>(window, _("ENABLING BLUETOOTH ..."), 
					[this, window, s]
					{
						s->setWaitingLoad(true);
						bool result = ApiSystem::getInstance()->enableBluetooth();
						SystemConf::getInstance()->setBool("bluetooth.enabled", result);
						if (result)
						{
							ApiSystem::getInstance()->startBluetoothLiveScan();
							Settings::getInstance()->saveFile();
						}
						return result;
					},
					[this, window, s](bool result)
					{
						s->setWaitingLoad(false);
						if (result)
							s->setVariable("reloadGuiMenu", true);

						delete s;
						openBluetoothSettings();
					}));
			}
			else
			{
				window->pushGui(new GuiLoading<bool>(window, _("DISABLING BLUETOOTH ..."), 
					[this, window, s]
					{
						s->setWaitingLoad(true);
						ApiSystem::getInstance()->stopBluetoothLiveScan();
						bool result = ApiSystem::getInstance()->disableBluetooth();
						SystemConf::getInstance()->setBool("bluetooth.enabled", !result);
						if (result)
							Settings::getInstance()->saveFile();
						
						return result;
					},
					[this, window, s, baseBtAudioConnected](bool result)
					{
						s->setWaitingLoad(false);
						if (result)
						{ // successfully disabled
							SystemConf::getInstance()->setBool("bluetooth.audio.connected", false);
							SystemConf::getInstance()->set("bluetooth.audio.device", "");

							if (baseBtAudioConnected)
							{
								s->setVariable("reinitSoundComponents", false);
								displayBluetoothAudioRestartDialog(window, true);
								return;
							}

							s->setVariable("reloadGuiMenu", true);
						}
						if (!result || !baseBtAudioConnected)
						{
							delete s;
							openBluetoothSettings();
						}
					}));
			}
		});

	if (baseBtEnabled)
	{
		std::string bt_title = _("SEARCH NEW BLUETOOTH DEVICES");
		s->addEntry(bt_title, true, [this, window, bt_title] { openBluetoothScanDevices(window, bt_title); });

		bt_title = _("PAIRED BLUETOOTH DEVICES");
		s->addEntry(bt_title, true, [this, window, bt_title] { openBluetoothPairedDevices(window, bt_title); });

		bt_title = _("CONNECTED BLUETOOTH DEVICES");
		s->addEntry(bt_title, true, [this, window, bt_title] { openBluetoothConnectedDevices(window, bt_title); });

		if (baseBtAudioConnected)
		{
			auto theme = ThemeData::getMenuTheme();
			std::shared_ptr<Font> font = theme->Text.font;
			unsigned int color = theme->Text.color;

			s->addWithLabel(_("AUDIO DEVICE"), std::make_shared<TextComponent>(window, baseBtAudioDevice, font, color));
		}

		// autoconnect audio device on boot
		auto auto_connect = std::make_shared<SwitchComponent>(window, Settings::getInstance()->getBool("bluetooth.audio.device.autoconnect"));
		s->addWithLabel(_("AUTO CONNECT AUDIO DEVICES ON BOOT"), auto_connect);
		auto_connect->setOnChangedCallback([this, s, auto_connect]
		{
			SystemConf::getInstance()->setBool("bluetooth.audio.device.autoconnect", auto_connect->getState());
		});

		auto auto_connect_timeout = std::make_shared<SliderComponent>(window, 0.f, 20.f, 1.0f, "s");
		auto_connect_timeout->setValue( (float)Settings::getInstance()->getInt("bluetooth.boot.game.timeout") );
		s->addWithDescription(_("WAIT BEFORE BOOT GAME LAUNCH"), _("TIMEOUT BEFORE LAUNCH THE BOOT GAME"), auto_connect_timeout);
		auto_connect_timeout->setOnValueChanged([window, s](const float &newVal)
			{
				int timeout = (int)Math::round(newVal);
				Settings::getInstance()->setInt("bluetooth.boot.game.timeout", timeout);
			});
	}

	auto pthis = this;

	s->onClose([s, pthis, window]
	{
		ApiSystem::getInstance()->stopBluetoothLiveScan();

		bool es_abt = SystemConf::getInstance()->getBool("bluetooth.audio.connected"),
			 sys_abt = ApiSystem::getInstance()->isBluetoothAudioDeviceConnected();

		if (es_abt != sys_abt)
		{
			if (pthis->displayBluetoothAudioRestartDialog(window))
				return;
		}

		if (s->getVariable("reinitSoundComponents"))
			pthis->reinitSoundComponentsAndMusic();

		if (s->getVariable("reloadGuiMenu"))
		{
			if (pthis)
				delete pthis;
			window->pushGui(new GuiMenu(window, false));
		}

	});

	window->pushGui(s);
}

void GuiMenu::reinitSoundComponentsAndMusic()
{
	// restart sound
	VolumeControl::getInstance()->init();
	AudioManager::getInstance()->init();

	// Play music
	if (ViewController::get()->getState().viewing == ViewController::GAME_LIST || ViewController::get()->getState().viewing == ViewController::SYSTEM_SELECT)
		AudioManager::getInstance()->changePlaylist(ViewController::get()->getState().getSystem()->getTheme(), true);
	else
		AudioManager::getInstance()->playRandomMusic();
}

void GuiMenu::openBluetoothScanDevices(Window* window, std::string title)
{
	window->pushGui(new GuiBluetoothScan(window, title));
}

void GuiMenu::openBluetoothPairedDevices(Window* window, std::string title)
{
	window->pushGui(new GuiBluetoothPaired(window, title, _("\"(**)\" ALREADY CONNECTED")));
}

void GuiMenu::openBluetoothConnectedDevices(Window* window, std::string title)
{
	window->pushGui(new GuiBluetoothConnected(window, title));
}

bool GuiMenu::displayBluetoothAudioRestartDialog(Window *window, bool force)
{
	bool restart = true;
	std::string msg = _("THE AUDIO INTERFACE HAS CHANGED.") + "\n" + _("THE EMULATIONSTATION WILL NOW RESTART.");
	Settings::getInstance()->saveFile();
	if (force)
	{
		window->pushGui(new GuiMsgBox(window, msg,
			_("YES"),
				[]
				{
					Utils::FileSystem::createFile(Utils::FileSystem::getEsConfigPath() + "/skip_auto_connect_bt_audio_device_onboot.lock");
					if (quitES(QuitMode::RESTART) != 0)
						LOG(LogWarning) << "GuiMenu::displayBluetoothAudioRestartDialog() - Restart terminated with non-zero result!";
				}));
	}
	else
	{
		window->pushGui(new GuiMsgBox(window, msg,
			_("YES"),
				[]
				{
					Utils::FileSystem::createFile(Utils::FileSystem::getEsConfigPath() + "/skip_auto_connect_bt_audio_device_onboot.lock");
					if (quitES(QuitMode::RESTART) != 0)
						LOG(LogWarning) << "GuiMenu::displayBluetoothAudioRestartDialog() - Restart terminated with non-zero result!";
				}, _("NO"),
				[&restart]
				{
					//LOG(LogDebug) << "GuiMenu::displayBluetoothAudioRestartDialog() - NO restarting Emulationstation!";
					restart = false;
				}));
	}
	return restart;
}

void GuiMenu::openUpdateSettings()
{
	Window* window = mWindow;
	auto s = new GuiSettings(window, _("DOWNLOADS AND UPDATES"));

	// themes installer/browser
	s->addEntry(_("THEME INSTALLER"), true, [this, window]
	{
		window->pushGui(new GuiThemeInstall(window));
	});

	// Enable updates
	auto updates_enabled = std::make_shared<SwitchComponent>(window, Settings::getInstance()->getBool("updates.enabled"));
	s->addWithLabel(_("AUTO UPDATES"), updates_enabled);
	s->addSaveFunc([updates_enabled]
	{
		Settings::getInstance()->setBool("updates.enabled", updates_enabled->getState());
	});

	// Start update
	s->addEntry(ApiSystem::getInstance()->state == UpdateState::State::UPDATE_READY ? _("APPLY UPDATE") : _("START UPDATE"), true, [this, window, s]
	{
		if (ApiSystem::getInstance()->checkUpdateVersion().empty())
		{
			window->pushGui(new GuiMsgBox(window, _("NO UPDATE AVAILABLE")));
			return;
		}

		if (ApiSystem::getInstance()->state == UpdateState::State::UPDATE_READY)
		{
			if (quitES(QuitMode::QUIT))
				LOG(LogWarning) << "GuiMenu::openUpdateSettings() - Reboot terminated with non-zero result!";
		}
		else if (ApiSystem::getInstance()->state == UpdateState::State::UPDATER_RUNNING)
			window->pushGui(new GuiMsgBox(window, _("UPDATE IS ALREADY RUNNING")));
		else
		{
			ApiSystem::getInstance()->startUpdate(window);

			s->setVariable("closeGuiMenu", true);
			s->close();			
		}
	});

	auto pthis = this;

	s->onFinalize([s, pthis]
	{
		if (s->getVariable("closeGuiMenu"))
		{
			if (pthis)
				delete pthis;
		}
	});

	window->pushGui(s);

}


void GuiMenu::openAdvancedSettings()
{
	Window* window = mWindow;
	auto s = new GuiSettings(window, _("ADVANCED SETTINGS"));

	auto theme = ThemeData::getMenuTheme();

	//Timezone - Adapted from emuelec
	auto es_timezones = std::make_shared<OptionListComponent<std::string>>(window, _("SELECT YOUR TIMEZONE"), false, true);

	std::string currentTimezone = SystemConf::getInstance()->get("system.timezone");
	if (currentTimezone.empty())
		currentTimezone = ApiSystem::getInstance()->getCurrentTimezone();

	std::string a;
	bool valid_tz = false;
	for(std::stringstream ss(ApiSystem::getInstance()->getTimezones()); getline(ss, a, ','); ) {
		es_timezones->add(a, a, currentTimezone == a);
		if (currentTimezone == a)
			valid_tz = true;
	}
	if (!valid_tz)
		currentTimezone = "Europe/Paris";

	s->addWithLabel(_("TIMEZONE"), es_timezones);
	s->addSaveFunc([es_timezones] {
		if (es_timezones->changed()) {
			std::string selectedTimezone = es_timezones->getSelected();
			ApiSystem::getInstance()->setTimezone(selectedTimezone);
			SystemConf::getInstance()->set("system.timezone", selectedTimezone);
		}
	});

	// LANGUAGE
	std::vector<std::string> languages;
	languages.push_back("en");

	std::string xmlpath = ResourceManager::getInstance()->getResourcePath(":/splash.svg");
	if (xmlpath.length() > 0)
	{
		xmlpath = Utils::FileSystem::getParent(xmlpath) + "/locale/";

		Utils::FileSystem::stringList dirContent = Utils::FileSystem::getDirContent(xmlpath, true);
		for (Utils::FileSystem::stringList::const_iterator it = dirContent.cbegin(); it != dirContent.cend(); ++it)
		{
			if (Utils::FileSystem::isDirectory(*it))
				continue;

			std::string name = *it;

			if (name.rfind("emulationstation2.po") == std::string::npos)
				continue;

			name = Utils::FileSystem::createRelativePath(name, xmlpath, false);
			if (Utils::String::startsWith(name, "./"))
			{
				name = name.substr(2);

				while (name.find("/") != std::string::npos)
					name = Utils::FileSystem::getParent(name);
			}
			else
				name = Utils::FileSystem::getParent(name);

			name = Utils::FileSystem::getFileName(name);

			if (name != "en")
				languages.push_back(name);
		}

		if (languages.size() > 1)
		{
			auto language = std::make_shared< FlagOptionListComponent<std::string> >(window, _("LANGUAGE"));
			std::string language_value = Settings::getInstance()->getString("Language");
			for (auto it = languages.cbegin(); it != languages.cend(); it++)
			{
				std::string language_label;
				if (*it == "br")
					language_label = "PORTUGUESE BRAZIL";
				else if (*it == "de")
					language_label = "DEUTSCHE";
				else if (*it == "es")
					language_label = "SPANISH";
				else if (*it == "en")
					language_label = "ENGLISH";
				else if (*it == "fr")
					language_label = "FRENCH";
				else if (*it == "ko")
					language_label = "KOREAN";
				else if (*it == "pt")
					language_label = "PORTUGUESE PORTUGAL";
				else if (*it == "zh-CN")
					language_label = "SIMPLIFIED CHINESE";
				else if (*it == "it")
					language_label = "ITALIAN";
				else
					language_label = *it;

				language->add(_(language_label), *it, language_value == *it, ":/flags/" + *it + ".png");
			}

			s->addWithLabel(_("LANGUAGE"), language);
			s->addSaveFunc([language, window, s, language_value] {

				if (language_value != language->getSelected())
				{
					Settings::getInstance()->setString("Language", language->getSelected());
					s->setVariable("reloadGuiMenu", true);
					if (ApiSystem::getInstance()->isScriptingSupported(ApiSystem::ScriptId::LANGUAGE))
						ApiSystem::getInstance()->setLanguage(language->getSelected());
				}
			});
		}
	}


	// power saver
	auto power_saver = std::make_shared< OptionListComponent<std::string> >(window, _("POWER SAVER MODES"), false);
	std::vector<std::string> ps_modes;
	ps_modes.push_back("disabled");
	ps_modes.push_back("default");
	ps_modes.push_back("enhanced");
	ps_modes.push_back("instant");
	for (auto it = ps_modes.cbegin(); it != ps_modes.cend(); it++)
		power_saver->add(_(it->c_str()), *it, Settings::getInstance()->getString("PowerSaverMode") == *it);

	s->addWithLabel(_("POWER SAVER MODES"), power_saver);
	s->addSaveFunc([this, power_saver] {
		if (Settings::getInstance()->getString("PowerSaverMode") != "instant" && power_saver->getSelected() == "instant") {
			Settings::getInstance()->setString("TransitionStyle", "instant");
			Settings::getInstance()->setString("GameTransitionStyle", "instant");
			Settings::getInstance()->setBool("MoveCarousel", false);
			Settings::getInstance()->setBool("EnableSounds", false);
		}

		GuiComponent::ALLOWANIMATIONS = Settings::getInstance()->getString("TransitionStyle") != "instant";

		Settings::getInstance()->setString("PowerSaverMode", power_saver->getSelected());
		PowerSaver::init();
	});

	if (ApiSystem::getInstance()->isScriptingSupported(ApiSystem::ScriptId::OPTMIZE_SYSTEM))
	{
		// sleep / suspend modes
		auto suspend_mode = std::make_shared< OptionListComponent<std::string> >(window, _("SUSPEND MODES"), false);
		std::vector<std::string> suspend_modes = ApiSystem::getInstance()->getSuspendModes();
		std::string sm_value = ApiSystem::getInstance()->getSuspendMode();
		for (auto it = suspend_modes.cbegin(); it != suspend_modes.cend(); it++)
			suspend_mode->add(_(it->c_str()), *it, sm_value == *it);
		
		s->addWithLabel(_("SUSPEND MODES"), suspend_mode);
		s->addSaveFunc([this, s, suspend_mode]
			{
				if (SystemConf::getInstance()->set("suspend.device.mode", suspend_mode->getSelected()))
				{
					ApiSystem::getInstance()->setSuspendMode(suspend_mode->getSelected());
					if (suspend_mode->getSelected() == "DISABLED")
					{
						if (Settings::getInstance()->setString("OnlyExitAction", "shutdown")
							&& Settings::getInstance()->getBool("ShowOnlyExit"))
							s->setVariable("reloadGuiMenu", true);
					}
				}
			});

	}

	s->addGroup(_("GAME LIST"));

	// gamelists
	auto save_gamelists = std::make_shared<SwitchComponent>(window, Settings::getInstance()->getBool("SaveGamelistsOnExit"));
	s->addWithLabel(_("SAVE METADATA ON EXIT"), save_gamelists);
	s->addSaveFunc([save_gamelists] { Settings::getInstance()->setBool("SaveGamelistsOnExit", save_gamelists->getState()); });

	auto parse_gamelists = std::make_shared<SwitchComponent>(window, Settings::getInstance()->getBool("ParseGamelistOnly"));
	s->addWithLabel(_("PARSE GAMESLISTS ONLY"), parse_gamelists);
	s->addSaveFunc([parse_gamelists] { Settings::getInstance()->setBool("ParseGamelistOnly", parse_gamelists->getState()); });

	auto local_art = std::make_shared<SwitchComponent>(window, Settings::getInstance()->getBool("LocalArt"));
	s->addWithLabel(_("SEARCH FOR LOCAL ART"), local_art);
	s->addSaveFunc([local_art] { Settings::getInstance()->setBool("LocalArt", local_art->getState()); });


	s->addGroup(_("PERFORMANCE"));

	// maximum vram
	auto max_vram = std::make_shared<SliderComponent>(window, 40.f, 1000.f, 10.f, "Mb");
	max_vram->setValue((float)(Settings::getInstance()->getInt("MaxVRAM")));
	s->addWithLabel(_("VRAM LIMIT"), max_vram);
	s->addSaveFunc([max_vram] { Settings::getInstance()->setInt("MaxVRAM", (int)Math::round(max_vram->getValue())); });

	// preload UI
	auto preloadUI = std::make_shared<SwitchComponent>(window, Settings::getInstance()->getBool("PreloadUI"));
	s->addWithLabel(_("PRELOAD UI"), preloadUI);
	s->addSaveFunc([preloadUI] { Settings::getInstance()->setBool("PreloadUI", preloadUI->getState()); });
	
	// optimize VRAM
	auto optimizeVram = std::make_shared<SwitchComponent>(window, Settings::getInstance()->getBool("OptimizeVRAM"));
	s->addWithLabel(_("OPTIMIZE IMAGES VRAM USE"), optimizeVram);
	s->addSaveFunc([optimizeVram]
	{
		TextureData::OPTIMIZEVRAM = optimizeVram->getState();
		Settings::getInstance()->setBool("OptimizeVRAM", optimizeVram->getState());
	});

	// preload Medias
	auto preloadMedias = std::make_shared<SwitchComponent>(window, Settings::getInstance()->getBool("PreloadMedias"));
	s->addWithDescription(_("PRELOAD METADATA MEDIA ON BOOT"), _("Reduces lag when scrolling through a fully scraped gamelist, increases boot time."), preloadMedias);
	s->addSaveFunc([preloadMedias] { Settings::getInstance()->setBool("PreloadMedias", preloadMedias->getState()); });

	// threaded loading
	auto threadedLoading = std::make_shared<SwitchComponent>(window, Settings::getInstance()->getBool("ThreadedLoading"));
	s->addWithLabel(_("THREADED LOADING"), threadedLoading);
	s->addSaveFunc([threadedLoading] { Settings::getInstance()->setBool("ThreadedLoading", threadedLoading->getState()); });

	if (ApiSystem::getInstance()->isScriptingSupported(ApiSystem::ScriptId::OPTMIZE_SYSTEM))
	{
		// optimize system
		bool optimize_system_old_value = Settings::getInstance()->getBool("OptimizeSystem");
		auto optimize_system = std::make_shared<SwitchComponent>(window, optimize_system_old_value);
		s->addWithLabel(_("OPTIMIZE SYSTEM"), optimize_system);
		s->addSaveFunc([window, optimize_system, optimize_system_old_value]
			{
				if (optimize_system_old_value != optimize_system->getState())
				{
					window->pushGui(new GuiMsgBox(window, _("IT IS NOT RECOMMENDED TO ENABLE THIS OPTION IF YOU ARE DEVELOPING OR TESTING.") + " " + _("THE PROCESS MAY DURE SOME SECONDS.\nPLEASE WAIT."),
						_("OK"),
							[window, optimize_system] {
							LOG(LogInfo) << "GuiMenu::openAdvancedSettings() - optimize system: " << Utils::String::boolToString( optimize_system->getState() );
							Settings::getInstance()->setBool("OptimizeSystem", optimize_system->getState());
							Settings::getInstance()->saveFile();
							ApiSystem::getInstance()->setOptimizeSystem( optimize_system->getState() );

							window->pushGui(new GuiMsgBox(window, _("THE SYSTEM WILL NOW REBOOT"),
										_("OK"),
											[] {
											if (quitES(QuitMode::REBOOT) != 0)
												LOG(LogWarning) << "GuiMenu::openAdvancedSettings() - Restart terminated with non-zero result!";
										}));
					}));
				}
			});
	}

	if (ApiSystem::getInstance()->isScriptingSupported(ApiSystem::ScriptId::OVERCLOCK))
	{
		// overclock
		bool overclock_system_old_value = ApiSystem::getInstance()->isOverclockSystem();
		auto overclock_system = std::make_shared<SwitchComponent>(window, overclock_system_old_value);
		s->addWithLabel(_("OVERCLOCK SYSTEM"), overclock_system);
		s->addSaveFunc([window, overclock_system, overclock_system_old_value]
			{
				if (overclock_system_old_value != overclock_system->getState())
				{
					LOG(LogInfo) << "GuiMenu::openAdvancedSettings() - overclock system: " << Utils::String::boolToString( overclock_system->getState() );
					if ( ApiSystem::getInstance()->setOverclockSystem( overclock_system->getState() ) )
					{
						window->pushGui(new GuiMsgBox(window, _("THE SYSTEM WILL NOW REBOOT"),
									_("OK"),
										[] {
										if (quitES(QuitMode::REBOOT) != 0)
											LOG(LogWarning) << "GuiMenu::openAdvancedSettings() - Restart terminated with non-zero result!";
									}));
					}
					else
					{
						window->pushGui(new GuiMsgBox(window, _("OVERCLOCK CONFIGURATION ERROR"), _("OK"), nullptr));
					}
				}
			});
	}

	if (ApiSystem::getInstance()->isScriptingSupported(ApiSystem::ScriptId::PRELOAD_VLC))
	{
		// preload VLC - workaround for the freeze of the first video play
		auto preloa_VLC = std::make_shared<SwitchComponent>(window, Settings::getInstance()->getBool("PreloadVLC"));
		s->addWithLabel(_("PRELOAD VLC"), preloa_VLC);
		s->addSaveFunc([preloa_VLC]
			{
				Settings::getInstance()->setBool("PreloadVLC", preloa_VLC->getState());
			});
	}


	s->addGroup(_("USER INTERFACE"));

	// framerate
	auto framerate = std::make_shared<SwitchComponent>(window, Settings::getInstance()->getBool("DrawFramerate"));
	s->addWithLabel(_("SHOW FRAMERATE"), framerate);
	s->addSaveFunc([framerate] { Settings::getInstance()->setBool("DrawFramerate", framerate->getState()); });

	if (ApiSystem::getInstance()->isScriptingSupported(ApiSystem::ScriptId::SHOW_FPS))
	{
	// retroarch framerate
		bool retroarch_fps_value = ApiSystem::getInstance()->isShowRetroarchFps();
		auto retroarch_fps = std::make_shared<SwitchComponent>(window, retroarch_fps_value);
		s->addWithLabel(_("SHOW RETROARCH FPS"), retroarch_fps);
		s->addSaveFunc([retroarch_fps, retroarch_fps_value]
			{
				bool retroarch_fps_new_value = retroarch_fps->getState();
				if (retroarch_fps_value != retroarch_fps_new_value)
				{
					ApiSystem::getInstance()->setShowRetroarchFps(retroarch_fps_new_value);
				}
			});
	}

	// show detailed system information
	auto detailedSystemInfo = std::make_shared<SwitchComponent>(window, Settings::getInstance()->getBool("ShowDetailedSystemInfo"));
	s->addWithLabel(_("SHOW DETAILED SYSTEM INFO"), detailedSystemInfo);
	s->addSaveFunc([s, detailedSystemInfo]{
		bool old_value = Settings::getInstance()->getBool("ShowDetailedSystemInfo");
		if (old_value != detailedSystemInfo->getState())
		{
			Settings::getInstance()->setBool("ShowDetailedSystemInfo", detailedSystemInfo->getState());
			s->setVariable("reloadGuiMenu", true);
		}
	});

	// show file browser
	auto show_file_browser = std::make_shared<SwitchComponent>(window, Settings::getInstance()->getBool("ShowFileBrowser"));
	s->addWithLabel(_("SHOW FILE BROWSER"), show_file_browser);
	s->addSaveFunc([show_file_browser] { Settings::getInstance()->setBool("ShowFileBrowser", show_file_browser->getState()); });


	// CONTROLLER ACTIVITY
/*
	auto activity = std::make_shared<SwitchComponent>(window, Settings::getInstance()->getBool("ShowControllerActivity"));
	s->addWithLabel(_("SHOW CONTROLLER ACTIVITY"), activity);
	s->addSaveFunc([activity]
	{
		bool old_value = Settings::getInstance()->getBool("ShowControllerActivity");
		if (old_value != activity->getState())
		{
			Settings::getInstance()->setBool("ShowControllerActivity", activity->getState());
		}
	});
*/
	// Battery Indicator
	auto battery = std::make_shared<SwitchComponent>(window, Settings::getInstance()->getBool("ShowBatteryIndicator"));
	s->addWithLabel(_("SHOW BATTERY LEVEL"), battery);
	s->addSaveFunc([s, battery]
	{
		Settings::getInstance()->setBool("ShowBatteryIndicator", battery->getState());
		//s->setVariable("reloadAll", true);
	});

	// Close with select the game metadata edit gui
	auto gui_metadata_select = std::make_shared<SwitchComponent>(window, Settings::getInstance()->getBool("GuiEditMetadataCloseAllWindows"));
	s->addWithLabel(_("GAME METADATA EDIT - \"SELECT\" CLOSE MENU"), gui_metadata_select);
	s->addSaveFunc([s, gui_metadata_select]
	{
		Settings::getInstance()->setBool("GuiEditMetadataCloseAllWindows", gui_metadata_select->getState());
	});

	s->addGroup(_("OTHERS"));

	s->addEntry(_("\"QUIT\" SETTINGS"), true, [this, s] { openQuitSettings(s); });

	if (ApiSystem::getInstance()->isScriptingSupported(ApiSystem::ScriptId::AUTO_SUSPEND))
		s->addEntry(_("DEVICE AUTO SUSPEND SETTINGS"), true, [this] { openAutoSuspendSettings(); });

	if (ApiSystem::getInstance()->isScriptingSupported(ApiSystem::ScriptId::SYSTEM_HOTKEY_EVENTS))
		s->addEntry(_("SYSTEM HOTKEY EVENTS SETTINGS"), true, [this] { openSystemHotkeyEventsSettings(); });

	if (ApiSystem::getInstance()->isScriptingSupported(ApiSystem::ScriptId::RETROACHIVEMENTS))
		s->addEntry(_("RETROACHIEVEMENTS SETTINGS"), true, [this] { openRetroAchievementsSettings(); });

	s->addGroup(_("LOG INFO"));
	// log level
	auto logLevel = std::make_shared< OptionListComponent<std::string> >(window, _("LOG LEVEL"), false);
	std::vector<std::string> levels;
	levels.push_back("default");
	levels.push_back("disabled");
	levels.push_back("warning");
	levels.push_back("error");
	levels.push_back("debug");

	auto level = Settings::getInstance()->getString("LogLevel");
	if (level.empty())
		level = "default";

	for (auto it = levels.cbegin(); it != levels.cend(); it++)
		logLevel->add(_(it->c_str()), *it, level == *it);

	s->addWithLabel(_("LOG LEVEL"), logLevel);
	s->addSaveFunc([this, logLevel]
	{
		if (Settings::getInstance()->setString("LogLevel", logLevel->getSelected() == "default" ? "" : logLevel->getSelected()))
		{
			Log::setupReportingLevel();
			Log::init();
		}
	});

	auto logWithMilliseconds = std::make_shared<SwitchComponent>(window, Settings::getInstance()->getBool("LogWithMilliseconds"));
	s->addWithLabel(_("LOG WITH MILLISECONDS"), logWithMilliseconds);
	s->addSaveFunc([logWithMilliseconds] {
		if (Settings::getInstance()->setBool("LogWithMilliseconds", logWithMilliseconds->getState()))
		{
			Log::setupReportingLevel();
			Log::init();
		}
	});

	if (ApiSystem::getInstance()->isScriptingSupported(ApiSystem::ScriptId::LOG_SCRIPTS))
	{
		auto scripts_log_activated = std::make_shared<SwitchComponent>(window, ApiSystem::getInstance()->isEsScriptsLoggingActivated());
		s->addWithLabel(_("ACTIVATE ES SCRIPTS LOGGING"), scripts_log_activated);
		s->addSaveFunc([scripts_log_activated] {
			ApiSystem::getInstance()->setEsScriptsLoggingActivated(scripts_log_activated->getState(), Settings::getInstance()->getString("LogLevel"),
																   Settings::getInstance()->getBool("LogWithMilliseconds"));
		});
	}

	auto pthis = this;

	s->onFinalize([s, pthis, window]
	{
		if (s->getVariable("reloadGuiMenu"))
		{
			if (pthis)
				delete pthis;
			window->pushGui(new GuiMenu(window, false));
		}
	});

	window->pushGui(s);

}

void GuiMenu::openQuitSettings(GuiSettings *parentGui)
{
	Window* window = mWindow;
	auto pthis = this;

	auto s = new GuiQuitOptions(window);

	s->onFinalize([s, parentGui, pthis, window]
	{
		if (s->getVariable("reloadGuiMenu"))
		{
			if (parentGui)
				delete parentGui;
			if (pthis)
				delete pthis;
			
			auto main_menu = new GuiMenu(window, false);
			window->pushGui(main_menu);
			main_menu->openAdvancedSettings();
		}
	});

	window->pushGui(s);
}

void GuiMenu::openAutoSuspendSettings()
{
	Window *window = mWindow;
	window->pushGui(new GuiAutoSuspendOptions(window));
}

void GuiMenu::openSystemHotkeyEventsSettings()
{
	Window *window = mWindow;
	window->pushGui(new GuiSystemHotkeyEventsOptions(window));
}

void GuiMenu::openRetroAchievementsSettings()
{
	Window *window = mWindow;
	window->pushGui(new GuiRetroachievementsOptions(window));
}

void GuiMenu::openMenusSettings(GuiSettings *parentGui)
{
	Window* window = mWindow;
	auto pthis = this;
	auto s = new GuiMenusOptions(mWindow);

	s->onFinalize([s, parentGui, pthis, window]
	{
		if (s->getVariable("reloadGuiMenu"))
		{
			if (parentGui)
				delete parentGui;
			if (pthis)
				delete pthis;
			
			auto main_menu = new GuiMenu(window, false);
			window->pushGui(main_menu);
			main_menu->openUISettings();
		}

	});

	mWindow->pushGui(s);
}

void GuiMenu::openSystemInformation()
{
	Window *window = mWindow;
	window->pushGui(new GuiSystemInformation(window));
}

void GuiMenu::openConfigInput()
{
	Window* window = mWindow;
		
	window->pushGui(new GuiMsgBox(window, _("ARE YOU SURE YOU WANT TO CONFIGURE INPUT?"), _("YES"),
		[window] {
			window->pushGui(new GuiDetectDevice(window, false, nullptr));
		}, _("NO"), nullptr)
	);

}

void GuiMenu::openQuitMenu()
{
	Window *window = mWindow;
	GuiMenu::openQuitMenu_static(window);
}

void GuiMenu::openQuitMenu_static(Window *window, bool quickAccessMenu, bool animate)
{
	static std::function<void()> quitEsFunction = []
		{
			if(quitES() != 0)
				LOG(LogWarning) << "GuiMenu::openQuitMenu_static() - Quit ES terminated with non-zero result!";
		};

	static std::function<void()> restartEsFunction = []
		{
			if(quitES(QuitMode::RESTART) != 0)
				LOG(LogWarning) << "GuiMenu::openQuitMenu_static() - Restart ES terminated with non-zero result!";
		};

	static std::function<void()> restartDeviceFunction = []
		{
			if (quitES(QuitMode::REBOOT) != 0)
				LOG(LogWarning) << "GuiMenu::openQuitMenu_static() - Restart System terminated with non-zero result!";
		};

	static std::function<void()> fastRestartDeviceFunction = []
		{
			if (quitES(QuitMode::FAST_REBOOT) != 0)
				LOG(LogWarning) << "GuiMenu::openQuitMenu_static() - Fast Restart System terminated with non-zero result!";
		};

	static std::function<void()> suspendDeviceFunction = []
		{
			if (quitES(QuitMode::SUSPEND) != 0)
				LOG(LogWarning) << "GuiMenu::openQuitMenu_static() - Suspend System terminated with non-zero result!";
		};

	static std::function<void()> shutdownDeviceFunction = []
		{
			if (quitES(QuitMode::SHUTDOWN) != 0)
				LOG(LogWarning) << "GuiMenu::openQuitMenu_static() - Shutdown System terminated with non-zero result!";
		};

	static std::function<void()> fastShutdownDeviceFunction = []
		{
			if (quitES(QuitMode::FAST_SHUTDOWN) != 0)
				LOG(LogWarning) << "GuiMenu::openQuitMenu_static() - Fast Shutdown System terminated with non-zero result!";
		};

	if (Settings::getInstance()->getBool("ShowOnlyExit") && !quickAccessMenu)
	{
		std::string exit_action = Settings::getInstance()->getString("OnlyExitAction");
		std::string exit_label = "REALLY SHUTDOWN?";
		std::function<void()> exitFunction = shutdownDeviceFunction;

		if (Utils::String::equalsIgnoreCase(exit_action, "suspend"))
		{
			exitFunction = suspendDeviceFunction;
			exit_label = "REALLY SUSPEND?";
		}
		else if (Utils::String::equalsIgnoreCase(exit_action, "exit_es"))
		{
			exitFunction = quitEsFunction;
			exit_label = "REALLY QUIT?";
		}

		if (Settings::getInstance()->getBool("ConfirmToExit"))
			window->pushGui(new GuiMsgBox(window, _(exit_label), _("YES"), exitFunction, _("NO"), nullptr));
		 else
			exitFunction();

		return;
	}

	auto s = new GuiSettings(window, (quickAccessMenu ? _("QUICK ACCESS") : _("QUIT")));
	s->setCloseButton("select");

	if (quickAccessMenu)
	{
		s->addGroup(_("QUICK ACCESS"));

		// Don't like one of the songs? Press next
		if (AudioManager::getInstance()->isSongPlaying())
		{
			auto sname = AudioManager::getInstance()->getSongName();
			if (!sname.empty())
			{
				s->addWithDescription(_("SKIP TO THE NEXT SONG"), _("NOW PLAYING") + ": " + sname, nullptr, [s, window]
					{
						Window* w = window;
						AudioManager::getInstance()->playRandomMusic(false);
						delete s;
						openQuitMenu_static(w, true, false);
					}, "iconSound");
			}
		}

		s->addEntry(_("LAUNCH SCREENSAVER"), false, [s, window]
			{
				Window* w = window;
				window->postToUiThread([w]()
				{
					w->startScreenSaver();
					w->renderScreenSaver(true);
				});
				delete s;
			}, "iconScraper", true);
	}

	if (quickAccessMenu)
		s->addGroup(_("QUIT"));

	bool isUIModeFull = UIModeController::getInstance()->isUIModeFull();
	if (isUIModeFull)
	{
		s->addEntry(_("RESTART EMULATIONSTATION"), false, [window]
			{
				if (Settings::getInstance()->getBool("ConfirmToExit"))
					window->pushGui(new GuiMsgBox(window, _("REALLY RESTART?"),
													_("YES"), restartEsFunction,
													_("NO"), nullptr));
				else
					restartEsFunction();
			}, "iconRestartEmulationstaion");

		if(Settings::getInstance()->getBool("ShowExit"))
		{
			s->addEntry(_("QUIT EMULATIONSTATION"), false, [window]
				{
					if (Settings::getInstance()->getBool("ConfirmToExit"))
						window->pushGui(new GuiMsgBox(window, _("REALLY QUIT?"),
													_("YES"), quitEsFunction,
													_("NO"), nullptr));
					else
						quitEsFunction();
				}, "iconQuit");
		}
	}

	if (isUIModeFull)
	{
		s->addEntry(_("SUSPEND SYSTEM"), false, [window]
			{
				if (Settings::getInstance()->getBool("ConfirmToExit"))
					window->pushGui(new GuiMsgBox(window, _("REALLY SUSPEND?"),
													_("YES"), suspendDeviceFunction,
													_("NO"), nullptr));
				else
					suspendDeviceFunction();
			}, "iconSuspend");
	}

	s->addEntry(_("RESTART SYSTEM"), false, [window]
		{
			if (Settings::getInstance()->getBool("ConfirmToExit"))
				window->pushGui(new GuiMsgBox(window, _("REALLY RESTART?"),
												_("YES"), restartDeviceFunction,
												_("NO"), nullptr));
			else
				restartDeviceFunction();
		}, "iconRestart");

	if (isUIModeFull && Settings::getInstance()->getBool("ShowFastQuitActions"))
	{
		s->addWithDescription(_("FAST RESTART SYSTEM"), _("Restart without saving metadata."), nullptr, [window]
			{
				if (Settings::getInstance()->getBool("ConfirmToExit"))
					window->pushGui(new GuiMsgBox(window, _("REALLY RESTART WITHOUT SAVING METADATA?"),
													_("YES"), fastRestartDeviceFunction,
													_("NO"), nullptr));
				else
					fastRestartDeviceFunction();
			}, "iconFastRestart");
	}

	s->addEntry(_("SHUTDOWN SYSTEM"), false, [window]
		{
			if (Settings::getInstance()->getBool("ConfirmToExit"))
				window->pushGui(new GuiMsgBox(window, _("REALLY SHUTDOWN?"),
												_("YES"), shutdownDeviceFunction,
												_("NO"), nullptr));
			else
				shutdownDeviceFunction();
		}, "iconShutdown");

	if (isUIModeFull && Settings::getInstance()->getBool("ShowFastQuitActions"))
	{
		s->addWithDescription(_("FAST SHUTDOWN SYSTEM"), _("Shutdown without saving metadata."), nullptr, [window]
			{
				if (Settings::getInstance()->getBool("ConfirmToExit"))
					window->pushGui(new GuiMsgBox(window, _("REALLY SHUTDOWN WITHOUT SAVING METADATA?"),
													_("YES"), fastShutdownDeviceFunction,
													_("NO"), nullptr));
				else
					fastShutdownDeviceFunction();
			}, "iconFastShutdown");
	}

	if (quickAccessMenu && animate)
		s->getMenu().animateTo(Vector2f((Renderer::getScreenWidth() - s->getMenu().getSize().x()) / 2, (Renderer::getScreenHeight() - s->getMenu().getSize().y()) / 2));
	else if (quickAccessMenu)
		s->getMenu().setPosition((Renderer::getScreenWidth() - s->getMenu().getSize().x()) / 2, (Renderer::getScreenHeight() - s->getMenu().getSize().y()) / 2);

	window->pushGui(s);
}

std::string getBuildTime()
{
	std::string datestr = __DATE__;
	std::string timestr = __TIME__;

	std::istringstream iss_date(datestr);
	std::string str_month;
	int day;
	int year;
	iss_date >> str_month >> day >> year;

	int month;
	if (str_month == "Jan") month = 1;
	else if (str_month == "Feb") month = 2;
	else if (str_month == "Mar") month = 3;
	else if (str_month == "Apr") month = 4;
	else if (str_month == "May") month = 5;
	else if (str_month == "Jun") month = 6;
	else if (str_month == "Jul") month = 7;
	else if (str_month == "Aug") month = 8;
	else if (str_month == "Sep") month = 9;
	else if (str_month == "Oct") month = 10;
	else if (str_month == "Nov") month = 11;
	else if (str_month == "Dec") month = 12;
	else exit(-1);

	for (std::string::size_type pos = timestr.find(':'); pos != std::string::npos; pos = timestr.find(':', pos))
		timestr[pos] = ' ';

	std::istringstream iss_time(timestr);
	int hour, min, sec;
	iss_time >> hour >> min >> sec;

	char buffer[100];
	
	sprintf(buffer, "%4d%.2d%.2d%.2d%.2d%.2d\n", year, month, day, hour, min, sec);

	return buffer;
}

void GuiMenu::addVersionInfo()
{
	SoftwareInformation software = ApiSystem::getInstance()->getSoftwareInformation();
	addEntry(_U("\uF02B  Distro Version: ") + software.application_name + " " + software.version, false, [this] {  });

	if (Settings::getInstance()->getBool("ShowHelpPrompts"))
	{
		mVersion.setVisible(false);
		return;
	}

	std::string  buildDate = getBuildTime();
	auto theme = ThemeData::getMenuTheme();

	mVersion.setFont(theme->Footer.font);
	mVersion.setColor(theme->Footer.color);
	mVersion.setLineSpacing(0);
	
	mVersion.setText("EMULATIONSTATION V" + Utils::String::toUpper(software.es_version) + " BUILD " + buildDate);

	mVersion.setHorizontalAlignment(ALIGN_CENTER);	
	mVersion.setVerticalAlignment(ALIGN_CENTER);
	mVersion.setAutoScroll(true);
	mVersion.setVisible(true);

	addChild(&mVersion);
}

void GuiMenu::openScreensaverOptions() {
	Window *window = mWindow;
	window->pushGui(new GuiGeneralScreensaverOptions(window, _("SCREENSAVER SETTINGS")));
}

void GuiMenu::openCollectionSystemSettings(bool cursor)
{
	Window *window = mWindow;
	if (ThreadedScraper::isRunning())
	{
		window->pushGui(new GuiMsgBox(window, _("THIS FUNCTION IS DISABLED WHEN SCRAPING IS RUNNING")));
		return;
	}

	window->pushGui(new GuiCollectionSystemsOptions(window));
/*
	auto s = new GuiCollectionSystemsOptions(window, cursor);
	auto pthis = this;

	s->onFinalize([s, pthis, window]
	{
		if (s->getVariable("reloadMenu"))
		{
			Settings::getInstance()->saveFile();
			delete s;
			pthis->openCollectionSystemSettings(true);
		}

	});

	window->pushGui(s);
*/
}

void GuiMenu::onSizeChanged()
{
	if (Settings::getInstance()->getBool("ShowHelpPrompts"))
	{
		mVersion.setVisible(false);
		return;
	}

	float h = mMenu.getButtonGridHeight();

	mVersion.setSize(mSize.x(), h);
	mVersion.setPosition(0, mSize.y() - h); //  mVersion.getSize().y()
	mVersion.setVisible(true);
}

void GuiMenu::addEntry(std::string name, bool add_arrow, const std::function<void()>& func, const std::string iconName)
{
	Window *window = mWindow;
	auto theme = ThemeData::getMenuTheme();
	std::shared_ptr<Font> font = theme->Text.font;
	unsigned int color = theme->Text.color;

	// populate the list
	ComponentListRow row;

	if (!iconName.empty())
	{
		std::string iconPath = theme->getMenuIcon(iconName);
		if (!iconPath.empty())
		{
			// icon
			auto icon = std::make_shared<ImageComponent>(window);
			icon->setImage(iconPath);
			icon->setColorShift(color);
			icon->setResize(0, font->getLetterHeight() * 1.25f);
			row.addElement(icon, false);

			// spacer between icon and text
			auto spacer = std::make_shared<GuiComponent>(window);
			spacer->setSize(10, 0);
			row.addElement(spacer, false);
		}
	}

	auto text_comp = std::make_shared<TextComponent>(window, name, font, color);
	text_comp->setAutoScroll(Settings::getInstance()->getBool("AutoscrollMenuEntries"));
	row.addElement(text_comp, true);

	if (add_arrow)
	{
		std::shared_ptr<ImageComponent> bracket = makeArrow(window);
		row.addElement(bracket, false);
	}

	row.makeAcceptInputHandler(func);
	mMenu.addRow(row);
}

bool GuiMenu::input(InputConfig* config, Input input)
{
	if (GuiComponent::input(config, input))
		return true;

	if ((config->isMappedTo(BUTTON_BACK, input) || config->isMappedTo("start", input)) && input.value != 0)
	{
		auto pthis = this;
		if (pthis)
			delete pthis;
		return true;
	}

	return false;
}

HelpStyle GuiMenu::getHelpStyle()
{
	HelpStyle style = HelpStyle();

	if (ThemeData::getDefaultTheme() != nullptr)
	{
		std::shared_ptr<ThemeData> theme = std::shared_ptr<ThemeData>(ThemeData::getDefaultTheme(), [](ThemeData*) {});
		style.applyTheme(theme, "system");
	}
	else 
		style.applyTheme(ViewController::get()->getState().getSystem()->getTheme(), "system");

	return style;
}

std::vector<HelpPrompt> GuiMenu::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts;
	prompts.push_back(HelpPrompt("up/down", _("CHOOSE")));
	prompts.push_back(HelpPrompt(BUTTON_OK, _("SELECT")));
	prompts.push_back(HelpPrompt("start", _("CLOSE")));
	return prompts;
}

std::string GuiMenu::formatNetworkStatus(bool isConnected)
{
	return (isConnected ? "    " + _("CONNECTED") + " " : _("NOT CONNECTED"));
}

std::string GuiMenu::getIconBattery(int level, bool isCharging)
{
	if (isCharging)
		return ResourceManager::getInstance()->getResourcePath(":/battery/incharge.svg");
	else if (level > 75)
		return ResourceManager::getInstance()->getResourcePath(":/battery/full.svg");
	else if (level > 50)
		return ResourceManager::getInstance()->getResourcePath(":/battery/75.svg");
	else if (level > 25)
		return ResourceManager::getInstance()->getResourcePath(":/battery/50.svg");
	else if (level > 5)
		return ResourceManager::getInstance()->getResourcePath(":/battery/25.svg");
	else
		return ResourceManager::getInstance()->getResourcePath(":/battery/empty.svg");
}

std::string GuiMenu::getIconSound(int level)
{
	if (level > 70)
		return ResourceManager::getInstance()->getResourcePath(":/sound/sound_max.svg");
	else if (level > 20)
		return ResourceManager::getInstance()->getResourcePath(":/sound/sound_middle.svg");
	else if (level > 0)
		return ResourceManager::getInstance()->getResourcePath(":/sound/sound_min.svg");
	else
		return ResourceManager::getInstance()->getResourcePath(":/sound/sound_mute.svg");
}

std::string GuiMenu::getIconBrightness(int level)
{
	if (level > 89)
		return ResourceManager::getInstance()->getResourcePath(":/brightness/brightness_max.svg");
	else if (level > 74) // 78%
		return ResourceManager::getInstance()->getResourcePath(":/brightness/brightness_84.svg");
	else if (level > 60) // 65%
		return ResourceManager::getInstance()->getResourcePath(":/brightness/brightness_70.svg");
	else if (level > 45) // 50%
		return ResourceManager::getInstance()->getResourcePath(":/brightness/brightness_56.svg");
	else if (level > 35) // 38%
		return ResourceManager::getInstance()->getResourcePath(":/brightness/brightness_42.svg");
	else if (level > 22) // 25%
		return ResourceManager::getInstance()->getResourcePath(":/brightness/brightness_28.svg");
	else if (level > 10) // 13%
		return ResourceManager::getInstance()->getResourcePath(":/brightness/brightness_14.svg");
	else // <= 10%
		return ResourceManager::getInstance()->getResourcePath(":/brightness/brightness_min.svg");
}

std::string GuiMenu::getIconNetwork(bool status)
{
	if (status)
		return ResourceManager::getInstance()->getResourcePath(":/network/network.svg");
	else
		return ResourceManager::getInstance()->getResourcePath(":/network/network_disc.svg");
}

std::string GuiMenu::getIconBluetooth(bool status)
{
	if (status)
		return ResourceManager::getInstance()->getResourcePath(":/network/bluetooth_on.svg");
	else
		return ResourceManager::getInstance()->getResourcePath(":/network/bluetooth_off.svg");
}

void GuiMenu::addStatusBarInfo(Window* window)
{
	BatteryInformation battery = ApiSystem::getInstance()->getBatteryInformation();

	auto theme = ThemeData::getMenuTheme();
	std::shared_ptr<Font> font = theme->Text.font;
	unsigned int color = theme->Text.color;

	// populate the list
	ComponentListRow row;

	// spacer between icon and text
	auto spacer = std::make_shared<GuiComponent>(window);
	spacer->setSize(10, 0);

	// Battery Information
	std::string iconPath = getIconBattery(battery.level, battery.isCharging);
	int level = battery.level;
	std::string text;

	if (!iconPath.empty())
	{
		// icon
		auto icon = std::make_shared<ImageComponent>(window);
		icon->setImage(iconPath);
		icon->setColorShift(theme->Text.color);
		icon->setResize(0, theme->Text.font->getLetterHeight() * 1.50f);
		row.addElement(icon, false, false);
		row.addElement(spacer, false);
	}
	else
		text.append("BAT: ");

	row.addElement(std::make_shared<TextComponent>(window, text.append(std::to_string( level )).append("%  | "), font, color), false);

	// Sound Information
	level = ApiSystem::getInstance()->getVolume();
	iconPath = getIconSound(level);
	text.clear();
	if (!iconPath.empty())
	{
		// icon
		auto icon = std::make_shared<ImageComponent>(window);
		icon->setImage(iconPath);
		icon->setColorShift(theme->Text.color);
		icon->setResize(0, theme->Text.font->getLetterHeight() * 1.50f);
		row.addElement(icon, false, false);
		row.addElement(spacer, false);
	}
	else
		text.append("SND: ");

	row.addElement(std::make_shared<TextComponent>(window, text.append(std::to_string( level )).append("%  | "), font, color), false);

	// Brightness Information
	level = ApiSystem::getInstance()->getBrightnessLevel();
	iconPath = getIconBrightness(level);
	text.clear();
	if (!iconPath.empty())
	{
		// icon
		auto icon = std::make_shared<ImageComponent>(window);
		icon->setImage(iconPath);
		icon->setColorShift(theme->Text.color);
		icon->setResize(0, theme->Text.font->getLetterHeight() * 1.50f);
		row.addElement(icon, false, false);
		row.addElement(spacer, false);
	}
	else
		text.append("BRT: ");

	row.addElement(std::make_shared<TextComponent>(window, text.append(std::to_string( level )).append("%  | "), font, color), false);

	// Bluetooth Information
	bool status = ApiSystem::getInstance()->isBluetoothEnabled();
	iconPath = getIconBluetooth(status);
	text.clear();
	if (!iconPath.empty())
	{
		// icon
		auto icon = std::make_shared<ImageComponent>(window);
		icon->setImage(iconPath);
		icon->setColorShift(theme->Text.color);
		icon->setResize(0, theme->Text.font->getLetterHeight() * 1.50f);
		row.addElement(icon, false, false);
		row.addElement(spacer, false);
	}
	else
		text.append("BT: ").append(( status ? "ON" : "OFF" ));

	row.addElement(std::make_shared<TextComponent>(window, text.append(" | "), font, color), false);

	// Network Information
	iconPath = ResourceManager::getInstance()->getResourcePath(":/network/network_icon.svg");
	text.clear();
	if (!iconPath.empty())
	{
		// icon
		auto icon = std::make_shared<ImageComponent>(window);
		icon->setImage(iconPath);
		icon->setColorShift(theme->Text.color);
		icon->setResize(0, theme->Text.font->getLetterHeight() * 1.50f);
		row.addElement(icon, false, false);
		row.addElement(spacer, false);
	}
	else
		row.addElement(std::make_shared<TextComponent>(window, text.append("NTW: "), font, color), false);

	status = ApiSystem::getInstance()->isNetworkConnected();
	iconPath = getIconNetwork(status);
	text.clear();
	if (!iconPath.empty())
	{
		// icon
		auto icon = std::make_shared<ImageComponent>(window);
		icon->setImage(iconPath);
		icon->setColorShift(theme->Text.color);
		icon->setResize(0, theme->Text.font->getLetterHeight() * 1.50f);
		row.addElement(icon, false, false);		
		row.addElement(spacer, false);
	}
	else
		row.addElement(std::make_shared<TextComponent>(window, text.append(formatNetworkStatus(status)), font, color), false);

	mMenu.addRow(row);
}
