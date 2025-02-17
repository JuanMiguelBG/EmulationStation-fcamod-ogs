#include "views/SystemView.h"

#include "animations/LambdaAnimation.h"
#include "guis/GuiMsgBox.h"
#include "views/UIModeController.h"
#include "views/ViewController.h"
#include "Log.h"
#include "Scripting.h"
#include "Settings.h"
#include "SystemData.h"
#include "EsLocale.h"
#include "Window.h"
#include "AudioManager.h"
#include "components/VideoComponent.h"
#include "components/VideoVlcComponent.h"
#include "CollectionSystemManager.h"
#include <random>
#include "guis/GuiMsgBox.h"
#include "guis/GuiSettings.h"
#include "guis/GuiTextEditPopupKeyboard.h"
#include "guis/GuiTextEditPopup.h"
#include "guis/GuiMenu.h"

// buffer values for scrolling velocity (left, stopped, right)
const int logoBuffersLeft[] = { -5, -2, -1 };
const int logoBuffersRight[] = { 1, 2, 5 };

SystemView::SystemView(Window* window) : IList<SystemViewData, SystemData*>(window, LIST_SCROLL_STYLE_SLOW, LIST_ALWAYS_LOOP),
										 mViewNeedsReload(true),
										 mSystemInfo(window, _("SYSTEM INFO"), Font::get(FONT_SIZE_SMALL), 0x33333300, ALIGN_CENTER)
{
	mCamOffset = 0;
	mExtrasCamOffset = 0;
	mExtrasFadeOpacity = 0.0f;
	mLastSystem = nullptr;
	mScreensaverActive = false;
	mDisable = false;
	mLastCursor = 0;
	mStaticBackground = nullptr;
	mStaticVideoBackground = nullptr;
	mExtrasFadeOldCursor = -1;
	
	setSize((float)Renderer::getScreenWidth(), (float)Renderer::getScreenHeight());
	populate();
}

SystemView::~SystemView()
{
	if (mStaticVideoBackground != nullptr)
	{
		delete mStaticVideoBackground;
		mStaticVideoBackground = nullptr;
	}

	if (mStaticBackground != nullptr)
	{
		delete mStaticBackground;
		mStaticBackground = nullptr;
	}

	clearEntries();
}

void SystemView::clearEntries()
{
	for (int i = 0; i < mEntries.size(); i++)
	{
		for (auto extra : mEntries[i].data.backgroundExtras)
			delete extra;

		mEntries[i].data.backgroundExtras.clear();
	}

	mEntries.clear();
}

class SystemRandomPlaylist : public IPlaylist
{
public:
	enum PlaylistType
	{
		IMAGE,
		THUMBNAIL,
		MARQUEE,
		VIDEO
	};

	SystemRandomPlaylist(SystemData* system, PlaylistType type) : mMt19937(mRandomDevice())
	{
		mFirstRun = true;
		mSystem = system;
		mType = type;
	}
		
	std::string getNextItem()
	{
		if (mFirstRun)
		{
			std::vector<FileData*> files = mSystem->getRootFolder()->getFilesRecursive(GAME, false);

			for (auto file : files)
			{
				switch (mType)
				{
				case IMAGE:
					if (!file->getImagePath().empty())
						mPaths.push_back(file->getImagePath());
					break;

				case THUMBNAIL:
					if (!file->getThumbnailPath().empty())
						mPaths.push_back(file->getThumbnailPath());
					break;

				case MARQUEE:
					if (!file->getMarqueePath().empty())
						mPaths.push_back(file->getMarqueePath());
					break;

				case VIDEO:
					if (!file->getVideoPath().empty())
						mPaths.push_back(file->getVideoPath());
					break;
				}
			}

			if (mPaths.size() > 0)
				mUniformDistribution = std::uniform_int_distribution<int>(0, mPaths.size() - 1);

			mFirstRun = false;
		}

		if (mPaths.size() > 0)
		{
			int idx = mUniformDistribution(mMt19937);
			if (idx >= 0 && idx < mPaths.size() && Utils::FileSystem::exists(mPaths[idx]))
				return mPaths[idx];

			// File not found ? Try the next file...
			int stopidx = idx;

			idx++;
			if (idx >= mPaths.size())
				idx = 0;

			while (idx != stopidx && idx < mPaths.size() && !Utils::FileSystem::exists(mPaths[idx]))
			{			
				idx++;
				if (idx >= mPaths.size())
					idx = 0;
			}

			if (idx >= 0 && idx < mPaths.size() && Utils::FileSystem::exists(mPaths[idx]))
				return mPaths[idx];
		}

		return "";
	}
	
private:
	SystemData*		mSystem;
	bool			mFirstRun;
	PlaylistType	mType;

	std::vector<std::string> mPaths;

	std::random_device	mRandomDevice;
	std::mt19937		mMt19937;
	std::uniform_int_distribution<int> mUniformDistribution;
};

