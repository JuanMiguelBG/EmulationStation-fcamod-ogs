#include "views/gamelist/ISimpleGameListView.h"

#include "views/UIModeController.h"
#include "views/ViewController.h"
#include "CollectionSystemManager.h"
#include "Scripting.h"
#include "Settings.h"
#include "Sound.h"
#include "SystemData.h"
#include "guis/GuiTextEditPopupKeyboard.h"
#include "guis/GuiTextEditPopup.h"
#include "Window.h"

ISimpleGameListView::ISimpleGameListView(Window* window, FolderData* root) : IGameListView(window, root),
	mHeaderText(window), mHeaderImage(window), mBackground(window), mYButton("y")
{
	mHeaderText.setText(_("Logo Text"));
	mHeaderText.setSize(mSize.x(), 0);
	mHeaderText.setPosition(0, 0);
	mHeaderText.setHorizontalAlignment(ALIGN_CENTER);
	mHeaderText.setDefaultZIndex(50);

	mHeaderImage.setResize(0, mSize.y() * 0.185f);
	mHeaderImage.setOrigin(0.5f, 0.0f);
	mHeaderImage.setPosition(mSize.x() / 2, 0);
	mHeaderImage.setDefaultZIndex(50);

	mBackground.setResize(mSize.x(), mSize.y());
	mBackground.setDefaultZIndex(0);

	addChild(&mHeaderText);
	addChild(&mBackground);
}

void ISimpleGameListView::onThemeChanged(const std::shared_ptr<ThemeData>& theme)
{
	using namespace ThemeFlags;
	mBackground.applyTheme(theme, getName(), "background", ALL);
	mHeaderImage.applyTheme(theme, getName(), "logo", ALL);
	mHeaderText.applyTheme(theme, getName(), "logoText", ALL);

	// Remove old theme extras
	for (auto extra : mThemeExtras)
	{
		removeChild(extra);
		delete extra;
	}
	mThemeExtras.clear();

	// Add new theme extras
	mThemeExtras = ThemeData::makeExtras(theme, getName(), mWindow);
	for (auto extra : mThemeExtras)
	{
		addChild(extra);
	}

	if(mHeaderImage.hasImage())
	{
		removeChild(&mHeaderText);
		addChild(&mHeaderImage);
	}
	else
	{
		addChild(&mHeaderText);
		removeChild(&mHeaderImage);
	}
}

void ISimpleGameListView::onFileChanged(FileData* /*file*/, FileChangeType /*change*/)
{
	// we could be tricky here to be efficient;
	// but this shouldn't happen very often so we'll just always repopulate
	FileData* cursor = getCursor();
	if (!cursor->isPlaceHolder())
	{
		populateList(cursor->getParent()->getChildrenListToDisplay());
		setCursor(cursor);
	}
	else
	{
		populateList(mRoot->getChildrenListToDisplay());
		setCursor(cursor);
	}
}

bool ISimpleGameListView::input(InputConfig* config, Input input)
{
	bool hideSystemView = Settings::getInstance()->getBool("HideSystemView");

	if (input.value != 0)
	{
		if (config->isMappedTo(BUTTON_OK, input))
		{
			// Don't launch game if transition is still running
			if (ViewController::get()->isAnimationPlaying(0))
				return true;
			
			FileData* cursor = getCursor();
			FolderData* folder = NULL;

			if (mCursorStack.size() && cursor->getType() == PLACEHOLDER && cursor->getPath() == "..")
			{
				auto top = mCursorStack.top();
				mCursorStack.pop();

				FolderData* folder = top->getParent();
				if (folder == nullptr)
					folder = getCursor()->getSystem()->getParentGroupSystem()->getRootFolder();

				populateList(folder->getChildrenListToDisplay());
				setCursor(top);
				Sound::getFromTheme(getTheme(), getName(), "back")->play();
			}
			else
			{
				if (cursor->getType() == FOLDER)
					folder = (FolderData*)cursor;

				if (cursor->getType() == GAME)
				{
					Sound::getFromTheme(getTheme(), getName(), "launch")->play();
					launch(cursor);
				}
				else {
					// it's a folder
					if (folder != nullptr && folder->getChildren().size() > 0)
					{
						mCursorStack.push(cursor);
						populateList(folder->getChildrenListToDisplay());
						FileData* cursor = getCursor();
						setCursor(cursor);
					}
				}
			}
			return true;
		}
		else if(config->isMappedTo(BUTTON_BACK, input))
		{
			if (mCursorStack.size())
			{
				auto top = mCursorStack.top();
				mCursorStack.pop();

				FolderData* folder = top->getParent();
				if (folder == nullptr && getCursor()->getSystem()->getParentGroupSystem() != nullptr)
					folder = getCursor()->getSystem()->getParentGroupSystem()->getRootFolder();

				if (folder == nullptr)
					return true;

				populateList(folder->getChildrenListToDisplay());
				setCursor(top);
				Sound::getFromTheme(getTheme(), getName(), "back")->play();
			}
			else if (mPopupSelfReference)
			{
				ViewController::get()->setActiveView(mPopupParentView);
				closePopupContext();
				return true;
			}
			else if (!hideSystemView)
			{
				onFocusLost();
				SystemData* systemToView = getCursor()->getSystem();

				if (systemToView->isGroupChildSystem())
					systemToView = systemToView->getParentGroupSystem();
				else if (systemToView->isCollection())
					systemToView = CollectionSystemManager::getInstance()->getSystemToView(systemToView);

				ViewController::get()->goToSystemView(systemToView);
			}

			return true;
		}
		else if (Settings::getInstance()->getBool("QuickSystemSelect") && ( config->isMappedLike(getQuickSystemSelectRightButton(), input) || config->isMappedLike(getDefaultQuickSystemSelectRightButton(), input) ))
		{
			if (!mPopupSelfReference)
			{
				onFocusLost();
				ViewController::get()->goToNextGameList();
			}

			return true;
		}
		else if (Settings::getInstance()->getBool("QuickSystemSelect") && ( config->isMappedLike(getQuickSystemSelectLeftButton(), input) || config->isMappedLike(getDefaultQuickSystemSelectLeftButton(), input) ))
		{
			if (!mPopupSelfReference)
			{
				onFocusLost();
				ViewController::get()->goToPrevGameList();
			}

			return true;
		}
		else if (config->isMappedTo("x", input))
		{
			if (mRoot->getSystem()->isGameSystem())
			{
				// go to random system game
				FileData* randomGame = getCursor()->getSystem()->getRandomGame();
				if (randomGame)
				{
					setCursor(randomGame);
				}
				return true;
			}
		}
	}

	if (mYButton.isShortPressed(config, input) && !UIModeController::getInstance()->isUIModeKid())
	{
		if (mRoot->getSystem()->isGameSystem() || mRoot->getSystem()->isGroupSystem())
			if (CollectionSystemManager::getInstance()->toggleGameInCollection(getCursor()))
				return true;
	}
	else
	{
		FileData* cursor = getCursor();
		SystemData* system = getCursor()->getSystem();
		if (system != NULL)
			Scripting::fireEvent("game-select", system->getName(), cursor->getPath(), cursor->getName(), "input");
	}
	return IGameListView::input(config, input);
}

