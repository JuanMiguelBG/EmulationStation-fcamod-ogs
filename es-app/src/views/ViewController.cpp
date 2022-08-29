#include "views/ViewController.h"

#include "animations/Animation.h"
#include "animations/LambdaAnimation.h"
#include "animations/LaunchAnimation.h"
#include "animations/MoveCameraAnimation.h"
#include "guis/GuiMenu.h"
#include "views/gamelist/DetailedGameListView.h"
#include "views/gamelist/IGameListView.h"
#include "views/gamelist/GridGameListView.h"
#include "views/gamelist/VideoGameListView.h"
#include "views/SystemView.h"
#include "views/UIModeController.h"
#include "FileFilterIndex.h"
#include "CollectionSystemManager.h"
#include "Log.h"
#include "Settings.h"
#include "SystemData.h"
#include "Window.h"
#include "AudioManager.h"
#include "utils/ThreadPool.h"
#include <mutex>
#include <SDL_timer.h>

ViewController* ViewController::sInstance = nullptr;

ViewController* ViewController::get()
{
	return sInstance;
}

void ViewController::deinit()
{
	if (sInstance != nullptr)
		delete sInstance;

	sInstance = nullptr;
}

void ViewController::init(Window* window)
{
	if (sInstance != nullptr)
		delete sInstance;

	sInstance = new ViewController(window);
}

void ViewController::saveState()
{
	if (sInstance == nullptr)
		return;

	if (Settings::getInstance()->getString("StartupSystem") != "lastsystem")
		return;

	SystemData* activeSystem = nullptr;

	if (sInstance->mState.viewing == GAME_LIST)
		activeSystem = sInstance->mState.getSystem();
	else if (sInstance->mState.viewing == SYSTEM_SELECT)
		activeSystem = sInstance->mSystemListView->getActiveSystem();

	if (activeSystem != nullptr)
	{
		if (Settings::getInstance()->setString("LastSystem", activeSystem->getName()))
			Settings::getInstance()->saveFile();
	}
}

ViewController::ViewController(Window* window)
	: GuiComponent(window), mCurrentView(nullptr), mCamera(Transform4x4f::Identity()), mFadeOpacity(0), mLockInput(false)
{
	mSystemListView = nullptr;
	mState.viewing = NOTHING;
	mState.system = nullptr;
}

ViewController::~ViewController()
{
	ISimpleGameListView* simpleView = dynamic_cast<ISimpleGameListView*>(mCurrentView.get());
	if (simpleView != nullptr)
		simpleView->closePopupContext();

	sInstance = nullptr;
}

void ViewController::goToStart(bool forceImmediate)
{
	bool hideSystemView = Settings::getInstance()->getBool("HideSystemView");
	bool startOnGamelist = Settings::getInstance()->getBool("StartupOnGameList");

	// If specific system is requested, go directly to the game list
	auto requestedSystem = Settings::getInstance()->getString("StartupSystem");
	if (requestedSystem == "lastsystem")
		requestedSystem = Settings::getInstance()->getString("LastSystem");

	if("" != requestedSystem && "retropie" != requestedSystem)
	{
		auto system = SystemData::getSystem(requestedSystem);
		if (system != nullptr && !system->isGroupChildSystem())
		{
			if (hideSystemView || startOnGamelist)
				goToGameList(system, forceImmediate);
			else
				goToSystemView(system, forceImmediate);

			return;
		}
	}

	if (hideSystemView || startOnGamelist)
		goToGameList(SystemData::getFirstVisibleSystem(), forceImmediate);
	else
		goToSystemView(SystemData::getFirstVisibleSystem());
}

void ViewController::reloadAndGoToStart()
{
	ViewController::get()->reloadAll();
	ViewController::get()->goToStart(false);
}

int ViewController::getSystemId(SystemData* system)
{
	std::vector<SystemData*>& sysVec = SystemData::sSystemVector;
	return (int)(std::find(sysVec.cbegin(), sysVec.cend(), system) - sysVec.cbegin());
}

void ViewController::goToSystemView(std::string& systemName, bool forceImmediate, ViewController::ViewMode mode)
{
	auto system = SystemData::getSystem(systemName);
	if (system != nullptr)
	{
		if (mode == ViewController::ViewMode::GAME_LIST)
			goToGameList(system, forceImmediate);
		else
			goToSystemView(system, forceImmediate);
	}
}