void SystemView::populate()
{
	clearEntries();

	for(auto it = SystemData::sSystemVector.cbegin(); it != SystemData::sSystemVector.cend(); it++)
	{
		const std::shared_ptr<ThemeData>& theme = (*it)->getTheme();

		if (theme == nullptr)
			 continue;

		if(mViewNeedsReload)
			getViewElements(theme);

		if (!(*it)->isVisible())
			continue;

		Entry e;
		e.name = (*it)->getName();
		e.object = *it;

		// make logo
		const ThemeData::ThemeElement* logoElem = theme->getElement("system", "logo", "image");
		if (logoElem && logoElem->has("path"))
		{
			std::string path = logoElem->get<std::string>("path");
			std::string defaultPath = logoElem->has("default") ? logoElem->get<std::string>("default") : "";
			
			if ((!path.empty() && ResourceManager::getInstance()->fileExists(path))
				|| (!defaultPath.empty() && ResourceManager::getInstance()->fileExists(defaultPath)))
			{
				// Remove dynamic flags for png & jpg files : themes can contain oversized images that can't be unloaded by the TextureResource manager
				ImageComponent* logo = new ImageComponent(mWindow, false, Utils::String::toLower(Utils::FileSystem::getExtension(path)) != ".svg");
				logo->setMaxSize(mCarousel.logoSize * mCarousel.logoScale);
				logo->applyTheme(theme, "system", "logo", ThemeFlags::COLOR | ThemeFlags::ALIGNMENT | ThemeFlags::VISIBLE); //  ThemeFlags::PATH | 

				// Process here to be enable to set max picture size
				auto elem = theme->getElement("system", "logo", "image");
				if (elem && elem->has("path"))
				{
					auto path = elem->get<std::string>("path");
					if (Utils::FileSystem::exists(path))
						logo->setImage(path, (elem->has("tile") && elem->get<bool>("tile")), MaxSizeInfo(mCarousel.logoSize * mCarousel.logoScale));
				}

				if (mCarousel.size.x() != mCarousel.logoSize.x() & mCarousel.size.y() != mCarousel.logoSize.y())
					logo->setRotateByTargetSize(true);
					
				e.data.logo = std::shared_ptr<GuiComponent>(logo);
			}
		}

		if (!e.data.logo)
		{
			// no logo in theme; use text
			TextComponent* text = new TextComponent(mWindow,
				(*it)->getFullName(),
				Font::get(FONT_SIZE_LARGE),
				0x000000FF,
				ALIGN_CENTER);
			text->setSize(mCarousel.logoSize * mCarousel.logoScale);
			text->applyTheme((*it)->getTheme(), "system", "logoText", ThemeFlags::FONT_PATH | ThemeFlags::FONT_SIZE | ThemeFlags::COLOR | ThemeFlags::FORCE_UPPERCASE | ThemeFlags::LINE_SPACING | ThemeFlags::TEXT);
			e.data.logo = std::shared_ptr<GuiComponent>(text);

			if (mCarousel.type == VERTICAL || mCarousel.type == VERTICAL_WHEEL)
			{
				text->setHorizontalAlignment(mCarousel.logoAlignment);
				text->setVerticalAlignment(ALIGN_CENTER);
			}
			else 
			{
				text->setHorizontalAlignment(ALIGN_CENTER);
				text->setVerticalAlignment(mCarousel.logoAlignment);
			}
		}
		
		if (mCarousel.type == VERTICAL || mCarousel.type == VERTICAL_WHEEL)
		{
			if (mCarousel.logoAlignment == ALIGN_LEFT)
				e.data.logo->setOrigin(0, 0.5);
			else if (mCarousel.logoAlignment == ALIGN_RIGHT)
				e.data.logo->setOrigin(1.0, 0.5);
			else
				e.data.logo->setOrigin(0.5, 0.5);
		}
		else {
			if (mCarousel.logoAlignment == ALIGN_TOP)
				e.data.logo->setOrigin(0.5, 0);
			else if (mCarousel.logoAlignment == ALIGN_BOTTOM)
				e.data.logo->setOrigin(0.5, 1);
			else
				e.data.logo->setOrigin(0.5, 0.5);
		}

		
		Vector2f denormalized = mCarousel.logoSize * e.data.logo->getOrigin();
		e.data.logo->setPosition(denormalized.x(), denormalized.y(), 0.0);
		// delete any existing extras
		for (auto extra : e.data.backgroundExtras)
			delete extra;
		e.data.backgroundExtras.clear();
		
		// make background extras
		e.data.backgroundExtras = ThemeData::makeExtras((*it)->getTheme(), "system", mWindow);
		
		for (auto extra : e.data.backgroundExtras)
		{
			if (extra->isKindOf<VideoComponent>())
			{
				auto elem = (*it)->getTheme()->getElement("system", extra->getTag(), "video");
				if (elem != nullptr && elem->has("path") && Utils::String::startsWith(elem->get<std::string>("path"), "{random"))
					((VideoComponent*)extra)->setPlaylist(std::make_shared<SystemRandomPlaylist>(*it, SystemRandomPlaylist::VIDEO));
			}
			else if (extra->isKindOf<ImageComponent>())
			{
				auto elem = (*it)->getTheme()->getElement("system", extra->getTag(), "image");
				if (elem != nullptr && elem->has("path") && Utils::String::startsWith(elem->get<std::string>("path"), "{random"))
				{
					std::string src = elem->get<std::string>("path");

					SystemRandomPlaylist::PlaylistType type = SystemRandomPlaylist::IMAGE;

					if (src == "{random:thumbnail}")
						type = SystemRandomPlaylist::THUMBNAIL;
					else if (src == "{random:marquee}")
						type = SystemRandomPlaylist::MARQUEE;

					((ImageComponent*)extra)->setPlaylist(std::make_shared<SystemRandomPlaylist>(*it, type));
				}
			}
		}
	
		// sort the extras by z-index
		std::stable_sort(e.data.backgroundExtras.begin(), e.data.backgroundExtras.end(), [](GuiComponent* a, GuiComponent* b) {
			return b->getZIndex() > a->getZIndex();
		});
		
		this->add(e);
	}

	if (mEntries.size() == 0)
	{
		// Something is wrong, there is not a single system to show, check if UI mode is not full
		if (!UIModeController::getInstance()->isUIModeFull())
		{
			Settings::getInstance()->setString("UIMode", "Full");
			mWindow->pushGui(new GuiMsgBox(mWindow, _("The selected UI mode has nothing to show,\n returning to UI mode: FULL"), _("OK"), nullptr));
		}
		
		if (Settings::getInstance()->setString("HiddenSystems", ""))
		{
			Settings::getInstance()->saveFile();

			// refresh GUI
			populate();
			mWindow->pushGui(new GuiMsgBox(mWindow, _("ERROR: EVERY SYSTEM IS HIDDEN, RE-DISPLAYING ALL OF THEM NOW"), _("OK"), nullptr));
		}
	}
}

void SystemView::goToSystem(SystemData* system, bool animate)
{
	setCursor(system);

	if(!animate)
		finishAnimation(0);
}

static void _moveCursorInRange(int& value, int count, int sz)
{
	if (count >= sz)
		return;

	value += count;
	if (value < 0) value += sz;
	if (value >= sz) value -= sz;
}

int SystemView::moveCursorFast(bool forward)
{
	int cursor = mCursor;

	if(SystemData::isManufacturerSupported() && Settings::getInstance()->getString("SortSystems") == "alpha" && mCursor >= 0 && mCursor < mEntries.size())
	{
		char alpha = Utils::String::toUpper(mEntries[mCursor].object->getFullName())[0];

		int direction = forward ? 1 : -1;

		_moveCursorInRange(cursor, direction, mEntries.size());

		while (cursor != mCursor && Utils::String::toUpper(mEntries[cursor].object->getFullName())[0] == alpha)
			_moveCursorInRange(cursor, direction, mEntries.size());

		if (!forward && cursor != mCursor)
		{
			// Find first item
			alpha = Utils::String::toUpper(mEntries[cursor].object->getFullName())[0];
			while (cursor != mCursor && Utils::String::toUpper(mEntries[cursor].object->getFullName())[0] == alpha)
				_moveCursorInRange(cursor, -1, mEntries.size());

			_moveCursorInRange(cursor, 1, mEntries.size());
		}
	}
	else if (SystemData::isManufacturerSupported() && Settings::getInstance()->getString("SortSystems") == "manufacturer" && mCursor >= 0 && mCursor < mEntries.size())
	{
		std::string man = mEntries[mCursor].object->getSystemMetadata().manufacturer;

		int direction = forward ? 1 : -1;

		_moveCursorInRange(cursor, direction, mEntries.size());

		while (cursor != mCursor && mEntries[cursor].object->getSystemMetadata().manufacturer == man)
			_moveCursorInRange(cursor, direction, mEntries.size());

		if (!forward && cursor != mCursor)
		{
			// Find first item
			man = mEntries[cursor].object->getSystemMetadata().manufacturer;
			while (cursor != mCursor && mEntries[cursor].object->getSystemMetadata().manufacturer == man)
				_moveCursorInRange(cursor, -1, mEntries.size());

			_moveCursorInRange(cursor, 1, mEntries.size());
		}
	}
	else if(SystemData::isManufacturerSupported() && Settings::getInstance()->getString("SortSystems") == "hardware" && mCursor >= 0 && mCursor < mEntries.size())
	{
		std::string hwt = mEntries[mCursor].object->getSystemMetadata().hardwareType;

		int direction = forward ? 1 : -1;

		_moveCursorInRange(cursor, direction, mEntries.size());

		while (cursor != mCursor && mEntries[cursor].object->getSystemMetadata().hardwareType == hwt)
			_moveCursorInRange(cursor, direction, mEntries.size());

		if (!forward && cursor != mCursor)
		{
			// Find first item
			hwt = mEntries[cursor].object->getSystemMetadata().hardwareType;
			while (cursor != mCursor && mEntries[cursor].object->getSystemMetadata().hardwareType == hwt)
				_moveCursorInRange(cursor, -1, mEntries.size());

			_moveCursorInRange(cursor, 1, mEntries.size());
		}
	}
	else if(SystemData::isManufacturerSupported() && Settings::getInstance()->getString("SortSystems") == "releaseDate" && mCursor >= 0 && mCursor < mEntries.size())
	{
		int year = (mEntries[mCursor].object->getSystemMetadata().releaseYear / 10) * 10;

		int direction = forward ? 1 : -1;

		_moveCursorInRange(cursor, direction, mEntries.size());

		while (cursor != mCursor && ((mEntries[cursor].object->getSystemMetadata().releaseYear / 10) * 10) == year)
			_moveCursorInRange(cursor, direction, mEntries.size());

		if (!forward && cursor != mCursor)
		{
			// Find first item
			year = (mEntries[cursor].object->getSystemMetadata().releaseYear / 10) * 10;
			while (cursor != mCursor && ((mEntries[cursor].object->getSystemMetadata().releaseYear / 10) * 10) == year)
				_moveCursorInRange(cursor, -1, mEntries.size());

			_moveCursorInRange(cursor, 1, mEntries.size());
		}
	}
	else
		_moveCursorInRange(cursor, forward ? 10 : -10, mEntries.size());

	return cursor;
}