std::vector<std::string> ISimpleGameListView::getEntriesLetters()
{
	std::set<std::string> setOfLetters;

	for (auto file : getFileDataEntries())
		if (file->getType() == GAME)
			setOfLetters.insert(std::string(1, toupper(file->getName()[0])));

	std::vector<std::string> letters;

	for (const auto letter : setOfLetters)
		letters.push_back(letter);

	std::sort(letters.begin(), letters.end());
	return letters;
}

void ISimpleGameListView::setPopupContext(std::shared_ptr<IGameListView> pThis, std::shared_ptr<GuiComponent> parentView, const std::string label, const std::function<void()>& onExitTemporary)
{
	mPopupSelfReference = pThis;
	mPopupParentView = parentView;
	mOnExitPopup = onExitTemporary;

	if (mHeaderImage.hasImage())
	{
		mHeaderText.setText(_("Games similar to") + " : " + label); //

		mHeaderImage.setImage("");
		addChild(&mHeaderText);
		removeChild(&mHeaderImage);
	}
}

void ISimpleGameListView::closePopupContext()
{
	if (!mPopupSelfReference)
		return;

	auto exitPopup = mOnExitPopup;

	mPopupParentView.reset();
	mPopupSelfReference.reset();

	if (exitPopup != nullptr)
		exitPopup();
}

std::string ISimpleGameListView::getDefaultQuickSystemSelectRightButton()
{
	if (Settings::getInstance()->getBool("InvertButtonsPD"))
		return BUTTON_R2;

	return BUTTON_R1;
}

std::string ISimpleGameListView::getDefaultQuickSystemSelectLeftButton()
{
	if (Settings::getInstance()->getBool("InvertButtonsPU"))
		return BUTTON_L2;

	return BUTTON_L1;
}

void ISimpleGameListView::update(const int deltaTime)
{
	GuiComponent::update(deltaTime);

	if (mYButton.isLongPressed(deltaTime))
		showQuickSearch();
}

void ISimpleGameListView::showQuickSearch()
{
//	if (mRoot->getSystem()->isCollection())
//		return;

	std::string searchText;

	auto idx = mRoot->getSystem()->getIndex(false);
	if (idx != nullptr)
		searchText = idx->getTextFilter();

	auto updateVal = [this](const std::string& newVal)
	{
		auto index = mRoot->getSystem()->getIndex(!newVal.empty());
		if (index != nullptr)
		{
			index->setTextFilter(newVal);
			if (!index->isFiltered())
				mRoot->getSystem()->deleteIndex();
		}

//		if (mRoot->getSystem()->isCollection())
//			CollectionSystemManager::get()->reloadCollection(mRoot->getSystem()->getName());
//		else
			ViewController::get()->reloadGameListView(mRoot->getSystem());
		
		return true;
	};

	if (Settings::getInstance()->getBool("UseOSK"))
		mWindow->pushGui(new GuiTextEditPopupKeyboard(mWindow, _("FILTER GAMES BY TEXT"), searchText, updateVal, false));
	else
		mWindow->pushGui(new GuiTextEditPopup(mWindow, _("FILTER GAMES BY TEXT"), searchText, updateVal, false));
}
