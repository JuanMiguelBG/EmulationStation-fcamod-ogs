#include <SystemData.h>
#include "guis/GuiCollectionSystemsOptions.h"

#include "components/OptionListComponent.h"
#include "components/SwitchComponent.h"
#include "guis/GuiSettings.h"
#include "guis/GuiTextEditPopupKeyboard.h"
#include "guis/GuiTextEditPopup.h"
#include "guis/GuiMenu.h"
#include "utils/StringUtil.h"
#include "views/ViewController.h"
#include "CollectionSystemManager.h"
#include "Window.h"

GuiCollectionSystemsOptions::GuiCollectionSystemsOptions(Window* window)
	: GuiSettings(window, _("GAME COLLECTION SETTINGS").c_str())
{
	initializeMenu();
}

void GuiCollectionSystemsOptions::initializeMenu()
{
	addGroup(_("COLLECTIONS TO DISPLAY"));

	// Select systems to hide
	auto hiddenSystems = Utils::String::split(Settings::getInstance()->getString("HiddenSystems"), ';');
	auto displayedSystems = std::make_shared<OptionListComponent<SystemData*>>(mWindow, _("SYSTEMS DISPLAYED"), true);

	for (auto system : SystemData::sSystemVector)
		if(!system->isCollection() && !system->isGroupChildSystem())
			displayedSystems->add(system->getFullName(), system, std::find(hiddenSystems.cbegin(), hiddenSystems.cend(), system->getName()) == hiddenSystems.cend());

	addWithLabel(_("SYSTEMS DISPLAYED"), displayedSystems);
	addSaveFunc([this, displayedSystems]
	{
		std::string hiddenSystems;

		std::vector<SystemData*> sys = displayedSystems->getSelectedObjects();

		for (auto system : SystemData::sSystemVector)
		{
			if (system->isCollection() || system->isGroupChildSystem())
				continue;

			if (std::find(sys.cbegin(), sys.cend(), system) == sys.cend())
			{
				if (hiddenSystems.empty())
					hiddenSystems = system->getName();
				else
					hiddenSystems = hiddenSystems + ";" + system->getName();
			}
		}

		if (hiddenSystems != Settings::getInstance()->getString("HiddenSystems"))
		{
			LOG(LogDebug) << "GuiMenu::openUISettings() - hiddenSystems changed, new value: " << hiddenSystems;
			Settings::getInstance()->setString("HiddenSystems", hiddenSystems);
			Settings::getInstance()->saveFile();
			setVariable("reloadAll", true);
			setVariable("forceReloadGames", true);
		}
	});

	// get collections
	addSystemsToMenu();

	auto groupNames = SystemData::getAllGroupNames();
	if (groupNames.size() > 0)
	{
		auto ungroupedSystems = std::make_shared<OptionListComponent<std::string>>(mWindow, _("GROUPED SYSTEMS"), true);
		for (auto groupName : groupNames)
		{
			std::string description;
			for (auto zz : SystemData::getGroupChildSystemNames(groupName))
			{
				if (!description.empty())
					description += ", ";

				description += zz;
			}

			ungroupedSystems->addEx(groupName, description, groupName, !Settings::getInstance()->getBool(groupName + ".ungroup"));
		}

		addWithLabel(_("GROUPED SYSTEMS"), ungroupedSystems);
		addSaveFunc([this, ungroupedSystems, groupNames]
		{
			std::vector<std::string> checkedItems = ungroupedSystems->getSelectedObjects();
			for (auto groupName : groupNames)
			{
				bool isGroupActive = std::find(checkedItems.cbegin(), checkedItems.cend(), groupName) != checkedItems.cend();
				if (Settings::getInstance()->setBool(groupName + ".ungroup", !isGroupActive))
					setVariable("reloadSystems", true);
			}
		});
	}

	addGroup(_("CREATE CUSTOM COLLECTION"));

	// add "Create New Custom Collection from Theme"
	std::vector<std::string> unusedFolders = CollectionSystemManager::get()->getUnusedSystemsFromTheme();
	if (unusedFolders.size() > 0)
	{
		addEntry(_("CREATE NEW CUSTOM COLLECTION FROM THEME").c_str(), true,
			[this, unusedFolders] {
			auto s = new GuiSettings(mWindow, _("SELECT THEME FOLDER").c_str());
			std::shared_ptr< OptionListComponent<std::string> > folderThemes = std::make_shared< OptionListComponent<std::string> >(mWindow, _("SELECT THEME FOLDER"), true);

			// add Custom Systems
			for (auto it = unusedFolders.cbegin(); it != unusedFolders.cend(); it++)
			{
				ComponentListRow row;
				std::string name = *it;

				std::function<void()> createCollectionCall = [name, this, s] {
					createCollection(name);
				};
				row.makeAcceptInputHandler(createCollectionCall);

				auto themeFolder = std::make_shared<TextComponent>(mWindow, Utils::String::toUpper(name), ThemeData::getMenuTheme()->Text.font, ThemeData::getMenuTheme()->Text.color);
				row.addElement(themeFolder, true);
				s->addRow(row);
			}
			mWindow->pushGui(s);
		});
	}

	auto createCustomCollection = [this](const std::string& newVal) {
		std::string name = newVal;
		// we need to store the first Gui and remove it, as it'll be deleted by the actual Gui
		Window* window = mWindow;
		GuiComponent* topGui = window->peekGui();
		window->removeGui(topGui);
		createCollection(name);
		return true;
	};
	addEntry(_("CREATE NEW CUSTOM COLLECTION").c_str(), true, [this, createCustomCollection] {
		if (Settings::getInstance()->getBool("UseOSK")) {
			mWindow->pushGui(new GuiTextEditPopupKeyboard(mWindow, _("New Collection Name"), "", createCustomCollection, false));
		}
		else {
			mWindow->pushGui(new GuiTextEditPopup(mWindow, _("New Collection Name"), "", createCustomCollection, false));
		}
	});


	addGroup(_("OPTIONS"));

	// SORT COLLECTIONS AND SYSTEMS
	std::string sortMode = Settings::getInstance()->getString("SortSystems");

	auto sortType = std::make_shared< OptionListComponent<std::string> >(mWindow, _("SORT COLLECTIONS AND SYSTEMS"), false);
	sortType->add(_("NO"), "", sortMode.empty());
	sortType->add(_("ALPHABETICALLY"), "alpha", sortMode == "alpha");

	if (SystemData::isManufacturerSupported())
	{
		sortType->add(_("BY MANUFACTURER"), "manufacturer", sortMode == "manufacturer");
		sortType->add(_("BY HARDWARE TYPE"), "hardware", sortMode == "hardware");
		sortType->add(_("BY RELEASE YEAR"), "releaseDate", sortMode == "releaseDate");
	}

	if (!sortType->hasSelection())
		sortType->selectFirstItem();

	addWithLabel(_("SORT SYSTEMS"), sortType);
	addSaveFunc([this, sortType]
	{
		if (Settings::getInstance()->setString("SortSystems", sortType->getSelected()))
		if (sortType->getSelected() == "") // force default es_system.cfg order
			setVariable("reloadSystems", true);
		else
			setVariable("reloadAll", true);
	});

	// force full-name on sort systems
	auto forceFullName = std::make_shared<SwitchComponent>(mWindow, Settings::getInstance()->getBool("ForceFullNameSortSystems"));
	addWithLabel(_("USE FULL SYSTEM NAME IN SORT"), forceFullName);
	addSaveFunc([this, forceFullName, sortType]
		{
			if (Settings::getInstance()->setBool("ForceFullNameSortSystems", forceFullName->getState()))
			{
				if ((sortType->getSelected() != "") && (sortType->getSelected() != "alpha"))
					setVariable("reloadAll", true);
			}
		});

	// Optionally start in selected system
	auto systemfocus_list = std::make_shared< OptionListComponent<std::string> >(mWindow, _("START ON SYSTEM"), false);
	systemfocus_list->add(_("NONE"), "", Settings::getInstance()->getString("StartupSystem") == "");

	for (auto it = SystemData::sSystemVector.cbegin(); it != SystemData::sSystemVector.cend(); it++)
		if ("retropie" != (*it)->getName() && (*it)->isVisible())
			systemfocus_list->add((*it)->getName(), (*it)->getName(), Settings::getInstance()->getString("StartupSystem") == (*it)->getName());

	if (!systemfocus_list->hasSelection())
		systemfocus_list->selectFirstItem();

	addWithLabel(_("START ON SYSTEM"), systemfocus_list);
	addSaveFunc([systemfocus_list] {
		Settings::getInstance()->setString("StartupSystem", systemfocus_list->getSelected());
	});

	// Open gamelist at start
	auto bootOnGamelist = std::make_shared<SwitchComponent>(mWindow, Settings::getInstance()->getBool("StartupOnGameList"));
	addWithLabel(_("START ON GAMELIST"), bootOnGamelist);
	addSaveFunc([bootOnGamelist] { Settings::getInstance()->setBool("StartupOnGameList", bootOnGamelist->getState()); });

	std::shared_ptr<SwitchComponent> bundleCustomCollections = std::make_shared<SwitchComponent>(mWindow, Settings::getInstance()->getBool("UseCustomCollectionsSystem"));
	addWithLabel(_("GROUP UNTHEMED CUSTOM COLLECTIONS"), bundleCustomCollections);
	addSaveFunc([this, bundleCustomCollections]
	{
		if (Settings::getInstance()->setBool("UseCustomCollectionsSystem", bundleCustomCollections->getState()))
			setVariable("reloadAll", true);
	});

	std::shared_ptr<SwitchComponent> toggleSystemNameInCollections = std::make_shared<SwitchComponent>(mWindow, Settings::getInstance()->getBool("CollectionShowSystemInfo"));
	addWithLabel(_("SHOW SYSTEM NAME IN COLLECTIONS"), toggleSystemNameInCollections);
	addSaveFunc([this, toggleSystemNameInCollections]
	{
		if (Settings::getInstance()->setBool("CollectionShowSystemInfo", toggleSystemNameInCollections->getState()))
			setVariable("reloadAll", true);
	});

/*
	std::shared_ptr<SwitchComponent> alsoHideGames = std::make_shared<SwitchComponent>(mWindow, Settings::HiddenSystemsShowGames());
	addWithLabel(_("SHOW GAMES OF HIDDEN SYSTEMS IN COLLECTIONS"), alsoHideGames);
	addSaveFunc([this, alsoHideGames]
	{
		if (Settings::setHiddenSystemsShowGames(alsoHideGames->getState()))
		{
			FileData::resetSettings();
			setVariable("reloadSystems", true);
		}
	});
*/

	std::shared_ptr<SwitchComponent> favoritesFirstSwitch = std::make_shared<SwitchComponent>(mWindow, Settings::getInstance()->getBool("FavoritesFirst"));
	addWithLabel(_("SHOW FAVORITES ON TOP"), favoritesFirstSwitch);
	addSaveFunc([this, favoritesFirstSwitch]
	{
		if (Settings::getInstance()->setBool("FavoritesFirst", favoritesFirstSwitch->getState()))
			setVariable("reloadAll", true);
	});

	addEntry(_("UPDATE GAMES LISTS"), false, [this] { GuiMenu::updateGameLists(mWindow); }); // Game List Update

	if (CollectionSystemManager::get()->isEditing())
		addEntry((_("FINISH EDITING COLLECTION") + " : " + Utils::String::toUpper(CollectionSystemManager::get()->getEditingCollection())).c_str(), false, std::bind(&GuiCollectionSystemsOptions::exitEditMode, this));

	addSaveFunc([this]
	{
		std::string newAutoSettings = Utils::String::vectorToCommaString(autoOptionList->getSelectedObjects());
		std::string newCustomSettings = Utils::String::vectorToCommaString(customOptionList->getSelectedObjects());

		bool dirty = Settings::getInstance()->setString("CollectionSystemsAuto", newAutoSettings);
		dirty |= Settings::getInstance()->setString("CollectionSystemsCustom", newCustomSettings);

		if (dirty)
			setVariable("reloadAll", true);
	});

	onFinalize([this]
	{
		if (getVariable("reloadSystems"))
		{
			Window* window = mWindow;
			window->renderLoadingScreen(_("Loading..."));

			ViewController::get()->goToStart();
			delete ViewController::get();
			ViewController::init(window);
			CollectionSystemManager::deinit();
			CollectionSystemManager::init(window);
			SystemData::loadConfig(window);

			GuiComponent* gui;
			while ((gui = window->peekGui()) != NULL)
			{
				window->removeGui(gui);
				if (gui != this)
					delete gui;
			}
			ViewController::get()->reloadAll(nullptr); // Avoid reloading themes a second time
			window->endRenderLoadingScreen();

			window->pushGui(ViewController::get());
		}
		else if (getVariable("reloadAll"))
		{
			Settings::getInstance()->saveFile();

			CollectionSystemManager::get()->loadEnabledListFromSettings();
			CollectionSystemManager::get()->updateSystemsList();
			ViewController::get()->goToStart();
			ViewController::get()->reloadAll(mWindow);
			mWindow->endRenderLoadingScreen();
		}
	});
}