void SystemView::showQuickSearch()
{
	SystemData* all = SystemData::getSystem("all");
	if (all != nullptr)
	{
		auto updateVal = [this, all](const std::string& newVal)
		{
			auto index = all->getIndex(true);

			index->resetFilters();
			index->setTextFilter(newVal);

			ViewController::get()->reloadGameListView(all);
			ViewController::get()->goToGameList(all, false);
			return true;
		};

		if (Settings::getInstance()->getBool("UseOSK"))
			mWindow->pushGui(new GuiTextEditPopupKeyboard(mWindow, _("QUICK SEARCH"), "", updateVal, false));
		else
			mWindow->pushGui(new GuiTextEditPopup(mWindow, _("QUICK SEARCH"), "", updateVal, false));
	}
}

bool SystemView::input(InputConfig* config, Input input)
{
	if(input.value != 0)
	{
		if (config->isMappedTo("y", input))
		{
			showQuickSearch();
			return true;
		}
		if(config->getDeviceId() == DEVICE_KEYBOARD && input.value && input.id == SDLK_r && SDL_GetModState() & KMOD_LCTRL && Settings::getInstance()->getBool("Debug"))
		{
			LOG(LogInfo) << "SystemView::input() - Reloading all";
			ViewController::get()->reloadAll();
			return true;
		}

		switch (mCarousel.type)
		{
		case VERTICAL:
		case VERTICAL_WHEEL:
			if (config->isMappedLike("up", input))
			{
				listInput(-1);
				return true;
			}
			if (config->isMappedLike("down", input))
			{
				listInput(1);
				return true;
			}
			if (config->isMappedTo(BUTTON_PD, input))
			{
				int cursor = moveCursorFast(true);
				auto sd = mEntries.at(cursor).object;
				ViewController::get()->goToSystemView(sd, true);
				//listInput(cursor - mCursor);
				return true;
			}
			if (config->isMappedTo(BUTTON_PU, input))
			{
				int cursor = moveCursorFast(false);
				auto sd = mEntries.at(cursor).object;
				ViewController::get()->goToSystemView(sd, true);
				//listInput(cursor - mCursor);
				return true;
			}

			break;
		case HORIZONTAL:
		case HORIZONTAL_WHEEL:
		default:
			if (config->isMappedLike("left", input))
			{
				listInput(-1);
				return true;
			}
			if (config->isMappedLike("right", input))
			{
				listInput(1);
				return true;
			}
			if (config->isMappedTo(BUTTON_PD, input))
			{
				int cursor = moveCursorFast(true);
				auto sd = mEntries.at(cursor).object;
				ViewController::get()->goToSystemView(sd, true);
				//listInput(cursor - mCursor);
				return true;
			}
			if (config->isMappedTo(BUTTON_PU, input))
			{
				int cursor = moveCursorFast(false);
				auto sd = mEntries.at(cursor).object;
				ViewController::get()->goToSystemView(sd, true);
				//listInput(cursor - mCursor);
				return true;
			}

			break;
		}

		if (config->isMappedTo(BUTTON_OK, input))
		{
			stopScrolling();
			ViewController::get()->goToGameList(getSelected());
			return true;
		}

		if (config->isMappedTo(BUTTON_BACK, input) && SystemData::isManufacturerSupported())
		{
			auto sortMode = Settings::getInstance()->getString("SortSystems");
			if (sortMode == "alpha")
			{
				showNavigationBar(_("GO TO LETTER"), [](SystemData* meta) { if (meta->isCollection()) return _("COLLECTIONS"); if (Settings::getInstance()->getBool("SpecialAlphaSort") && (meta->getSystemMetadata().hardwareType == "system")) return _("SYSTEM TOOL"); return Utils::String::toUpper(meta->getSystemMetadata().fullName.substr(0, 1)); });
				return true;
			}
			else if (sortMode == "manufacturer")
			{
				showNavigationBar(_("GO TO MANUFACTURER"), [](SystemData* meta) { return meta->getSystemMetadata().manufacturer; });
				return true;
			}
			else if (sortMode == "hardware")
			{
				showNavigationBar(_("GO TO HARDWARE"), [](SystemData* meta) { return meta->getSystemMetadata().hardwareType; });
				return true;
			}
			else if (sortMode == "releaseDate")
			{
				showNavigationBar(_("GO TO DECADE"), [](SystemData* meta) { if (meta->getSystemMetadata().releaseYear == 0) return _("UNKNOWN"); return std::to_string((meta->getSystemMetadata().releaseYear / 10) * 10) + "'s"; });
				return true;
			}
		}

		if (config->isMappedTo("x", input))
		{
			// get random system
			// go to system
			setCursor(SystemData::getRandomSystem());
			return true;
		}

		if (!UIModeController::getInstance()->isUIModeKid())
		{
			if (config->isMappedTo("select", input) && Settings::getInstance()->getBool("ShowQuitMenuWithSelect"))
			{
				GuiMenu::openQuitMenu_static(mWindow, true);
				return true;
			}
			else if (config->isMappedTo("select", input) && Settings::getInstance()->getBool("ScreenSaverControls"))
			{
				std::string screensaver_behavior = Settings::getInstance()->getString("ScreenSaverBehavior");
				if (mWindow->isScreenSaverEnabled() && (screensaver_behavior != "suspend"))
				{
					mWindow->startScreenSaver();
					mWindow->renderScreenSaver(true);
				}
				return true;
			}
		}
	}
	else
	{
		if (config->isMappedLike("left", input) ||
				config->isMappedLike("right", input) ||
				config->isMappedLike("up", input) ||
				config->isMappedLike("down", input) ||
				config->isMappedTo(BUTTON_PD, input) ||
				config->isMappedTo(BUTTON_PU, input))
			listInput(0);
		
		Scripting::fireEvent("system-select", getSelected()->getName(), "input");
	}

	return GuiComponent::input(config, input);
}