void ViewController::goToSystemView(SystemData* system, bool forceImmediate)
{
	// Tell any current view it's about to be hidden
	if (mCurrentView)
	{
		mCurrentView->onHide();
	}

	mState.viewing = SYSTEM_SELECT;
	mState.system = system;

	auto systemList = getSystemListView();
	systemList->setPosition(getSystemId(system) * (float)Renderer::getScreenWidth(), systemList->getPosition().y());

	systemList->goToSystem(system, false);
	mCurrentView = systemList;
	mCurrentView->onShow();
	PowerSaver::setState(true);

	playViewTransition(forceImmediate);
}

void ViewController::goToNextGameList()
{
	assert(mState.viewing == GAME_LIST);
	SystemData* system = getState().getSystem();
	assert(system);

	system = system->getNext();
	goToGameList(system);	

	if (AudioManager::isInitialized())
		AudioManager::getInstance()->changePlaylist(system->getTheme());
}

void ViewController::goToPrevGameList()
{
	assert(mState.viewing == GAME_LIST);
	SystemData* system = getState().getSystem();
	assert(system);

	system = system->getPrev();
	goToGameList(system);

	if (AudioManager::isInitialized())
		AudioManager::getInstance()->changePlaylist(system->getTheme());
}

void ViewController::goToGameList(SystemData* system, bool forceImmediate)
{
	if(mState.viewing == SYSTEM_SELECT)
	{
		// move system list
		auto sysList = getSystemListView();
		float offX = sysList->getPosition().x();
		int sysId = getSystemId(system);
		sysList->setPosition(sysId * (float)Renderer::getScreenWidth(), sysList->getPosition().y());
		offX = sysList->getPosition().x() - offX;
		mCamera.translation().x() -= offX;
	}

	mState.viewing = GAME_LIST;
	mState.system = system;

	if (mCurrentView)
		mCurrentView->onHide();

	mCurrentView = getGameListView(system);
	if (mCurrentView)
		mCurrentView->onShow();

	playViewTransition(forceImmediate);
}

void ViewController::playViewTransition(bool forceImmediate)
{
	Vector3f target(Vector3f::Zero());
	if(mCurrentView)
		target = mCurrentView->getPosition();

	// no need to animate, we're not going anywhere (probably goToNextGamelist() or goToPrevGamelist() when there's only 1 system)
	if(target == -mCamera.translation() && !isAnimationPlaying(0))
		return;

	std::string transition_style = Settings::getInstance()->getString("TransitionStyle");
	if (!forceImmediate && transition_style == "fade")
	{
		// fade
		// stop whatever's currently playing, leaving mFadeOpacity wherever it is
		cancelAnimation(0);

		auto fadeFunc = [this](float t) {
			mFadeOpacity = Math::lerp(0, 1, t);
		};

		const static int FADE_DURATION = 240; // fade in/out time
		const static int FADE_WAIT = 320; // time to wait between in/out
		setAnimation(new LambdaAnimation(fadeFunc, FADE_DURATION), 0, [this, fadeFunc, target] {
			this->mCamera.translation() = -target;
			updateHelpPrompts();
			setAnimation(new LambdaAnimation(fadeFunc, FADE_DURATION), FADE_WAIT, nullptr, true);
		});

		// fast-forward animation if we're partway faded
		if (target == -mCamera.translation())
		{
			// not changing screens, so cancel the first half entirely
			advanceAnimation(0, FADE_DURATION);
			advanceAnimation(0, FADE_WAIT);
			advanceAnimation(0, FADE_DURATION - (int)(mFadeOpacity * FADE_DURATION));
		}
		else
			advanceAnimation(0, (int)(mFadeOpacity * FADE_DURATION));		
	} 
	else if (!forceImmediate && (transition_style == "slide" || transition_style == "auto"))
	{
		// slide or simple slide
		setAnimation(new MoveCameraAnimation(mCamera, target));
		updateHelpPrompts(); // update help prompts immediately
	} 
	else 
	{
		// instant
		setAnimation(new LambdaAnimation(
			[this, target](float /*t*/)
		{
			this->mCamera.translation() = -target;
		}, 1));
		updateHelpPrompts();
	}
}