void GuiCollectionSystemsOptions::createCollection(std::string inName)
{
	std::string name = CollectionSystemManager::get()->getValidNewCollectionName(inName);
	SystemData* newSys = CollectionSystemManager::get()->addNewCustomCollection(name);
	customOptionList->add(name, name, true);

	std::string outAuto = Utils::String::vectorToCommaString(autoOptionList->getSelectedObjects());
	std::string outCustom = Utils::String::vectorToCommaString(customOptionList->getSelectedObjects());
	updateSettings(outAuto, outCustom);

	ViewController::get()->goToSystemView(newSys);

	Window* window = mWindow;
	CollectionSystemManager::get()->setEditMode(name);
	while (window->peekGui() && window->peekGui() != ViewController::get())
		delete window->peekGui();

	return;
}

void GuiCollectionSystemsOptions::exitEditMode()
{
	CollectionSystemManager::get()->exitEditMode();
	close();
}

GuiCollectionSystemsOptions::~GuiCollectionSystemsOptions()
{

}

void GuiCollectionSystemsOptions::addSystemsToMenu()
{
	// add Auto Systems && preserve order
	std::map<std::string, CollectionSystemData> &autoSystems = CollectionSystemManager::get()->getAutoCollectionSystems();
	autoOptionList = std::make_shared< OptionListComponent<std::string> >(mWindow, _("SELECT COLLECTIONS"), true);
	bool hasGroup = false;

	for (auto systemDecl : CollectionSystemManager::getSystemDecls())
	{
		auto it = autoSystems.find(systemDecl.name);
		if (it == autoSystems.cend())
			continue;

		if (it->second.decl.displayIfEmpty)
			autoOptionList->add(_(it->second.decl.longName.c_str()), it->second.decl.name, it->second.isEnabled);
		else
		{
			if (!it->second.isPopulated)
				CollectionSystemManager::get()->populateAutoCollection(&(it->second));

			if (it->second.system->getRootFolder()->getChildren().size() == 0)
				continue;

			if (!hasGroup)
			{
				autoOptionList->addGroup(_("ARCADE SYSTEMS"));
				hasGroup = true;
			}

			autoOptionList->add(_(it->second.decl.longName.c_str()), it->second.decl.name, it->second.isEnabled);
		}
	}
	if (!autoOptionList->empty())
		addWithLabel(_("AUTOMATIC GAME COLLECTIONS"), autoOptionList);

	// add Custom Systems
	std::map<std::string, CollectionSystemData> customSystems = CollectionSystemManager::get()->getCustomCollectionSystems();
	customOptionList = std::make_shared< OptionListComponent<std::string> >(mWindow, _("SELECT COLLECTIONS"), true);
	for (std::map<std::string, CollectionSystemData>::const_iterator it = customSystems.cbegin(); it != customSystems.cend(); it++)
	{
		customOptionList->add(it->second.decl.longName, it->second.decl.name, it->second.isEnabled);
	}
	if (!customOptionList->empty())
		addWithLabel(_("CUSTOM GAME COLLECTIONS"), customOptionList);

}

void GuiCollectionSystemsOptions::updateSettings(std::string newAutoSettings, std::string newCustomSettings)
{
	bool dirty = Settings::getInstance()->setString("CollectionSystemsAuto", newAutoSettings);
	dirty |= Settings::getInstance()->setString("CollectionSystemsCustom", newCustomSettings);

	if (dirty)
	{
		Settings::getInstance()->saveFile();
		CollectionSystemManager::get()->loadEnabledListFromSettings();
		CollectionSystemManager::get()->updateSystemsList();
		ViewController::get()->goToStart();
		ViewController::get()->reloadAll();
	}
}