void SystemView::showNavigationBar(const std::string& title, const std::function<std::string(SystemData* system)>& selector)
{
	stopScrolling();

	GuiSettings* gs = new GuiSettings(mWindow, title);

	int idx = 0;
	std::string sel = selector(getSelected());

// TO DELETE
//	LOG(LogDebug) << "SystemView::showNavigationBar() - Ordered Systems:";
//	int i=0;
//	for (auto sy : SystemData::sSystemVector)
//	{
//		LOG(LogDebug) << "SystemView::showNavigationBar() -     position: " << std::to_string(i) << ", system: " << sy->getName() << ", full name: " << sy->getFullName();
//		i++;
//	}
//	LOG(LogDebug) << "SystemView::showNavigationBar() - END Ordered Systems.";
// END - TO DELETE

	std::string man = "*-*";
	for (int i = 0; i < SystemData::sSystemVector.size(); i++)
	{
//		LOG(LogDebug) << "SystemView::showNavigationBar() - actual man: " << man;
		auto system = SystemData::sSystemVector[i];
		if (!system->isVisible())
			continue;

		auto mf = selector(system);
//		LOG(LogDebug) << "SystemView::showNavigationBar() - sel: " << sel << ", i: " << std::to_string(i) << ", system: " << system->getName() << ", mf: " << mf << ", man: " << man;
		if (man != mf)
		{
			std::vector<std::string> names;
			for (auto sy : SystemData::sSystemVector)
			{
				if (sy->isVisible() && selector(sy) == mf)
				{
					std::string name = sy->getFullName();
					if (system->isCollection())
						name = Utils::String::startWithUpper( _(name) );

					names.push_back(name);
				}
			}
//				LOG(LogDebug) << "SystemView::showNavigationBar() - ordenation ==> mf: " << mf << ", position: " << std::to_string(idx) << ", system: " << system->getName() << ", names: " << Utils::String::join(names, ", ");

			gs->getMenu().addWithDescription(_(Utils::String::toUpper(mf)), Utils::String::join(names, ", "), nullptr, [this, gs, system, idx]
			{
				listInput(idx - mCursor);
				listInput(0);

				auto pthis = this;

				delete gs;

				pthis->mLastCursor = -1;
				pthis->onCursorChanged(CURSOR_STOPPED);

			}, "", sel == mf);

			man = mf;
//			LOG(LogDebug) << "SystemView::showNavigationBar() - new man: " << man;
		}

		idx++;
	}

	float w = Math::min(Renderer::getScreenWidth() * 0.5, ThemeData::getMenuTheme()->Text.font->getLetterWidth() * 31.0f);
	w = Math::max(w, Renderer::getScreenWidth() / 3.0f);

	// resize
	float height_ratio = 1.0f;
	if ( Settings::getInstance()->getBool("ShowHelpPrompts") && !Renderer::isSmallScreen() )
		height_ratio = 0.93f;

	gs->getMenu().setSize(w, Renderer::getScreenHeight() * height_ratio);

	gs->getMenu().animateTo(
		Vector2f(-w, 0),
		Vector2f(0, 0), AnimateFlags::OPACITY | AnimateFlags::POSITION);

	mWindow->pushGui(gs);
}

void SystemView::update(int deltaTime)
{
	listUpdate(deltaTime);
	updateExtras([this, deltaTime](GuiComponent* p) { p->update(deltaTime); });
	GuiComponent::update(deltaTime);
}

void SystemView::updateExtraTextBinding()
{
	if (mCursor < 0 || mCursor >= mEntries.size())
		return;

	GameCountInfo* info = getSelected()->getGameCountInfo();

	for (auto extra : mEntries[mCursor].data.backgroundExtras)
	{
		TextComponent* text = dynamic_cast<TextComponent*>(extra);
		if (text == nullptr)
			continue;

		auto src = text->getOriginalThemeText();
		if (src.find("{binding:") == std::string::npos)
			continue;

		if (info->totalGames != info->visibleGames)
			src = Utils::String::replace(src, "{binding:total}", std::to_string(info->visibleGames) + " / " + std::to_string(info->totalGames));
		else
			src = Utils::String::replace(src, "{binding:total}", std::to_string(info->totalGames));

		if (info->playCount == 0)
			src = Utils::String::replace(src, "{binding:played}", _("None"));
		else
			src = Utils::String::replace(src, "{binding:played}", std::to_string(info->playCount));

		if (info->favoriteCount == 0)
			src = Utils::String::replace(src, "{binding:favorites}", _("None"));
		else
			src = Utils::String::replace(src, "{binding:favorites}", std::to_string(info->favoriteCount));

		if (info->hiddenCount == 0)
			src = Utils::String::replace(src, "{binding:hidden}", _("None"));
		else
			src = Utils::String::replace(src, "{binding:hidden}", std::to_string(info->hiddenCount));

		if (info->gamesPlayed == 0)
			src = Utils::String::replace(src, "{binding:gamesPlayed}", _("None"));
		else
			src = Utils::String::replace(src, "{binding:gamesPlayed}", std::to_string(info->gamesPlayed));

		if (info->mostPlayed.empty())
			src = Utils::String::replace(src, "{binding:mostPlayed}", _("Unknown"));
		else
			src = Utils::String::replace(src, "{binding:mostPlayed}", info->mostPlayed);

		Utils::Time::DateTime dt = info->lastPlayedDate;

		if (dt.getTime() == 0)
			src = Utils::String::replace(src, "{binding:lastPlayedDate}", _("Unknown"));
		else
		{
			time_t     clockNow = dt.getTime();
			struct tm  clockTstruct = *localtime(&clockNow);

			char       clockBuf[256];
			strftime(clockBuf, sizeof(clockBuf), "%Ex", &clockTstruct);

			src = Utils::String::replace(src, "{binding:lastPlayedDate}", clockBuf);
		}

		text->setText(src);
	}
}