void ViewController::onFileChanged(FileData* file, FileChangeType change)
{
	auto it = mGameListViews.find(file->getSystem());
	if(it != mGameListViews.cend())
		it->second->onFileChanged(file, change);
}

void ViewController::launch(FileData* game, Vector3f center)
{
	if(game->getType() != GAME)
	{
		LOG(LogError) << "tried to launch something that isn't a game";
		return;
	}

	// Hide the current view
	//if (mCurrentView)
		//mCurrentView->onHide();

	Transform4x4f origCamera = mCamera;
	origCamera.translation() = -mCurrentView->getPosition();

	center += mCurrentView->getPosition();
	stopAnimation(1); // make sure the fade in isn't still playing
	mWindow->stopInfoPopup(); // make sure we disable any existing info popup
	mLockInput = true;

	mWindow->loadCustomImageLoadingScreen(game->getImagePath(), game->getName());

	std::string transition_style = Settings::getInstance()->getString("GameTransitionStyle");
	if(transition_style == "fade")
	{
		// fade out, launch game, fade back in
		auto fadeFunc = [this](float t) {
			mFadeOpacity = Math::lerp(0.0f, 1.0f, t);
		};
		setAnimation(new LambdaAnimation(fadeFunc, 800), 0, [this, game, fadeFunc]
		{
			game->launchGame(mWindow);
			setAnimation(new LambdaAnimation(fadeFunc, 800), 0, [this] { mLockInput = false; mWindow->endRenderLoadingScreen(); }, true);
			this->onFileChanged(game, FILE_METADATA_CHANGED);
		});
	} else if (transition_style == "slide"){
		// move camera to zoom in on center + fade out, launch game, come back in
		setAnimation(new LaunchAnimation(mCamera, mFadeOpacity, center, 1500), 0, [this, origCamera, center, game]
		{
			game->launchGame(mWindow);
			mCamera = origCamera;
			setAnimation(new LaunchAnimation(mCamera, mFadeOpacity, center, 600), 0, [this] { mLockInput = false; mWindow->endRenderLoadingScreen(); }, true);
			this->onFileChanged(game, FILE_METADATA_CHANGED);
		});
	} else { // instant
		setAnimation(new LaunchAnimation(mCamera, mFadeOpacity, center, 10), 0, [this, origCamera, center, game]
		{
			game->launchGame(mWindow);
			mCamera = origCamera;
			setAnimation(new LaunchAnimation(mCamera, mFadeOpacity, center, 10), 0, [this] { mLockInput = false; mWindow->endRenderLoadingScreen(); }, true);
			this->onFileChanged(game, FILE_METADATA_CHANGED);
		});
	}
}

void ViewController::removeGameListView(SystemData* system)
{
	//if we already made one, return that one
	auto exists = mGameListViews.find(system);
	if(exists != mGameListViews.cend())
	{
		exists->second.reset();
		mGameListViews.erase(system);
	}
}

