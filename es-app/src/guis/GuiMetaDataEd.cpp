#include "guis/GuiMetaDataEd.h"

#include "components/ButtonComponent.h"
#include "components/ComponentList.h"
#include "components/DateTimeEditComponent.h"
#include "components/MenuComponent.h"
#include "components/RatingComponent.h"
#include "components/SwitchComponent.h"
#include "components/TextComponent.h"
#include "guis/GuiGameScraper.h"
#include "guis/GuiMsgBox.h"
#include "guis/GuiTextEditPopup.h"
#include "guis/GuiFileBrowser.h"
#include "resources/Font.h"
#include "utils/StringUtil.h"
#include "views/ViewController.h"
#include "CollectionSystemManager.h"
#include "FileData.h"
#include "FileFilterIndex.h"
#include "SystemData.h"
#include "Window.h"
#include "guis/GuiTextEditPopupKeyboard.h"

GuiMetaDataEd::GuiMetaDataEd(Window* window, MetaDataList* md, const std::vector<MetaDataDecl>& mdd, ScraperSearchParams scraperParams,
	const std::string& /*header*/, std::function<void()> saveCallback, std::function<void()> deleteFunc, FileData* file) : GuiComponent(window),
	mScraperParams(scraperParams),

	mBackground(window, ":/frame.png"),
	mGrid(window, Vector2i(1, 3)),

	mMetaDataDecl(mdd),
	mMetaData(md),
	mSavedCallback(saveCallback), mDeleteFunc(deleteFunc)
{
	auto theme = ThemeData::getMenuTheme();
	mBackground.setImagePath(theme->Background.path); // ":/frame.png"
	mBackground.setEdgeColor(theme->Background.color);
	mBackground.setCenterColor(theme->Background.centerColor);
	mBackground.setCornerSize(theme->Background.cornerSize);

	addChild(&mBackground);
	addChild(&mGrid);

	mHeaderGrid = std::make_shared<ComponentGrid>(mWindow, Vector2i(1, 5));

	mTitle = std::make_shared<TextComponent>(mWindow, _("EDIT METADATA"), theme->Title.font, theme->Title.color, ALIGN_CENTER);
	mSubtitle = std::make_shared<TextComponent>(mWindow, Utils::String::toUpper(Utils::FileSystem::getFileName(scraperParams.game->getPath())),
		theme->TextSmall.font, theme->TextSmall.color, ALIGN_CENTER);

	mHeaderGrid->setEntry(mTitle, Vector2i(0, 1), false, true);
	mHeaderGrid->setEntry(mSubtitle, Vector2i(0, 3), false, true);

	mGrid.setEntry(mHeaderGrid, Vector2i(0, 0), false, true);

	mList = std::make_shared<ComponentList>(mWindow);
	mGrid.setEntry(mList, Vector2i(0, 1), true, true);


	SystemData* system = file->getSystem();

	auto emul_choice = std::make_shared<OptionListComponent<std::string>>(mWindow, _("SELECT EMULATOR"), false);
	auto core_choice = std::make_shared<OptionListComponent<std::string>>(mWindow, _("SELECT CORE"), false);
	std::string relativePath = mMetaData->getRelativeRootPath();

	// populate list
	for(auto iter = mdd.cbegin(); iter != mdd.cend(); iter++)
	{
		std::shared_ptr<GuiComponent> ed;

		// don't add statistics
		if(iter->isStatistic)
			continue;

		// create ed and add it (and any related components) to mMenu
		// ed's value will be set below
		ComponentListRow row;

		if (iter->displayName == "emulator")
		{
			std::string defaultEmul = system->getSystemEnvData()->getDefaultEmulator();
			std::string currentEmul = file->getEmulator();

			if (defaultEmul.length() == 0)
				emul_choice->add(_("DEFAULT"), "", true);
			else
				emul_choice->add(_("DEFAULT") + " (" + defaultEmul + ")", "", currentEmul.length() == 0);

			for (auto core : file->getSystem()->getSystemEnvData()->mEmulators)
				emul_choice->add(core.mName, core.mName, core.mName == currentEmul);

			row.addElement(std::make_shared<TextComponent>(mWindow, _("EMULATOR"), theme->Text.font, theme->Text.color), true);
			row.addElement(emul_choice, false);

			mList->addRow(row);
			emul_choice->setTag(iter->key);
			mEditors.push_back(emul_choice);
			
			emul_choice->setSelectedChangedCallback([this, system, core_choice, file](std::string emulatorName)
			{
				std::string currentCore = file->getCore();

				std::string defaultCore = system->getSystemEnvData()->getDefaultCore(emulatorName);
				if (emulatorName.length() == 0)
					defaultCore = system->getSystemEnvData()->getDefaultCore(system->getSystemEnvData()->getDefaultEmulator());

				core_choice->clear();
				if (defaultCore.length() == 0)
					core_choice->add(_("DEFAULT"), "", false);
				else 
					core_choice->add(_("DEFAULT") + " (" + defaultCore + ")", "", false);
							
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
			
			continue;
		}
		
		if (iter->displayName == "core")
		{
		//	core_choice->add(_("DEFAULT"), "", true);
			core_choice->setTag(iter->key);

			row.addElement(std::make_shared<TextComponent>(mWindow, _("CORE"), theme->Text.font, theme->Text.color), true);
			row.addElement(core_choice, false);

			mList->addRow(row);
			ed = core_choice;

			mEditors.push_back(core_choice);

			// force change event to load core list
			emul_choice->invalidate();
			continue;
		}
		
		auto lbl = std::make_shared<TextComponent>(mWindow, _(Utils::String::toUpper(iter->displayName)), theme->Text.font, theme->Text.color);
		row.addElement(lbl, true); // label

		switch (iter->type)
		{
			case MD_BOOL:
			{
				ed = std::make_shared<SwitchComponent>(window);
				row.addElement(ed, false, true);
				break;
			}
			case MD_RATING:
			{
				ed = std::make_shared<RatingComponent>(window);
				const float height = lbl->getSize().y() * 0.71f;
				ed->setSize(0, height);
				row.addElement(ed, false, true, true);

				auto spacer = std::make_shared<GuiComponent>(mWindow);
				spacer->setSize(Renderer::getScreenWidth() * 0.0025f, 0);
				row.addElement(spacer, false);

				// pass input to the actual RatingComponent instead of the spacer
				row.input_handler = std::bind(&GuiComponent::input, ed.get(), std::placeholders::_1, std::placeholders::_2);

				break;
			}
			case MD_DATE:
			{
				ed = std::make_shared<DateTimeEditComponent>(window);
				row.addElement(ed, false, true, true);

				auto spacer = std::make_shared<GuiComponent>(mWindow);
				spacer->setSize(Renderer::getScreenWidth() * 0.0025f, 0);
				row.addElement(spacer, false);

				// pass input to the actual DateTimeEditComponent instead of the spacer
				row.input_handler = std::bind(&GuiComponent::input, ed.get(), std::placeholders::_1, std::placeholders::_2);

				break;
			}
			case MD_TIME:
			{
				ed = std::make_shared<DateTimeEditComponent>(window, DateTimeEditComponent::DISP_RELATIVE_TO_NOW);
				row.addElement(ed, false);
				break;
			}

			case MD_PATH:
			{
				ed = std::make_shared<TextComponent>(window, "", theme->Text.font, theme->Text.color, ALIGN_RIGHT);
				row.addElement(ed, true);

				auto spacer = std::make_shared<GuiComponent>(mWindow);
				spacer->setSize(Renderer::getScreenWidth() * 0.005f, 0);
				row.addElement(spacer, false);

				auto bracket = std::make_shared<ImageComponent>(mWindow);
				bracket->setImage(ThemeData::getMenuTheme()->Icons.arrow);
				bracket->setResize(Vector2f(0, lbl->getFont()->getLetterHeight()));
				row.addElement(bracket, false);

				GuiFileBrowser::FileTypes type = GuiFileBrowser::FileTypes::IMAGES;

				if (iter->key == "video")
					type = GuiFileBrowser::FileTypes::VIDEO;
				else if (iter->key == "manual" || iter->key == "magazine" || iter->key == "map")
					type = (GuiFileBrowser::FileTypes) (GuiFileBrowser::FileTypes::IMAGES | GuiFileBrowser::FileTypes::MANUALS);

				auto updateVal = [ed, relativePath](const std::string& newVal)
				{
					auto val = Utils::FileSystem::createRelativePath(newVal, relativePath, true);
					ed->setValue(val);
					return true;
				};

				row.makeAcceptInputHandler([this, type, ed, iter, updateVal, relativePath]
				{
					std::string filePath = ed->getValue();
					if (!filePath.empty())
						filePath = Utils::FileSystem::resolveRelativePath(filePath, relativePath, true);

					std::string dir = Utils::FileSystem::getParent(filePath);
					//if (dir.empty())
						//dir = relativePath;

					std::string title = iter->displayName + " - " + mMetaData->getName();

					if (Settings::getInstance()->getBool("ShowFileBrowser"))
						mWindow->pushGui(new GuiFileBrowser(mWindow, dir, filePath, type, updateVal, title));
					else
						mWindow->pushGui(new GuiTextEditPopupKeyboard(mWindow, title, filePath, updateVal, false));
				});

				break;
			}

			case MD_MULTILINE_STRING:
			default:
			{
				// MD_STRING
				ed = std::make_shared<TextComponent>(window, "", theme->Text.font, theme->Text.color, ALIGN_RIGHT);
				row.addElement(ed, true);

				auto spacer = std::make_shared<GuiComponent>(mWindow);
				spacer->setSize(Renderer::getScreenWidth() * 0.005f, 0);
				row.addElement(spacer, false);

				auto bracket = std::make_shared<ImageComponent>(mWindow);
				bracket->setImage(ThemeData::getMenuTheme()->Icons.arrow);// ":/arrow.svg");
				bracket->setResize(Vector2f(0, lbl->getFont()->getLetterHeight()));
				row.addElement(bracket, false);

				bool multiLine = iter->type == MD_MULTILINE_STRING;
				const std::string title = _(iter->displayPrompt.c_str());
				if (ed->getValue() == "unknown")
					ed->setValue(_("unknown"));

				auto updateVal = [ed](const std::string& newVal) {
						std::string new_value = newVal;
						if (new_value == "unknown")
							new_value = _("unknown").c_str();

					ed->setValue(new_value);
					return true;
				}; // ok callback (apply new value to ed)
				row.makeAcceptInputHandler([this, title, ed, updateVal, multiLine] 
				{
					if (!multiLine && Settings::getInstance()->getBool("UseOSK"))
						mWindow->pushGui(new GuiTextEditPopupKeyboard(mWindow, title, ed->getValue(), updateVal, multiLine));
					else
						mWindow->pushGui(new GuiTextEditPopup(mWindow, title, ed->getValue(), updateVal, multiLine));
				});
				break;
			}
		}

		assert(ed);
		mList->addRow(row);

		std::string ed_value = mMetaData->get(iter->key);
		if (ed_value == "unknown")
			ed_value = _("unknown");

		ed->setTag(iter->key);
		ed->setValue(ed_value);

		mEditors.push_back(ed);
	}

	std::vector< std::shared_ptr<ButtonComponent> > buttons;

	if (!scraperParams.system->hasPlatformId(PlatformIds::PLATFORM_IGNORE))
		buttons.push_back(std::make_shared<ButtonComponent>(mWindow, _("SCRAPE"), _("SCRAPE"), std::bind(&GuiMetaDataEd::fetch, this)));

	buttons.push_back(std::make_shared<ButtonComponent>(mWindow, _("SAVE"), _("SAVE"), [&] { save(); delete this; }));
	buttons.push_back(std::make_shared<ButtonComponent>(mWindow, _("CANCEL"), _("CANCEL"), [&] { delete this; }));

	if(mDeleteFunc)
	{
		auto deleteFileAndSelf = [&] { mDeleteFunc(); delete this; };
		auto deleteBtnFunc = [this, deleteFileAndSelf] { mWindow->pushGui(new GuiMsgBox(mWindow,
			_("THIS WILL DELETE THE ACTUAL GAME FILE(S)!\nARE YOU SURE?"),
			_("YES"), deleteFileAndSelf, _("NO"), nullptr)); };
		buttons.push_back(std::make_shared<ButtonComponent>(mWindow, _("DELETE"), _("DELETE"), deleteBtnFunc));
	}

	mButtons = makeButtonGrid(mWindow, buttons);
	mGrid.setEntry(mButtons, Vector2i(0, 2), true, false);

	mGrid.setUnhandledInputCallback([this](InputConfig* config, Input input) -> bool {
		if (config->isMappedLike("down", input)) {
			mGrid.setCursorTo(mList);
			mList->setCursorIndex(0);
			return true;
		}
		if (config->isMappedLike("up", input)) {
			mList->setCursorIndex(mList->size() - 1);
			mGrid.moveCursor(Vector2i(0, 1));
			return true;
		}
		return false;
	});

	// resize
	bool change_height_ratio = Settings::getInstance()->getBool("ShowHelpPrompts");
	float height_ratio = 1.0f;
	if ( change_height_ratio )
	{
		height_ratio = 0.88f;
		if ( Settings::getInstance()->getBool("MenusOnDisplayTop") || Settings::getInstance()->getBool("MenusAllHeight") )
			height_ratio = 0.93f;
	}

	setSize(Renderer::getScreenWidth(), Renderer::getScreenHeight() * height_ratio);

	// center
	float new_y = (Renderer::getScreenHeight() - mSize.y()) / 2;
	if ( Settings::getInstance()->getBool("MenusOnDisplayTop") || Settings::getInstance()->getBool("MenusAllHeight") )
		new_y = 0.f;

	setPosition(0.f, new_y);
}

void GuiMetaDataEd::onSizeChanged()
{
	mBackground.fitTo(mSize, Vector3f::Zero(), Vector2f(-32, -32));

	mGrid.setSize(mSize);

	const float titleHeight = mTitle->getFont()->getLetterHeight();
	const float subtitleHeight = mSubtitle->getFont()->getLetterHeight();
	const float titleSubtitleSpacing = mSize.y() * 0.03f;

	mGrid.setRowHeightPerc(0, (titleHeight + titleSubtitleSpacing + subtitleHeight + TITLE_VERT_PADDING) / mSize.y());
	mGrid.setRowHeightPerc(2, mButtons->getSize().y() / mSize.y());

	mHeaderGrid->setRowHeightPerc(1, titleHeight / mHeaderGrid->getSize().y());
	mHeaderGrid->setRowHeightPerc(2, titleSubtitleSpacing / mHeaderGrid->getSize().y());
	mHeaderGrid->setRowHeightPerc(3, subtitleHeight / mHeaderGrid->getSize().y());
}


#include "Gamelist.h"

void GuiMetaDataEd::save()
{
	// remove game from index
	mScraperParams.system->removeFromIndex(mScraperParams.game);

	for (unsigned int i = 0; i < mEditors.size(); i++)
	{			
		std::shared_ptr<GuiComponent> ed = mEditors.at(i);

		auto val = ed->getValue();
		auto key = ed->getTag();

		if (key == "core" || key == "emulator")
		{
			std::shared_ptr<OptionListComponent<std::string>> list = std::static_pointer_cast<OptionListComponent<std::string>>(ed);
			val = list->getSelected();
		}

		mMetaData->set(key, val);
	}

	// enter game in index
	mScraperParams.system->addToIndex(mScraperParams.game);

	if (mSavedCallback)
		mSavedCallback();

	saveToGamelistRecovery(mScraperParams.game);

	// update respective Collection Entries
	CollectionSystemManager::get()->refreshCollectionSystems(mScraperParams.game);
}

void GuiMetaDataEd::fetch()
{
//	mScraperParams.nameOverride = mScraperParams.game->getName();

	GuiGameScraper* scr = new GuiGameScraper(mWindow, mScraperParams, std::bind(&GuiMetaDataEd::fetchDone, this, std::placeholders::_1));
	mWindow->pushGui(scr);
}

void GuiMetaDataEd::fetchDone(const ScraperSearchResult& result)
{
	for (unsigned int i = 0; i < mEditors.size(); i++)
	{
		auto key = mEditors.at(i)->getTag();
		if (isStatistic(key))
			continue;

		// Don't override favorite & hidden values, as they are not statistics
		if (key == "favorite" || key == "hidden")
			continue;

		if (key == "rating" && result.mdl.getFloat(MetaDataId::Rating) < 0)
			continue;

		// Don't override medias when scrap result has nothing
		if ((key == "image" || key == "thumbnail" || key == "marquee" || key == "fanart" || key == "titleshot" || key == "manual" || key == "map" || key == "video" || key == "magazine") && result.mdl.get(key).empty())
			continue;

		mEditors.at(i)->setValue(result.mdl.get(key));
	}

	//mScrappedPk2 = result.p2k;
}

void GuiMetaDataEd::close(bool closeAllWindows, bool ignoreChanges)
{

	// find out if the user made any changes
	bool dirty = false;

	if (!ignoreChanges)
	{
		for(unsigned int i = 0; i < mEditors.size(); i++)
		{
			auto key = mEditors.at(i)->getTag();
			if(mMetaData->get(key) != mEditors.at(i)->getValue())
			{
				dirty = true;
				break;
			}
		}
	}

	std::function<void()> closeFunc;
	if(!closeAllWindows)
	{
		closeFunc = [this] { delete this; };
	}else{
		Window* window = mWindow;
		closeFunc = [window, this] {
			while(window->peekGui() != ViewController::get())
				delete window->peekGui();
		};
	}


	if(dirty)
	{
		// changes were made, ask if the user wants to save them
		mWindow->pushGui(new GuiMsgBox(mWindow,
			_("SAVE CHANGES ?"),
			_("YES"), [this, closeFunc] { save(); closeFunc(); },
			_("NO"), closeFunc
		));
	}else{
		closeFunc();
	}
}

bool GuiMetaDataEd::input(InputConfig* config, Input input)
{
	if(GuiComponent::input(config, input))
		return true;

	const bool isStart = config->isMappedTo("start", input);
	if (input.value != 0)
	{
		if (config->isMappedTo(BUTTON_BACK, input) || isStart)
		{
			close(isStart);
			return true;
		}
		if (config->isMappedTo("select", input))
		{
			save();
			if (Settings::getInstance()->getBool("GuiEditMetadataCloseAllWindows"))
				close(true, true);
			else
				delete this;
		}
	}

	return false;
}

std::vector<HelpPrompt> GuiMetaDataEd::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts = mGrid.getHelpPrompts();
	prompts.push_back(HelpPrompt(BUTTON_BACK, _("BACK")));
	prompts.push_back(HelpPrompt("start", _("CLOSE")));
	prompts.push_back(HelpPrompt("select", _("SAVE")));
	return prompts;
}

bool GuiMetaDataEd::isStatistic(const std::string name)
{
	for (auto in : mMetaDataDecl)
		if (in.key == name && in.isStatistic)
			return true;

	return false;
}