void SystemView::onCursorChanged(const CursorState& /*state*/)
{
	if (mLastSystem != getSelected())
	{
		mLastSystem = getSelected();
		if (AudioManager::isInitialized())
			AudioManager::getInstance()->changePlaylist(getSelected()->getTheme());
	}

	// update help style
	updateHelpPrompts();

	float startPos = mCamOffset;

	float posMax = (float)mEntries.size();
	float target = (float)mCursor;

	// what's the shortest way to get to our target?
	// it's one of these...

	float endPos = target; // directly
	float dist = abs(endPos - startPos);

	if(abs(target + posMax - startPos) < dist)
		endPos = target + posMax; // loop around the end (0 -> max)
	if(abs(target - posMax - startPos) < dist)
		endPos = target - posMax; // loop around the start (max - 1 -> -1)

	// animate mSystemInfo's opacity (fade out, wait, fade back in)

	cancelAnimation(1);
	cancelAnimation(2);

	std::string transition_style = Settings::getInstance()->getString("TransitionStyle");
	if (transition_style == "auto")
	{
		if (mCarousel.defaultTransition == "instant" || mCarousel.defaultTransition == "fade" || mCarousel.defaultTransition == "slide")
			transition_style = mCarousel.defaultTransition;
		else
			transition_style = "slide";
	}

	int systemInfoDelay = mCarousel.systemInfoDelay;

	bool goFast = transition_style == "instant" || systemInfoDelay == 0;
	const float infoStartOpacity = mSystemInfo.getOpacity() / 255.f;

	Animation* infoFadeOut = new LambdaAnimation(
		[infoStartOpacity, this] (float t)
	{
		mSystemInfo.setOpacity((unsigned char)(Math::lerp(infoStartOpacity, 0.f, t) * 255));
	}, (int)(infoStartOpacity * (goFast ? 10 : 150)));

	unsigned int gameCount = getSelected()->getGameCountInfo()->visibleGames;

	updateExtraTextBinding();

	// also change the text after we've fully faded out
	setAnimation(infoFadeOut, 0, [this, gameCount]
	{
		if (!getSelected()->isGameSystem() && !getSelected()->isGroupSystem())
			mSystemInfo.setText(_("CONFIGURATION"));
		else if (mCarousel.systemInfoCountOnly)
			mSystemInfo.setText(std::to_string(gameCount));
		else
		{
			std::stringstream ss;
			char strbuf[256];

			if (getSelected() == CollectionSystemManager::get()->getCustomCollectionsBundle())
			{
				int collectionCount = getSelected()->getRootFolder()->getChildren().size();
				snprintf(strbuf, 256, EsLocale::nGetText("%i COLLECTION", "%i COLLECTIONS", collectionCount).c_str(), collectionCount);
			}
			else if (getSelected()->hasPlatformId(PlatformIds::PLATFORM_IGNORE) && !getSelected()->isCollection())
				snprintf(strbuf, 256, EsLocale::nGetText("%i ITEM", "%i ITEMS", gameCount).c_str(), gameCount);
			else
				snprintf(strbuf, 256, EsLocale::nGetText("%i GAME", "%i GAMES", gameCount).c_str(), gameCount);

			ss << strbuf;
			mSystemInfo.setText(ss.str());
		}

		mSystemInfo.onShow();
	}, false, 1);

	Animation* infoFadeIn = new LambdaAnimation(
		[this](float t)
	{
		mSystemInfo.setOpacity((unsigned char)(Math::lerp(0.f, 1.f, t) * 255));
	}, goFast ? 10 : 300);

	// wait 600ms to fade in
	setAnimation(infoFadeIn, goFast ? 0 : systemInfoDelay, nullptr, false, 2);
	// fake preload
	//setAnimation(infoFadeIn, goFast ? 0 : systemInfoDelay, [this] { ViewController::get()->getGameListView(mEntries.at(mCursor).object); }, false, 2);

	// no need to animate transition, we're not going anywhere (probably mEntries.size() == 1)
	if(endPos == mCamOffset && endPos == mExtrasCamOffset)
		return;

	if (mLastCursor == mCursor)
		return;

	if (!mCarousel.scrollSound.empty())
		Sound::get(mCarousel.scrollSound)->play();

	int oldCursor = mLastCursor;
	mLastCursor = mCursor;

	Animation* anim;
	bool move_carousel = Settings::getInstance()->getBool("MoveCarousel");
	if(transition_style == "fade")
	{
		float startExtrasFade = mExtrasFadeOpacity;
		anim = new LambdaAnimation(
			[this, startExtrasFade, startPos, endPos, posMax, move_carousel, oldCursor](float t)
		{
			mExtrasFadeOldCursor = oldCursor;

			t -= 1;
			float f = Math::lerp(startPos, endPos, t*t*t + 1);
			if(f < 0)
				f += posMax;
			if(f >= posMax)
				f -= posMax;

			this->mCamOffset = move_carousel ? f : endPos;

			t += 1;
			/*
			if(t < 0.3f)
				this->mExtrasFadeOpacity = Math::lerp(0.0f, 1.0f, t / 0.3f + startExtrasFade);
			else if(t < 0.7f)
				this->mExtrasFadeOpacity = 1.0f;
			else
				this->mExtrasFadeOpacity = Math::lerp(1.0f, 0.0f, (t - 0.7f) / 0.3f);
				*/

			if (t < 0.3f)
				this->mExtrasFadeOpacity = 1.0f;
			else if (t >= 0.7f)
				this->mExtrasFadeOpacity = 0.0f;
			else
				this->mExtrasFadeOpacity = Math::lerp(1.0f, 0.0f, (t - 0.3f) / 0.4f);

			this->mExtrasCamOffset = endPos;

		}, 500);
	} else if (transition_style == "slide") {
		// slide
		anim = new LambdaAnimation(
			[this, startPos, endPos, posMax, move_carousel](float t)
		{
			t -= 1;
			float f = Math::lerp(startPos, endPos, t*t*t + 1);
			if(f < 0)
				f += posMax;
			if(f >= posMax)
				f -= posMax;

			this->mCamOffset = move_carousel ? f : endPos;
			this->mExtrasCamOffset = f;
		}, 500);
	} else {
		// instant
		anim = new LambdaAnimation(
			[this, startPos, endPos, posMax, move_carousel ](float t)
		{
			t -= 1;
			float f = Math::lerp(startPos, endPos, t*t*t + 1);
			if(f < 0)
				f += posMax;
			if(f >= posMax)
				f -= posMax;

			this->mCamOffset = move_carousel ? f : endPos;
			this->mExtrasCamOffset = endPos;
		}, move_carousel ? 500 : 1);
	}

	for (int i = 0; i < mEntries.size(); i++)
		if (i != oldCursor && i != mCursor)
			activateExtras(i, false);

	activateExtras(mCursor);

	setAnimation(anim, 0, [this]
	{
		mExtrasFadeOldCursor = -1;

		for (int i = 0; i < mEntries.size(); i++)
			if (i != mCursor)
				activateExtras(i, false);

	}, false, 0);
}

void SystemView::render(const Transform4x4f& parentTrans)
{
	if (size() == 0 || !isVisible())
		return;  // nothing to render

	Transform4x4f trans = getTransform() * parentTrans;

	if (!Renderer::isVisibleOnScreen(trans.translation().x(), trans.translation().y(), mSize.x(), mSize.y()))
		return;

	auto systemInfoZIndex = mSystemInfo.getZIndex();
	auto minMax = std::minmax(mCarousel.zIndex, systemInfoZIndex);

	renderExtras(trans, INT16_MIN, minMax.first);
	// renderFade(trans);

	if (mStaticBackground != nullptr)
		mStaticBackground->render(trans);

	if (mStaticVideoBackground != nullptr)
		mStaticVideoBackground->render(trans);

	if (mCarousel.zIndex > mSystemInfo.getZIndex()) {
		renderInfoBar(trans);
	} else {
		renderCarousel(trans);
	}

	renderExtras(trans, minMax.first, minMax.second);

	if (mCarousel.zIndex > mSystemInfo.getZIndex()) {
		renderCarousel(trans);
	} else {
		renderInfoBar(trans);
	}

	renderExtras(trans, minMax.second, INT16_MAX);
}

std::vector<HelpPrompt> SystemView::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts;
	if (mCarousel.type == VERTICAL || mCarousel.type == VERTICAL_WHEEL)
		prompts.push_back(HelpPrompt("up/down", _("CHOOSE")));
	else
		prompts.push_back(HelpPrompt("left/right", _("CHOOSE")));

	prompts.push_back(HelpPrompt(BUTTON_OK, _("SELECT")));
	prompts.push_back(HelpPrompt("x", _("RANDOM")));
	if (SystemData::getSystem("all") != nullptr)
		prompts.push_back(HelpPrompt("y", _("SEARCH"))); // QUICK

	if (Settings::getInstance()->getBool("ShowQuitMenuWithSelect"))
	{
		prompts.push_back(HelpPrompt("select", _("QUIT MENU")));
	}
	else
	{
		std::string screensaver_behavior = Settings::getInstance()->getString("ScreenSaverBehavior");
		if (!UIModeController::getInstance()->isUIModeKid() && Settings::getInstance()->getBool("ScreenSaverControls") && ((screensaver_behavior != "suspend") && mWindow->isScreenSaverEnabled()))
			prompts.push_back(HelpPrompt("select", _("LAUNCH SCREENSAVER")));
	}

	return prompts;
}

HelpStyle SystemView::getHelpStyle()
{
	HelpStyle style;
	style.applyTheme(mEntries.at(mCursor).object->getTheme(), "system");
	return style;
}

void  SystemView::onThemeChanged(const std::shared_ptr<ThemeData>& /*theme*/)
{
	//LOG(LogDebug) << "SystemView::onThemeChanged()";
	mViewNeedsReload = true;
	populate();
}

//  Get the ThemeElements that make up the SystemView.
void  SystemView::getViewElements(const std::shared_ptr<ThemeData>& theme)
{
	//LOG(LogDebug) << "SystemView::getViewElements()";

	getDefaultElements();

	if (!theme->hasView("system"))
		return;

	const ThemeData::ThemeElement* carouselElem = theme->getElement("system", "systemcarousel", "carousel");
	if (carouselElem)
		getCarouselFromTheme(carouselElem);

	const ThemeData::ThemeElement* sysInfoElem = theme->getElement("system", "systemInfo", "text");
	if (sysInfoElem)
	{
		mSystemInfo.applyTheme(theme, "system", "systemInfo", ThemeFlags::ALL);
		mSystemInfo.setOpacity(0);
	}

	const ThemeData::ThemeElement* fixedBackgroundElem = theme->getElement("system", "staticBackground", "image");
	if (fixedBackgroundElem)
	{
		if (mStaticBackground == nullptr)
			mStaticBackground = new ImageComponent(mWindow, false);

		mStaticBackground->applyTheme(theme, "system", "staticBackground", ThemeFlags::ALL);
	}
	else if (mStaticBackground != nullptr)
	{
		delete mStaticBackground;
		mStaticBackground = nullptr;
	}

	const ThemeData::ThemeElement* fixedVideoBackgroundElem = theme->getElement("system", "staticBackgroundVideo", "video");
	if (fixedVideoBackgroundElem && (!fixedVideoBackgroundElem->has("visible") || fixedVideoBackgroundElem->get<bool>("visible")))
	{		
		if (mStaticVideoBackground == nullptr)
			mStaticVideoBackground = new VideoVlcComponent(mWindow);

		mStaticVideoBackground->applyTheme(theme, "system", "staticBackgroundVideo", ThemeFlags::ALL);
	}
	else if (mStaticBackground != nullptr)
	{
		delete mStaticVideoBackground;
		mStaticVideoBackground = nullptr;
	}
	

	mViewNeedsReload = false;
}