std::shared_ptr<IGameListView> ViewController::getGameListView(SystemData* system, bool loadIfnull, const std::function<void()>& createAsPopupAndSetExitFunction)
{
	//if we already made one, return that one
	auto exists = mGameListViews.find(system);
	if(exists != mGameListViews.cend())
		return exists->second;

	if (!loadIfnull)
		return nullptr;

	system->setUIModeFilters();
	system->updateDisplayedGameCount();

	//if we didn't, make it, remember it, and return it
	std::shared_ptr<IGameListView> view;

	bool themeHasVideoView = system->getTheme()->hasView("video");
	bool themeHasGridView = system->getTheme()->hasView("grid");

	//decide type
	GameListViewType selectedViewType = AUTOMATIC;

	bool allowDetailedDowngrade = false;

	bool forceView = false;
	std::string viewPreference = Settings::getInstance()->getString("GamelistViewStyle");
	if (!system->getTheme()->hasView(viewPreference))
		viewPreference = "automatic";	

	std::string customThemeName;
	Vector2f gridSizeOverride = Vector2f::parseString(Settings::getInstance()->getString("DefaultGridSize"));
		
	
	if (viewPreference != "automatic" && !system->getSystemViewMode().empty() && system->getTheme()->hasView(system->getSystemViewMode()) && system->getSystemViewMode() != viewPreference)
		gridSizeOverride = Vector2f(0, 0);

	Vector2f bySystemGridOverride = system->getGridSizeOverride(); //Vector2f(0,0);
	if (bySystemGridOverride != Vector2f(0, 0))
		gridSizeOverride = bySystemGridOverride;
	
	if (!system->getSystemViewMode().empty() && system->getTheme()->hasView(system->getSystemViewMode()))
	{		
		viewPreference = system->getSystemViewMode();
		forceView = true;
	}

	if (viewPreference == "automatic")
	{
		auto defaultView = system->getTheme()->getDefaultView();
		if (!defaultView.empty() && system->getTheme()->hasView(defaultView))
			viewPreference = defaultView;
	}

	if (system->getTheme()->isCustomView(viewPreference))
	{
		auto baseClass = system->getTheme()->getCustomViewBaseType(viewPreference);
		if (!baseClass.empty()) // this is a customView
		{
			customThemeName = viewPreference;
			viewPreference = baseClass;
		}
	}

	if (viewPreference.compare("basic") == 0)
		selectedViewType = BASIC;
	else if (viewPreference.compare("detailed") == 0)
	{
		allowDetailedDowngrade = true;
		selectedViewType = DETAILED;
	}
	else if (themeHasGridView && viewPreference.compare("grid") == 0)
		selectedViewType = GRID;
	else if (viewPreference.compare("video") == 0)
		selectedViewType = VIDEO;

	if (!forceView && (selectedViewType == AUTOMATIC || allowDetailedDowngrade))
	{		
		selectedViewType = BASIC;

		if (system->getTheme()->getDefaultView() != "basic")
		{
			std::vector<FileData*> files = system->getRootFolder()->getFilesRecursive(GAME | FOLDER);
			for (auto it = files.cbegin(); it != files.cend(); it++)
			{
				if (themeHasVideoView && !(*it)->getVideoPath().empty() && viewPreference.compare("detailed") != 0)
				{
					selectedViewType = VIDEO;
					break;
				}
				else if (!(*it)->getThumbnailPath().empty())
				{
					/*
					if (!allowDetailedDowngrade && (*it)->metadata.get("thumbnail").length() > 0)
					{
						if (themeHasGridView)
							selectedViewType = GRID;
						else
							selectedViewType = DETAILED;
					}
					else*/
						selectedViewType = DETAILED;

					if (!themeHasVideoView)
						break;

					// Don't break out in case any subsequent files have video
				}
			}
		}		
	}

	// Create the view
	switch (selectedViewType)
	{
		case VIDEO:
			view = std::shared_ptr<IGameListView>(new VideoGameListView(mWindow, system->getRootFolder()));
			break;
		case DETAILED:
			view = std::shared_ptr<IGameListView>(new DetailedGameListView(mWindow, system->getRootFolder()));
			break;
		case GRID:		
			{
				view = std::shared_ptr<IGameListView>(new GridGameListView(mWindow, system->getRootFolder(), system->getTheme(), customThemeName, gridSizeOverride));
			}
						
			break;
		case BASIC:
		default:
			view = std::shared_ptr<IGameListView>(new BasicGameListView(mWindow, system->getRootFolder()));
			break;
	}
	
	if (selectedViewType != GRID)
	{
		// GridGameListView theme needs to be loaded before populating.

		if (!customThemeName.empty())
			view->setThemeName(customThemeName);

		view->setTheme(system->getTheme());
	}

	ISimpleGameListView* simpleListView = dynamic_cast<ISimpleGameListView*>(view.get());

	if (createAsPopupAndSetExitFunction != nullptr && simpleListView != nullptr)
	{
		if (mCurrentView)
		{
			mCurrentView->onHide();
			view->setPosition(mCurrentView->getPosition());
		}

		simpleListView->setPopupContext(view, mCurrentView, system->getIndex(true)->getTextFilter(), createAsPopupAndSetExitFunction);

		mCurrentView = view;
		mCurrentView->onShow();
		mCurrentView->topWindow(true);
	}
	else
	{
		int id = getSystemId(system);
		view->setPosition(id * (float)Renderer::getScreenWidth(), (float)Renderer::getScreenHeight() * 2);

		addChild(view.get());
		mGameListViews[system] = view;
	}

	return view;
}