//  Render system carousel
void SystemView::renderCarousel(const Transform4x4f& trans)
{
	// background box behind logos
	Transform4x4f carouselTrans = trans;
	carouselTrans.translate(Vector3f(mCarousel.pos.x(), mCarousel.pos.y(), 0.0));
	carouselTrans.translate(Vector3f(mCarousel.origin.x() * mCarousel.size.x() * -1, mCarousel.origin.y() * mCarousel.size.y() * -1, 0.0f));

	Vector2f clipPos(carouselTrans.translation().x(), carouselTrans.translation().y());
	Renderer::pushClipRect(Vector2i((int)clipPos.x(), (int)clipPos.y()), Vector2i((int)mCarousel.size.x(), (int)mCarousel.size.y()));

	Renderer::setMatrix(carouselTrans);
	Renderer::drawRect(0.0f, 0.0f, mCarousel.size.x(), mCarousel.size.y(), mCarousel.color, mCarousel.colorEnd, mCarousel.colorGradientHorizontal);

	// draw logos
	Vector2f logoSpacing(0.0, 0.0); // NB: logoSpacing will include the size of the logo itself as well!
	float xOff = 0.0;
	float yOff = 0.0;

	switch (mCarousel.type)
	{
		case VERTICAL_WHEEL:
			yOff = (mCarousel.size.y() - mCarousel.logoSize.y()) / 2.f - (mCamOffset * logoSpacing[1]);
			if (mCarousel.logoAlignment == ALIGN_LEFT)
				xOff = mCarousel.logoSize.x() / 10.f;
			else if (mCarousel.logoAlignment == ALIGN_RIGHT)
				xOff = mCarousel.size.x() - (mCarousel.logoSize.x() * 1.1f);
			else
				xOff = (mCarousel.size.x() - mCarousel.logoSize.x()) / 2.f;
			break;
		case VERTICAL:
			logoSpacing[1] = ((mCarousel.size.y() - (mCarousel.logoSize.y() * mCarousel.maxLogoCount)) / (mCarousel.maxLogoCount)) + mCarousel.logoSize.y();
			yOff = (mCarousel.size.y() - mCarousel.logoSize.y()) / 2.f - (mCamOffset * logoSpacing[1]);

			if (mCarousel.logoAlignment == ALIGN_LEFT)
				xOff = mCarousel.logoSize.x() / 10.f;
			else if (mCarousel.logoAlignment == ALIGN_RIGHT)
				xOff = mCarousel.size.x() - (mCarousel.logoSize.x() * 1.1f);
			else
				xOff = (mCarousel.size.x() - mCarousel.logoSize.x()) / 2;
			break;
		case HORIZONTAL_WHEEL:
			xOff = (mCarousel.size.x() - mCarousel.logoSize.x()) / 2 - (mCamOffset * logoSpacing[1]);
			if (mCarousel.logoAlignment == ALIGN_TOP)
				yOff = mCarousel.logoSize.y() / 10;
			else if (mCarousel.logoAlignment == ALIGN_BOTTOM)
				yOff = mCarousel.size.y() - (mCarousel.logoSize.y() * 1.1f);
			else
				yOff = (mCarousel.size.y() - mCarousel.logoSize.y()) / 2;
			break;
		case HORIZONTAL:
		default:
			logoSpacing[0] = ((mCarousel.size.x() - (mCarousel.logoSize.x() * mCarousel.maxLogoCount)) / (mCarousel.maxLogoCount)) + mCarousel.logoSize.x();
			xOff = (mCarousel.size.x() - mCarousel.logoSize.x()) / 2.f - (mCamOffset * logoSpacing[0]);

			if (mCarousel.logoAlignment == ALIGN_TOP)
				yOff = mCarousel.logoSize.y() / 10.f;
			else if (mCarousel.logoAlignment == ALIGN_BOTTOM)
				yOff = mCarousel.size.y() - (mCarousel.logoSize.y() * 1.1f);
			else
				yOff = (mCarousel.size.y() - mCarousel.logoSize.y()) / 2.f;
			break;
	}

	if (mCarousel.logoPos.x() >= 0)
		xOff = mCarousel.logoPos.x() - (mCarousel.type == HORIZONTAL ? (mCamOffset * logoSpacing[0]) : 0);

	if (mCarousel.logoPos.y() >= 0)
		yOff = mCarousel.logoPos.y() - (mCarousel.type == VERTICAL ? (mCamOffset * logoSpacing[1]) : 0);

	int center = (int)(mCamOffset);
	int logoCount = Math::min(mCarousel.maxLogoCount, (int)mEntries.size());

	// Adding texture loading buffers depending on scrolling speed and status
	int bufferIndex = getScrollingVelocity() + 1;
	int bufferLeft = logoBuffersLeft[bufferIndex];
	int bufferRight = logoBuffersRight[bufferIndex];
	if (logoCount == 1 && mCamOffset == 0)
	{
		bufferLeft = 0;
		bufferRight = 0;
	}

	for (int i = center - logoCount / 2 + bufferLeft; i <= center + logoCount / 2 + bufferRight; i++)
	{
		int index = i;
		while (index < 0)
			index += (int)mEntries.size();
		while (index >= (int)mEntries.size())
			index -= (int)mEntries.size();

		Transform4x4f logoTrans = carouselTrans;
		logoTrans.translate(Vector3f(i * logoSpacing[0] + xOff, i * logoSpacing[1] + yOff, 0));

		float distance = i - mCamOffset;

		float scale = 1.0f + ((mCarousel.logoScale - 1.0f) * (1.0f - fabs(distance)));
		scale = Math::min(mCarousel.logoScale, Math::max(1.0f, scale));
		scale /= mCarousel.logoScale;

		int opacity = (int)Math::round(0x80 + ((0xFF - 0x80) * (1.0f - fabs(distance))));
		opacity = Math::max((int) 0x80, opacity);

		const std::shared_ptr<GuiComponent> &comp = mEntries.at(index).data.logo;
		if (mCarousel.type == VERTICAL_WHEEL || mCarousel.type == HORIZONTAL_WHEEL) {
			comp->setRotationDegrees(mCarousel.logoRotation * distance);
			comp->setRotationOrigin(mCarousel.logoRotationOrigin);
		}
		comp->setScale(scale);
		comp->setOpacity((unsigned char)opacity);
		comp->render(logoTrans);
	}
	Renderer::popClipRect();
}

void SystemView::renderInfoBar(const Transform4x4f& trans)
{
	Renderer::setMatrix(trans);
	mSystemInfo.render(trans);
}

#include <unordered_set>