std::shared_ptr<SystemView> ViewController::getSystemListView()
{
	//if we already made one, return that one
	if(mSystemListView)
		return mSystemListView;

	mSystemListView = std::shared_ptr<SystemView>(new SystemView(mWindow));
	addChild(mSystemListView.get());
	mSystemListView->setPosition(0, (float)Renderer::getScreenHeight());
	return mSystemListView;
}


bool ViewController::input(InputConfig* config, Input input)
{
	if (mLockInput)
		return true;

	if (config->getDeviceId() == DEVICE_KEYBOARD && input.value && input.id == SDLK_F5)
	{
		mWindow->render();

		ResourceManager::getInstance()->unloadAll();
		ResourceManager::getInstance()->reloadAll();

		ViewController::get()->reloadAll(mWindow);
		mWindow->endRenderLoadingScreen();
		return true;
	}

	if (input.value != 0)
	{
		// open menu
		if(!UIModeController::getInstance()->isUIModeKid() && config->isMappedTo("start", input))
		{
			// open menu
			mWindow->pushGui(new GuiMenu(mWindow, Settings::getInstance()->getBool("AnimatedMainMenu")));
			return true;
		}
		if (config->isMappedLike("leftthumb", input) || config->isMappedLike("rightthumb", input))
		{
			// next song
			AudioManager::getInstance()->playRandomMusic(false);
			return true;
		}
	}

	if(UIModeController::getInstance()->listen(config, input))  // check if UI mode has changed due to passphrase completion
		return true;

	if (mCurrentView)
		return mCurrentView->input(config, input);

	return false;
}

void ViewController::update(int deltaTime)
{
	if(mCurrentView)
		mCurrentView->update(deltaTime);

	updateSelf(deltaTime);
}

void ViewController::render(const Transform4x4f& parentTrans)
{
	Transform4x4f trans = mCamera * parentTrans;
	Transform4x4f transInverse;
	transInverse.invert(trans);

	// camera position, position + size
	Vector3f viewStart = transInverse.translation();
	Vector3f viewEnd = transInverse * Vector3f((float)Renderer::getScreenWidth(), (float)Renderer::getScreenHeight(), 0);

	// Keep track of UI mode changes.
	UIModeController::getInstance()->monitorUIMode();

	// clipping
	Vector3f sysStart = getSystemListView()->getPosition();
	Vector3f sysEnd = getSystemListView()->getPosition() + Vector3f(getSystemListView()->getSize().x(), getSystemListView()->getSize().y(), 0);

	// draw systemview
	if (!Settings::getInstance()->getBool("HideSystemView"))
		getSystemListView()->render(trans);
	
	// draw gamelists
	for(auto it = mGameListViews.cbegin(); it != mGameListViews.cend(); it++)
	{
		// clipping
		Vector3f guiStart = it->second->getPosition();
		Vector3f guiEnd = it->second->getPosition() + Vector3f(it->second->getSize().x(), it->second->getSize().y(), 0);

		if (guiEnd.x() > viewStart.x() && guiEnd.y() >= viewStart.y() && guiStart.x() < viewEnd.x() && guiStart.y() <= viewEnd.y())
			it->second->render(trans);
	}

	if(mWindow->peekGui() == this)
		mWindow->renderHelpPromptsEarly();

	// fade out
	if (mFadeOpacity)
	{
		if (Settings::getInstance()->getBool("HideWindow"))
		{		
			Renderer::setMatrix(parentTrans);		
			Renderer::drawRect(0.0f, 0.0f, Renderer::getScreenWidth(), Renderer::getScreenHeight(), 0x00000000 | (unsigned char)(mFadeOpacity * 255));
		}
		else 
			mWindow->renderGameLoadingScreen(mFadeOpacity, false);
	}
}

void ViewController::preload()
{
	bool preloadUI = Settings::getInstance()->getBool("PreloadUI");
	if (!preloadUI)
		return;

	mWindow->renderLoadingScreen(_("Preloading UI..."), 0);
	getSystemListView();

	int i = 1;
	int max = SystemData::sSystemVector.size() + 1;
	bool splash = preloadUI && Settings::getInstance()->getBool("SplashScreen") && Settings::getInstance()->getBool("SplashScreenProgress");

	for(auto it = SystemData::sSystemVector.cbegin(); it != SystemData::sSystemVector.cend(); it++)
	{
		if ((*it)->isGroupChildSystem() || !(*it)->isVisible())
			continue;

		if (splash)
		{
			i++;

			if ((i % 4) == 0)
				mWindow->renderLoadingScreen(_("Starting UI..."), (float)i / (float)max);
		}

		(*it)->resetFilters();
		getGameListView(*it);
	}
}

void ViewController::reloadGameListView(IGameListView* view, bool reloadTheme)
{
	if (reloadTheme)
		ThemeData::setDefaultTheme(nullptr);

	for(auto it = mGameListViews.cbegin(); it != mGameListViews.cend(); it++)
	{
		if(it->second.get() == view)
		{

		//	addPlaceholder()

			bool isCurrent = (mCurrentView == it->second);

			SystemData* system = it->first;
		
			FileData* cursor = view->getCursor();

			mGameListViews.erase(it);

			if (reloadTheme)
				system->loadTheme();

			system->setUIModeFilters();
			system->updateDisplayedGameCount();

			std::shared_ptr<IGameListView> newView = getGameListView(system);
			
			// to counter having come from a placeholder
			if (cursor != nullptr && !cursor->isPlaceHolder() && system->getName() != "recent")
				newView->setCursor(cursor);
			
			if(isCurrent)
				mCurrentView = newView;

			break;
		}
	}

	if (SystemData::sSystemVector.size() > 0 && reloadTheme)
		ViewController::get()->onThemeChanged(SystemData::sSystemVector.at(0)->getTheme());

	// Redisplay the current view
	if (mCurrentView)
		mCurrentView->onShow();
}

void ViewController::reloadAll(Window* window, bool reloadTheme)
{
	Utils::FileSystem::FileSystemCacheActivator fsc;

	if (mCurrentView != nullptr)
	{
		ISimpleGameListView* simpleView = dynamic_cast<ISimpleGameListView*>(mCurrentView.get());
		if (simpleView != nullptr)
			simpleView->closePopupContext();

		mCurrentView->onHide();
	}

	ThemeData::setDefaultTheme(nullptr);

	SystemData* system = nullptr;

	if (mState.viewing == SYSTEM_SELECT)
		system = getSelectedSystem();

	int gameListCount = 0;
	// clear all gamelistviews
	std::map<SystemData*, FileData*> cursorMap;
	for (auto it = mGameListViews.cbegin(); it != mGameListViews.cend(); it++)
	{
		gameListCount++;
		cursorMap[it->first] = it->second->getCursor();
	}

	mGameListViews.clear();

	// If preloaded is disabled
	for (auto it = SystemData::sSystemVector.cbegin(); it != SystemData::sSystemVector.cend(); it++)
		if (cursorMap.find((*it)) == cursorMap.end())
			cursorMap[(*it)] = NULL;

	if (reloadTheme && cursorMap.size() > 0)
	{
		int processedSystem = 0;
		int systemCount = cursorMap.size();

		Utils::ThreadPool pool;

		for (auto it = cursorMap.cbegin(); it != cursorMap.cend(); it++)
		{
			SystemData* pooledSystem = it->first;

			pool.queueWorkItem([pooledSystem, &processedSystem]
			{
				pooledSystem->loadTheme();
				pooledSystem->resetFilters();
				processedSystem++;
			});
		}

		if (window)
		{
			pool.wait([window, &processedSystem, systemCount]
			{
				int px = processedSystem;
				if (px >= 0 && px < systemCount)
					window->renderLoadingScreen(_("Loading theme..."), (float)px / (float)systemCount);
			}, 5);
		}
		else
			pool.wait();
	}

	bool preloadUI = Settings::getInstance()->getBool("PreloadUI");

	if (gameListCount > 0)
	{
		int lastTime = SDL_GetTicks() - 50;

		if (window)
			window->renderLoadingScreen(_("Loading gamelists..."), 0.0f);

		float idx = 0;
		// load themes, create gamelistviews and reset filters
		for (auto it = cursorMap.cbegin(); it != cursorMap.cend(); it++)
		{
			if (it->second == nullptr)
				continue;

			if (preloadUI)
				getGameListView(it->first)->setCursor(it->second);
			else if (mState.viewing == GAME_LIST)
			{
				if (mState.getSystem() == it->first)
					getGameListView(mState.getSystem())->setCursor(it->second);
			}

			idx++;

			int time = SDL_GetTicks();
			if (window && time - lastTime >= 20)
			{
				lastTime = time;
				window->renderLoadingScreen(_("Loading gamelists..."), (float)idx / (float)gameListCount);
			}
		}
	}

	if (window != nullptr)
		window->renderLoadingScreen(_("Loading..."));

	if (SystemData::sSystemVector.size() > 0)
	{
		for (auto sys : SystemData::sSystemVector)
		{
			auto theme = sys->getTheme();
			if (theme != nullptr)
			{
				ViewController::get()->onThemeChanged(theme);
				break;
			}
		}
	}

	// Rebuild SystemListView
	mSystemListView.reset();
	getSystemListView();

	// update mCurrentView since the pointers changed
	if(mState.viewing == GAME_LIST)
	{
		mCurrentView = getGameListView(mState.getSystem());
	}
	else if(mState.viewing == SYSTEM_SELECT && system != nullptr)
	{
		goToSystemView(system);
		mSystemListView->goToSystem(system, false);
		mCurrentView = mSystemListView;
	}
	else
		goToSystemView(SystemData::getFirstVisibleSystem());

	if (mCurrentView != nullptr)
		mCurrentView->onShow();

	updateHelpPrompts();
}