// Draw background extras
void SystemView::renderExtras(const Transform4x4f& trans, float lower, float upper)
{
	int extrasCenter = (int)mExtrasCamOffset;

	// Adding texture loading buffers depending on scrolling speed and status
	int bufferIndex = getScrollingVelocity() + 1;

	Renderer::pushClipRect(Vector2i::Zero(), Vector2i((int)mSize.x(), (int)mSize.y()));
	
	std::unordered_set<std::string> allPaths;
	std::unordered_set<std::string> paths;
	std::unordered_set<std::string> allValues;
	std::unordered_set<std::string> values;

	if (mExtrasFadeOpacity && mExtrasFadeOldCursor >= 0 && mExtrasFadeOldCursor < mEntries.size() && mExtrasFadeOldCursor != mCursor)
	{
		// ExtrasFadeOpacity : Collect images paths & text values		
		// paths & values must have only the elements that are not common 
		if (mCursor >= 0 && mCursor < mEntries.size())
		{
			for (GuiComponent* extra : mEntries.at(mCursor).data.backgroundExtras)
			{
				if (extra->getZIndex() < lower || extra->getZIndex() >= upper)
					continue;
				
				if (extra->isStaticExtra())
					continue;

				std::string value = extra->getValue();
				if (extra->isKindOf<ImageComponent>())
					paths.insert(value);
				else if (extra->isKindOf<TextComponent>())
					values.insert(value);
			}

			allValues = values;
			allPaths = paths;

			for (GuiComponent* extra : mEntries.at(mExtrasFadeOldCursor).data.backgroundExtras)
			{
				if (extra->getZIndex() < lower || extra->getZIndex() >= upper)
					continue;

				if (extra->isStaticExtra())
					continue;

				std::string value = extra->getValue();
				if (extra->isKindOf<ImageComponent>())
					paths.erase(value);
				else if (extra->isKindOf<TextComponent>())
					values.erase(value);
			}
		}

		Renderer::pushClipRect(Vector2i((int)trans.translation()[0], (int)trans.translation()[1]), Vector2i((int)mSize.x(), (int)mSize.y()));

		// ExtrasFadeOpacity : Render only items with different paths or values
		for (GuiComponent* extra : mEntries.at(mExtrasFadeOldCursor).data.backgroundExtras)
		{
			if (extra->getZIndex() < lower || extra->getZIndex() >= upper)
				continue;
			
			if (extra->isStaticExtra())
				continue;

			std::string value = extra->getValue();
			if (extra->isKindOf<ImageComponent>())
			{
				if (allPaths.find(value) == allPaths.cend())
					extra->render(trans);
				else if (((ImageComponent*)extra)->isTiled() && extra->getPosition() == Vector3f::Zero() && extra->getSize() == Vector2f(Renderer::getScreenWidth(), Renderer::getScreenHeight()))
					extra->render(trans);
			}
			else if (extra->isKindOf<TextComponent>() && allValues.find(value) == allValues.cend())
				extra->render(trans);
		}

		Renderer::popClipRect();
	}

	for (int i = extrasCenter + logoBuffersLeft[bufferIndex]; i <= extrasCenter + logoBuffersRight[bufferIndex]; i++)
	{
		int index = i;
		while (index < 0)
			index += (int)mEntries.size();
		while (index >= (int)mEntries.size())
			index -= (int)mEntries.size();

		if (mExtrasFadeOpacity && (index == mExtrasFadeOldCursor || index != mCursor))
			continue;

		//Only render selected system when not showing
		if (!isShowing() && index != mCursor)
			continue;

		Entry& entry = mEntries.at(index);

		Vector2i size = Vector2i(Math::round(mSize.x()), Math::round(mSize.y()));

		Transform4x4f extrasTrans = trans;
		if (mCarousel.type == HORIZONTAL || mCarousel.type == HORIZONTAL_WHEEL)
		{
			extrasTrans.translate(Vector3f((i - mExtrasCamOffset) * mSize.x(), 0, 0));

			if (extrasTrans.translation()[0] >= 0 && extrasTrans.translation()[0] <= Renderer::getScreenWidth() && extrasTrans.translation()[0] + mSize.x() > Renderer::getScreenWidth())
				size.x() = Renderer::getScreenWidth() - extrasTrans.translation()[0];
		}
		else
		{
			extrasTrans.translate(Vector3f(0, (i - mExtrasCamOffset) * mSize.y(), 0));

			if (extrasTrans.translation()[1] >= 0 && extrasTrans.translation()[1] <= Renderer::getScreenHeight() && extrasTrans.translation()[1] + mSize.y() > Renderer::getScreenHeight())
				size.y() = Renderer::getScreenHeight() - extrasTrans.translation()[1];
		}

		if (!Renderer::isVisibleOnScreen(extrasTrans.translation()[0], extrasTrans.translation()[1], mSize.x(), mSize.y()))
			continue;

		if (mExtrasFadeOpacity && mExtrasFadeOldCursor == index)
			extrasTrans = trans;

		Renderer::pushClipRect(Vector2i(Math::round(extrasTrans.translation()[0]), Math::round(extrasTrans.translation()[1])), Vector2i(Math::round(size.x()), Math::round(size.y())));

		for (GuiComponent* extra : mEntries.at(index).data.backgroundExtras)
		{
			if (extra->getZIndex() < lower || extra->getZIndex() >= upper)
				continue;

			// ExtrasFadeOpacity : Apply opacity only on elements that are not common with the original view
			if (mExtrasFadeOpacity && !extra->isStaticExtra())
			{
				std::string value = extra->getValue();
				if (extra->isKindOf<ImageComponent>())
				{
					if (paths.find(value) != paths.cend())
					{
						auto opa = extra->getOpacity();
						extra->setOpacity((1.0f - mExtrasFadeOpacity) * opa);
						extra->render(extra->isStaticExtra() ? trans : extrasTrans);
						extra->setOpacity(opa);
						continue;
					}
					else if (((ImageComponent*)extra)->isTiled() && extra->getPosition() == Vector3f::Zero() && extra->getSize() == Vector2f(Renderer::getScreenWidth(), Renderer::getScreenHeight()))
					{
						auto opa = extra->getOpacity();
						extra->setOpacity((1.0f - mExtrasFadeOpacity) * opa);
						extra->render(extra->isStaticExtra() ? trans : extrasTrans);
						extra->setOpacity(opa);
						continue;
					}
				}
				else if (extra->isKindOf<TextComponent>() && values.find(value) != values.cend())
				{
					auto opa = extra->getOpacity();
					extra->setOpacity((1.0f - mExtrasFadeOpacity) * opa);
					extra->render(extra->isStaticExtra() ? trans : extrasTrans);
					extra->setOpacity(opa);
					continue;
				}
			}

			if (extra->isStaticExtra())
			{
				bool popClip = false;

				if (extrasTrans.translation()[0] > Renderer::getScreenWidth())
					continue;
				else if (extrasTrans.translation()[1] > Renderer::getScreenHeight())
					continue;
				else if (extrasTrans.translation()[0] < 0)
				{
					int x = Math::round(size.x() + extrasTrans.translation()[0]);
					if (x == 0)
						continue;

					Renderer::pushClipRect(Vector2i(0, Math::round(extrasTrans.translation()[1])), Vector2i(x, Math::round(size.y())));
					popClip = true;
				}
				else if (extrasTrans.translation()[1] < 0)
				{
					int y = Math::round(size.y() + extrasTrans.translation()[1]);
					if (y == 0)
						continue;

					Renderer::pushClipRect(Vector2i(Math::round(extrasTrans.translation()[0]), 0), Vector2i(Math::round(size.x()), y));
					popClip = true;
				}

				extra->render(trans);

				if (popClip)
					Renderer::popClipRect();
			}
			else
				extra->render(extrasTrans);
		}

		Renderer::popClipRect();
	}

	Renderer::popClipRect();
}

void SystemView::renderFade(const Transform4x4f& trans)
{
	// fade extras if necessary
	if (mExtrasFadeOpacity)
	{
		unsigned int fadeColor = 0x00000000 | (unsigned char)(mExtrasFadeOpacity * 255);
		Renderer::setMatrix(trans);
		//Renderer::drawRect(0.0f, 0.0f, mSize.x(), mSize.y(), fadeColor, fadeColor);
	}
}