std::vector<HelpPrompt> ViewController::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts;
	if(!mCurrentView)
		return prompts;

	prompts = mCurrentView->getHelpPrompts();
	if(!UIModeController::getInstance()->isUIModeKid())
		prompts.push_back(HelpPrompt("start", _("MENU")));

	return prompts;
}

HelpStyle ViewController::getHelpStyle()
{
	if(!mCurrentView)
		return GuiComponent::getHelpStyle();

	return mCurrentView->getHelpStyle();
}


void ViewController::onThemeChanged(const std::shared_ptr<ThemeData>& theme)
{
	ThemeData::setDefaultTheme(theme.get());
	mWindow->onThemeChanged(theme);
}

void ViewController::onShow()
{
	if (mCurrentView)
		mCurrentView->onShow();
}

void ViewController::onScreenSaverActivate()
{
	GuiComponent::onScreenSaverActivate();

	if (mCurrentView)
		mCurrentView->onScreenSaverActivate();
}

void ViewController::onScreenSaverDeactivate()
{
	GuiComponent::onScreenSaverDeactivate();

	if (mCurrentView)
		mCurrentView->onScreenSaverDeactivate();
}

SystemData* ViewController::getSelectedSystem()
{
	if (mState.viewing == SYSTEM_SELECT)
	{
		int idx = mSystemListView->getCursorIndex();
		if (idx >= 0 && idx < mSystemListView->getObjects().size())
			return mSystemListView->getObjects()[mSystemListView->getCursorIndex()];
	}

	return mState.getSystem();
}

ViewController::ViewMode ViewController::getViewMode()
{
	return mState.viewing;
}

void ViewController::reloadAllGames(Window* window, bool deleteCurrentGui)
{
	if (sInstance == nullptr)
		return;

	Utils::FileSystem::FileSystemCacheActivator fsc;

	auto viewMode = ViewController::get()->getViewMode();
	auto systemName = ViewController::get()->getSelectedSystem()->getName();
	window->endRenderLoadingScreen();
	window->renderLoadingScreen(_("Loading..."));

	if (!deleteCurrentGui)
	{
		GuiComponent* topGui = window->peekGui();
		if (topGui != nullptr)
			window->removeGui(topGui);
	}

	GuiComponent *gui;
	while ((gui = window->peekGui()) != NULL)
	{
		window->removeGui(gui);

		if (gui != sInstance)
			delete gui;
	}

	ViewController::deinit();

	ViewController::init(window);
	CollectionSystemManager::init(window);
	SystemData::loadConfig(window);
	ViewController::get()->goToSystemView(systemName, true, viewMode);
	ViewController::get()->reloadAll(nullptr, false); // Avoid reloading themes a second time

	window->endRenderLoadingScreen();
	window->pushGui(ViewController::get());
}

void ViewController::setActiveView(std::shared_ptr<GuiComponent> view)
{
	if (mCurrentView != nullptr)
	{
		mCurrentView->topWindow(false);
		mCurrentView->onHide();
	}

	mCurrentView = view;
	mCurrentView->onShow();
	mCurrentView->topWindow(true);
}