// Populate the system carousel with the legacy values
void  SystemView::getDefaultElements(void)
{
	// Carousel
	mCarousel.type = HORIZONTAL;
	mCarousel.logoAlignment = ALIGN_CENTER;
	mCarousel.size.x() = mSize.x();
	mCarousel.size.y() = 0.2325f * mSize.y();
	mCarousel.pos.x() = 0.0f;
	mCarousel.pos.y() = 0.5f * (mSize.y() - mCarousel.size.y());
	mCarousel.origin.x() = 0.0f;
	mCarousel.origin.y() = 0.0f;
	mCarousel.color = 0xFFFFFFD8;
	mCarousel.colorEnd = 0xFFFFFFD8;
	mCarousel.colorGradientHorizontal = true;
	mCarousel.logoScale = 1.2f;
	mCarousel.logoRotation = 7.5;
	mCarousel.logoRotationOrigin.x() = -5;
	mCarousel.logoRotationOrigin.y() = 0.5;
	mCarousel.logoSize.x() = 0.25f * mSize.x();
	mCarousel.logoSize.y() = 0.155f * mSize.y();
	mCarousel.logoPos = Vector2f(-1, -1);
	mCarousel.maxLogoCount = 3;
	mCarousel.zIndex = 40;
	mCarousel.systemInfoDelay = 2000;
	mCarousel.systemInfoCountOnly = false;
	mCarousel.scrollSound = "";
	mCarousel.defaultTransition = "";

	// System Info Bar
	mSystemInfo.setSize(mSize.x(), mSystemInfo.getFont()->getLetterHeight()*2.2f);
	mSystemInfo.setPosition(0, (mCarousel.pos.y() + mCarousel.size.y() - 0.2f));
	mSystemInfo.setBackgroundColor(0xDDDDDDD8);
	mSystemInfo.setRenderBackground(true);
	mSystemInfo.setFont(Font::get((int)(0.035f * mSize.y()), Font::getDefaultPath()));
	mSystemInfo.setColor(0x000000FF);
	mSystemInfo.setZIndex(50);
	mSystemInfo.setDefaultZIndex(50);

	if (mStaticBackground != nullptr)
	{
		delete mStaticBackground;
		mStaticBackground = nullptr;
	}

	if (mStaticVideoBackground != nullptr)
	{
		delete mStaticVideoBackground;
		mStaticVideoBackground = nullptr;
	}
}

void SystemView::getCarouselFromTheme(const ThemeData::ThemeElement* elem)
{
	if (elem->has("type"))
	{
		if (!(elem->get<std::string>("type").compare("vertical")))
			mCarousel.type = VERTICAL;
		else if (!(elem->get<std::string>("type").compare("vertical_wheel")))
			mCarousel.type = VERTICAL_WHEEL;
		else if (!(elem->get<std::string>("type").compare("horizontal_wheel")))
			mCarousel.type = HORIZONTAL_WHEEL;
		else
			mCarousel.type = HORIZONTAL;
	}
	if (elem->has("size"))
		mCarousel.size = elem->get<Vector2f>("size") * mSize;
	if (elem->has("pos"))
		mCarousel.pos = elem->get<Vector2f>("pos") * mSize;
	if (elem->has("origin"))
		mCarousel.origin = elem->get<Vector2f>("origin");
	if (elem->has("color"))
	{
		mCarousel.color = elem->get<unsigned int>("color");
		mCarousel.colorEnd = mCarousel.color;
	}
	if (elem->has("colorEnd"))
		mCarousel.colorEnd = elem->get<unsigned int>("colorEnd");
	if (elem->has("gradientType"))
		mCarousel.colorGradientHorizontal = elem->get<std::string>("gradientType").compare("horizontal");
	if (elem->has("logoScale"))
		mCarousel.logoScale = elem->get<float>("logoScale");
	if (elem->has("logoSize"))
		mCarousel.logoSize = elem->get<Vector2f>("logoSize") * mSize;
	if (elem->has("logoPos"))
		mCarousel.logoPos = elem->get<Vector2f>("logoPos") * mSize;
	if (elem->has("maxLogoCount"))
		mCarousel.maxLogoCount = (int)Math::round(elem->get<float>("maxLogoCount"));
	if (elem->has("zIndex"))
		mCarousel.zIndex = elem->get<float>("zIndex");
	if (elem->has("logoRotation"))
		mCarousel.logoRotation = elem->get<float>("logoRotation");
	if (elem->has("logoRotationOrigin"))
		mCarousel.logoRotationOrigin = elem->get<Vector2f>("logoRotationOrigin");
	if (elem->has("logoAlignment"))
	{
		if (!(elem->get<std::string>("logoAlignment").compare("left")))
			mCarousel.logoAlignment = ALIGN_LEFT;
		else if (!(elem->get<std::string>("logoAlignment").compare("right")))
			mCarousel.logoAlignment = ALIGN_RIGHT;
		else if (!(elem->get<std::string>("logoAlignment").compare("top")))
			mCarousel.logoAlignment = ALIGN_TOP;
		else if (!(elem->get<std::string>("logoAlignment").compare("bottom")))
			mCarousel.logoAlignment = ALIGN_BOTTOM;
		else
			mCarousel.logoAlignment = ALIGN_CENTER;
	}

	if (elem->has("systemInfoDelay"))
		mCarousel.systemInfoDelay = elem->get<float>("systemInfoDelay");

	if (elem->has("systemInfoCountOnly"))
		mCarousel.systemInfoCountOnly = elem->get<bool>("systemInfoCountOnly");

	if (elem->has("scrollSound"))
		mCarousel.scrollSound = elem->get<std::string>("scrollSound");

	if (elem->has("defaultTransition"))
		mCarousel.defaultTransition = elem->get<std::string>("defaultTransition");
}

void SystemView::onShow()
{
	GuiComponent::onShow();	

	activateExtras(mCursor);

	if (mStaticVideoBackground)
		mStaticVideoBackground->onShow();
}

void SystemView::onHide()
{
	GuiComponent::onHide();

	updateExtras([this](GuiComponent* p) { p->onHide(); });

	if (mStaticVideoBackground)
		mStaticVideoBackground->onHide();
}

void SystemView::onScreenSaverActivate()
{
	mScreensaverActive = true;
	updateExtras([this](GuiComponent* p) { p->onScreenSaverActivate(); });

	if (mStaticVideoBackground)
		mStaticVideoBackground->onScreenSaverActivate();
}

void SystemView::onScreenSaverDeactivate()
{
	mScreensaverActive = false;
	updateExtras([this](GuiComponent* p) { p->onScreenSaverDeactivate(); });

	if (mStaticVideoBackground)
		mStaticVideoBackground->onScreenSaverDeactivate();
}

void SystemView::topWindow(bool isTop)
{
	mDisable = !isTop;
	updateExtras([this, isTop](GuiComponent* p) { p->topWindow(isTop); });

	if (mStaticVideoBackground)
		mStaticVideoBackground->topWindow(isTop);
}

void SystemView::updateExtras(const std::function<void(GuiComponent*)>& func)
{
	for (int i = 0; i < mEntries.size(); i++)
	{
		SystemViewData data = mEntries.at(i).data;
		for (unsigned int j = 0; j < data.backgroundExtras.size(); j++)
		{
			GuiComponent* extra = data.backgroundExtras[j];
			func(extra);
		}
	}
}

void SystemView::activateExtras(int cursor, bool activate)
{
	if (cursor < 0 || cursor >= mEntries.size())
		return;

	bool show = activate && isShowing() && !mScreensaverActive && !mDisable;

	SystemViewData data = mEntries.at(cursor).data;
	for (unsigned int j = 0; j < data.backgroundExtras.size(); j++)
	{
		GuiComponent *extra = data.backgroundExtras[j];
		if (show && activate)
			extra->onShow();
		else
			extra->onHide();
	}
}

SystemData* SystemView::getActiveSystem()
{
	if (mCursor < 0 || mCursor >= mEntries.size())
		return nullptr;

	return mEntries[mCursor].object;
}
