#include "Window.h"

#include "components/HelpComponent.h"
#include "components/ImageComponent.h"
#include "components/TextComponent.h"
#include "resources/Font.h"
#include "resources/TextureResource.h"
#include "Log.h"
#include "Scripting.h"
#include <algorithm>
#include <iomanip>
#include <SDL_events.h>
#include "guis/GuiInfoPopup.h"
#include "components/AsyncNotificationComponent.h"
#include "components/ControllerActivityComponent.h"
#include "components/BatteryIndicatorComponent.h"
#include "guis/GuiMsgBox.h"
#include "AudioManager.h"
#include <string>
#include "utils/StringUtil.h"
#include "utils/TimeUtil.h"
#include "components/VolumeInfoComponent.h"
#include "components/BrightnessInfoComponent.h"


Window::Window() : mNormalizeNextUpdate(false), mFrameTimeElapsed(0), mFrameCountElapsed(0), mAverageDeltaTime(10),
  mAllowSleep(true), mSleeping(false), mTimeSinceLastInput(0), mScreenSaver(NULL), mRenderScreenSaver(false), mInfoPopup(NULL), mClockElapsed(0) // batocera
{	
	mHelp = new HelpComponent(this);
	mBackgroundOverlay = new ImageComponent(this);

	mSplash = NULL;	
}

Window::~Window()
{
	for (auto extra : mScreenExtras)
		delete extra;

	delete mBackgroundOverlay;

	// delete all our GUIs
	while(peekGui())
		delete peekGui();

	delete mHelp;
}

void Window::pushGui(GuiComponent* gui)
{
	if (mGuiStack.size() > 0)
	{
		auto& top = mGuiStack.back();
		top->topWindow(false);
	}
	mGuiStack.push_back(gui);
	gui->updateHelpPrompts();
}

void Window::removeGui(GuiComponent* gui)
{
	for(auto i = mGuiStack.cbegin(); i != mGuiStack.cend(); i++)
	{
		if(*i == gui)
		{
			i = mGuiStack.erase(i);

			if(i == mGuiStack.cend() && mGuiStack.size()) // we just popped the stack and the stack is not empty
			{
				mGuiStack.back()->updateHelpPrompts();
				mGuiStack.back()->topWindow(true);
			}

			return;
		}
	}
}

GuiComponent* Window::peekGui()
{
	if(mGuiStack.size() < 1)
		return NULL;

	return mGuiStack.back();
}

bool Window::init(bool initRenderer)
{
	LOG(LogInfo) << "Window::init() - initRenderer: " << Utils::String::boolToString(initRenderer);

	if (initRenderer)
	{
		if (!Renderer::init())
		{
			LOG(LogError) << "Window::init() --> Renderer failed to initialize!";
			return false;
		}
	}
	else
		Renderer::activateWindow();

	ResourceManager::getInstance()->reloadAll();

	//keep a reference to the default fonts, so they don't keep getting destroyed/recreated
	if(mDefaultFonts.empty())
	{
		mDefaultFonts.push_back(Font::get(FONT_SIZE_SMALL));
		mDefaultFonts.push_back(Font::get(FONT_SIZE_MEDIUM));
		mDefaultFonts.push_back(Font::get(FONT_SIZE_LARGE));
	}

	mBackgroundOverlay->setImage(":/scroll_gradient.png");
	mBackgroundOverlay->setResize((float)Renderer::getScreenWidth(), (float)Renderer::getScreenHeight());

	if (mClock == nullptr)
	{
		mClock = std::make_shared<TextComponent>(this);
		mClock->setFont(Font::get(FONT_SIZE_SMALL));
		mClock->setHorizontalAlignment(ALIGN_RIGHT);
		mClock->setVerticalAlignment(ALIGN_TOP);
		float clockPosition = Renderer::getScreenWidth() * 0.89f;
		float clockSize = mClock->getFont()->getLetterWidth() * 7.f;
		mClock->setPosition(clockPosition, Renderer::getScreenHeight() * 0.9965 - mClock->getFont()->getHeight());
		mClock->setSize(clockSize, 0);
		mClock->setColor(0x777777FF);
	}

	//if (mControllerActivity == nullptr)
		//mControllerActivity = std::make_shared<ControllerActivityComponent>(this);

	if (mBatteryIndicator == nullptr)
		mBatteryIndicator = std::make_shared<BatteryIndicatorComponent>(this);

	if (mVolumeInfo == nullptr)
		mVolumeInfo = std::make_shared<VolumeInfoComponent>(this);
	else
		mVolumeInfo->reset();

	if (mBrightnessInfo == nullptr)
		mBrightnessInfo = std::make_shared<BrightnessInfoComponent>(this);
	else
		mBrightnessInfo->reset();

	// update our help because font sizes probably changed
	if (peekGui())
		peekGui()->updateHelpPrompts();
	
	return true;
}

void Window::reactivateGui()
{
	for (auto extra : mScreenExtras)
		extra->onShow();

	for (auto i = mGuiStack.cbegin(); i != mGuiStack.cend(); i++)
		(*i)->onShow();

	if (peekGui())
		peekGui()->updateHelpPrompts();
}

void Window::deinit(bool deinitRenderer)
{
	LOG(LogInfo) << "Window::deinit() - deinitRenderer: " << (deinitRenderer ? "true" : "false");
	for (auto extra : mScreenExtras)
		extra->onHide();

	// Hide all GUI elements on uninitialisation - this disable
	for(auto i = mGuiStack.cbegin(); i != mGuiStack.cend(); i++)
		(*i)->onHide();

	TextureResource::resetCache();

	ResourceManager::getInstance()->unloadAll();

	if (deinitRenderer)
		Renderer::deinit();
}

void Window::textInput(const char* text)
{
	if(peekGui())
		peekGui()->textInput(text);
}

void Window::input(InputConfig* config, Input input)
{
	if (mScreenSaver && mScreenSaver->isEnabled())
	{
		std::string screensaver_behavior = Settings::getInstance()->getString("ScreenSaverBehavior");
		if (screensaver_behavior != "suspend")
		{
			if (mScreenSaver->isScreenSaverActive())
			{
				if (((screensaver_behavior == "slideshow") || (screensaver_behavior == "random video")) && Settings::getInstance()->getBool("ScreenSaverControls"))
				{
					if((mScreenSaver->getCurrentGame() != NULL) && (config->isMappedLike("right", input) || config->isMappedTo("start", input) || config->isMappedTo("select", input)))
					{
						if(config->isMappedLike("right", input) || config->isMappedTo("select", input))
						{
							if (input.value != 0) // handle screensaver control
								mScreenSaver->nextVideo();

							return;
						}
						else if(config->isMappedTo("start", input) && input.value != 0)
						{
							// launch game!
							cancelScreenSaver();
							mScreenSaver->launchGame();
							// to force handling the wake up process
							mSleeping = true;
						}
					}
				}
				else if ( ((screensaver_behavior == "black") || (screensaver_behavior == "dim")) && (config->isMappedTo("select", input) && input.value == 0))
					return;
			}
		}
	}

	if(mSleeping)
	{
		// wake up
		mTimeSinceLastInput = 0;
		cancelScreenSaver();
		mSleeping = false;
		onWake();
		return;
	}

	mTimeSinceLastInput = 0;
	if (cancelScreenSaver())
		return;

	if(config->getDeviceId() == DEVICE_KEYBOARD && input.value && input.id == SDLK_g && SDL_GetModState() & KMOD_LCTRL/* && Settings::getInstance()->getBool("Debug")*/)
	{
		// toggle debug grid with Ctrl-G
		Settings::getInstance()->setBool("DebugGrid", !Settings::getInstance()->getBool("DebugGrid"));
	}
	else if(config->getDeviceId() == DEVICE_KEYBOARD && input.value && input.id == SDLK_t && SDL_GetModState() & KMOD_LCTRL/* && Settings::getInstance()->getBool("Debug")*/)
	{
		// toggle TextComponent debug view with Ctrl-T
		Settings::getInstance()->setBool("DebugText", !Settings::getInstance()->getBool("DebugText"));
	}
	else if(config->getDeviceId() == DEVICE_KEYBOARD && input.value && input.id == SDLK_i && SDL_GetModState() & KMOD_LCTRL/* && Settings::getInstance()->getBool("Debug")*/)
	{
		// toggle TextComponent debug view with Ctrl-I
		Settings::getInstance()->setBool("DebugImage", !Settings::getInstance()->getBool("DebugImage"));
	}
	else
	{
		//if (Settings::getInstance()->getBool("ShowControllerActivity") && (mControllerActivity != nullptr))
			//mControllerActivity->input(config, input);

		if (peekGui())
		{
			this->peekGui()->input(config, input); // this is where the majority of inputs will be consumed: the GuiComponent Stack
		}
	}
}

void Window::update(int deltaTime)
{	
	processPostedFunctions();
	processNotificationMessages();

	if(mNormalizeNextUpdate)
	{
		mNormalizeNextUpdate = false;
		if(deltaTime > mAverageDeltaTime)
			deltaTime = mAverageDeltaTime;
	}

	if (Settings::getInstance()->getBool("VolumePopup") && (mVolumeInfo != nullptr))
		mVolumeInfo->update(deltaTime);

	if (Settings::getInstance()->getBool("BrightnessPopup") && (mBrightnessInfo != nullptr))
		mBrightnessInfo->update(deltaTime);

	mFrameTimeElapsed += deltaTime;
	mFrameCountElapsed++;
	if(mFrameTimeElapsed > 500)
	{
		mAverageDeltaTime = mFrameTimeElapsed / mFrameCountElapsed;

		if(Settings::getInstance()->getBool("DrawFramerate"))
		{
			std::stringstream ss;

			// fps
			ss << std::fixed << std::setprecision(1) << (1000.0f * (float)mFrameCountElapsed / (float)mFrameTimeElapsed) << "fps, ";
			ss << std::fixed << std::setprecision(2) << ((float)mFrameTimeElapsed / (float)mFrameCountElapsed) << "ms";

			// vram
			float textureVramUsageMb = TextureResource::getTotalMemUsage() / 1000.0f / 1000.0f;
			float textureTotalUsageMb = TextureResource::getTotalTextureSize() / 1000.0f / 1000.0f;
			float fontVramUsageMb = Font::getTotalMemUsage() / 1000.0f / 1000.0f;

			ss << "\nFont VRAM: " << fontVramUsageMb << " Tex VRAM: " << textureVramUsageMb <<
				  " Tex Max: " << textureTotalUsageMb;
			mFrameDataText = std::unique_ptr<TextCache>(mDefaultFonts.at(1)->buildTextCache(ss.str(), 50.f, 50.f, 0xFF00FFFF));
		}

		mFrameTimeElapsed = 0;
		mFrameCountElapsed = 0;
	}

	/* draw the clock */ // batocera
	if (Settings::getInstance()->getBool("DrawClock") && mClock) 
	{
		mClockElapsed -= deltaTime;
		if (mClockElapsed <= 0)
		{
			time_t     clockNow = time(0);
			struct tm  clockTstruct = *localtime(&clockNow);

			if (clockTstruct.tm_year > 100) 
			{ 
				// Display the clock only if year is more than 1900+100
				// Visit http://en.cppreference.com/w/cpp/chrono/c/strftime for more information about date/time format
				
				std::string clockBuf;
				if (Settings::getInstance()->getBool("ClockMode12"))
					clockBuf = Utils::Time::timeToString(clockNow, "%I:%M %p");
				else
					clockBuf = Utils::Time::timeToString(clockNow, "%H:%M");

				mClock->setText(clockBuf);
			}

			mClockElapsed = 1000; // next update in 1000ms
		}
	}
	
	mTimeSinceLastInput += deltaTime;

	if(peekGui())
		peekGui()->update(deltaTime);

	// Update the screensaver
	if (isScreenSaverEnabled())
		mScreenSaver->update(deltaTime);

	// update pads // batocera
	//if (Settings::getInstance()->getBool("ShowControllerActivity") && mControllerActivity)
	//if (mControllerActivity)
		//mControllerActivity->update(deltaTime);

	if ((mBatteryIndicator != nullptr) && (Settings::getInstance()->getBool("ShowBatteryIndicator") || Settings::getInstance()->getBool("ShowNetworkIndicator")))
		mBatteryIndicator->update(deltaTime);

	AudioManager::update(deltaTime);
}

void Window::render()
{
	Transform4x4f transform = Transform4x4f::Identity();

	mRenderedHelpPrompts = false;

	// draw only bottom and top of GuiStack (if they are different)
	if (mGuiStack.size())
	{
		auto& bottom = mGuiStack.front();
		auto& top = mGuiStack.back();

		bottom->render(transform);
		if(bottom != top)
		{
			if (top->isKindOf<GuiMsgBox>() && mGuiStack.size() > 2)
			{
				auto& middle = mGuiStack.at(mGuiStack.size() - 2);
				if (middle != bottom)
					middle->render(transform);
			}

			mBackgroundOverlay->render(transform);
			top->render(transform);
		}
	}

	if (!mRenderedHelpPrompts)
		mHelp->render(transform);

	if (Settings::getInstance()->getBool("DrawFramerate") && mFrameDataText)
	{
		Renderer::setMatrix(Transform4x4f::Identity());
		mDefaultFonts.at(1)->renderTextCache(mFrameDataText.get());
	}


	// clock // batocera
	if (Settings::getInstance()->getBool("DrawClock") && (mClock != nullptr) && (mGuiStack.size() < 2 || !Renderer::isSmallScreen()))
		mClock->render(transform);

	//if (Settings::getInstance()->getBool("ShowControllerActivity") && (mControllerActivity != nullptr) && (mGuiStack.size() < 2 || !Renderer::isSmallScreen()))ShowNetworkIndicator
	//if ((mControllerActivity != nullptr) && (mGuiStack.size() < 2 || !Renderer::isSmallScreen()))
		//mControllerActivity->render(transform);

	if ((mBatteryIndicator != nullptr) && (Settings::getInstance()->getBool("ShowBatteryIndicator") || Settings::getInstance()->getBool("ShowNetworkIndicator")) && (mGuiStack.size() < 2 || !Renderer::isSmallScreen()))
		mBatteryIndicator->render(transform);

	// pads // batocera
	Renderer::setMatrix(Transform4x4f::Identity());

	unsigned int screensaverTime = (unsigned int)Settings::getInstance()->getInt("ScreenSaverTime");
	if (isScreenSaverEnabled() && (mTimeSinceLastInput >= screensaverTime) )
		startScreenSaver();

	if (!mRenderScreenSaver && mInfoPopup)
		mInfoPopup->render(transform);

	renderRegisteredNotificationComponents(transform);
	

	// Always call the screensaver render function regardless of whether the screensaver is active
	// or not because it may perform a fade on transition
	renderScreenSaver();

	for (auto extra : mScreenExtras)
		extra->render(transform);

	if (Settings::getInstance()->getBool("VolumePopup") && mVolumeInfo)
		mVolumeInfo->render(transform);

	if (Settings::getInstance()->getBool("BrightnessPopup") && mBrightnessInfo)
		mBrightnessInfo->render(transform);

	if(isScreenSaverEnabled() && (mTimeSinceLastInput >= screensaverTime))
		screensaverNeedsToGoToSleep();
}

void Window::normalizeNextUpdate()
{
	mNormalizeNextUpdate = true;
}

bool Window::getAllowSleep()
{
	return mAllowSleep;
}

void Window::setAllowSleep(bool sleep)
{
	mAllowSleep = sleep;
}

void Window::endRenderLoadingScreen()
{
	mSplash = NULL;
	mCustomSplash = "";
}

void Window::renderLoadingScreen(std::string text, float percent, unsigned char opacity)
{	
	if (mSplash == NULL)
		mSplash = TextureResource::get(":/splash.svg", false, true, false, false);

	Transform4x4f trans = Transform4x4f::Identity();
	Renderer::setMatrix(trans);
	Renderer::drawRect(0, 0, Renderer::getScreenWidth(), Renderer::getScreenHeight(), 0x00000000 | opacity);
	
	if (percent >= 0)
	{
		float baseHeight = 0.04f;

		float w = Renderer::getScreenWidth() / 2;
		float h = Renderer::getScreenHeight() * baseHeight;

		float x = Renderer::getScreenWidth() / 2 - w / 2;
		float y = Renderer::getScreenHeight() - (Renderer::getScreenHeight() * 3 * baseHeight);

		Renderer::drawRect(x, y, w, h, 0x25252500 | opacity);
		Renderer::drawRect(x, y, (w*percent), h, 0x006C9E00 | opacity, 0x003E5C00 | opacity, true); // 0xFFFFFFFF
	}
	
	ImageComponent splash(this, true);
	splash.setResize(Renderer::getScreenWidth() * 0.4f, 0.0f);

	if (mSplash != NULL)
		splash.setImage(mSplash);
	else
		splash.setImage(":/splash.svg");

	splash.setPosition((Renderer::getScreenWidth() - splash.getSize().x()) / 2, (Renderer::getScreenHeight() - splash.getSize().y()) / 2 * 0.7f);
	splash.render(trans);
	
	auto& font = mDefaultFonts.at(1);
	TextCache* cache = font->buildTextCache(text, 0, 0, 0x65656500 | opacity);

	float x = Math::round((Renderer::getScreenWidth() - cache->metrics.size.x()) / 2.0f);
	float y = Math::round(Renderer::getScreenHeight() * 0.78f); // 35
	trans = trans.translate(Vector3f(x, y, 0.0f));
	Renderer::setMatrix(trans);
	font->renderTextCache(cache);
	delete cache;

	Renderer::swapBuffers();
}

void Window::loadCustomImageLoadingScreen(std::string imagePath, std::string customText)
{
	if (!Utils::FileSystem::exists(imagePath))
		return;

	if (Settings::getInstance()->getBool("HideWindow"))
		return;

	if (mSplash != NULL)
		endRenderLoadingScreen();

	mSplash = TextureResource::get(imagePath, false, false, true, false, false, MaxSizeInfo(Renderer::getScreenWidth() * 0.60f, Renderer::getScreenHeight() * 0.60f));
	mCustomSplash = customText;
	
	std::shared_ptr<ResourceManager>& rm = ResourceManager::getInstance();
	rm->removeReloadable(mSplash);
}

void Window::renderGameLoadingScreen(float opacity, bool swapBuffers)
{
	if (mSplash == NULL)
		mSplash = TextureResource::get(":/splash.svg", false, true, false, false);

	Transform4x4f trans = Transform4x4f::Identity();
	Renderer::setMatrix(trans);
	Renderer::drawRect(0.0f, 0.0f, Renderer::getScreenWidth(), Renderer::getScreenHeight(), 0x00000000 | (unsigned char)(opacity * 255));

	ImageComponent splash(this, true);

	if (mSplash != NULL)
		splash.setImage(mSplash);
	else
		splash.setImage(":/splash.svg");

	splash.setOrigin(0.5, 0.5);
	splash.setPosition(Renderer::getScreenWidth() / 2.0f, Renderer::getScreenHeight() * 0.835f / 2.0f);
	splash.setMaxSize(Renderer::getScreenWidth() * 0.60f, Renderer::getScreenHeight() * 0.60f);

	if (!mCustomSplash.empty())
		splash.setColorShift(0xFFFFFF00 | (unsigned char)(opacity * 210));
	else
		splash.setColorShift(0xFFFFFF00 | (unsigned char)(opacity * 255));

	splash.render(trans);

	auto& font = mDefaultFonts.at(1);
	font->reload(); // Ensure font is loaded

	TextCache* cache = font->buildTextCache(mCustomSplash.empty() ? _("Loading...") : mCustomSplash, 0, 0, 0x65656500 | (unsigned char)(opacity * 255));

	float x = Math::round((Renderer::getScreenWidth() - cache->metrics.size.x()) / 2.0f);
	float y = Math::round(Renderer::getScreenHeight() * 0.835f);
	trans = trans.translate(Vector3f(x, y, 0.0f));
	Renderer::setMatrix(trans);
	font->renderTextCache(cache);
	delete cache;

	if (swapBuffers)
		Renderer::swapBuffers();
}


void Window::renderHelpPromptsEarly()
{
	mHelp->render(Transform4x4f::Identity());
	mRenderedHelpPrompts = true;
}

void Window::setHelpPrompts(const std::vector<HelpPrompt>& prompts, const HelpStyle& style)
{
	// Keep a temporary reference to the previous grid.
	// It avoids unloading/reloading images if they are the same, and avoids flickerings
	auto oldGrid = mHelp->getGrid();

	mHelp->clearPrompts();
	mHelp->setStyle(style);
	
	mClockElapsed = -1;

	std::vector<HelpPrompt> addPrompts;

	std::map<std::string, bool> inputSeenMap;
	std::map<std::string, int> mappedToSeenMap;
	for(auto it = prompts.cbegin(); it != prompts.cend(); it++)
	{
		// only add it if the same icon hasn't already been added
		if(inputSeenMap.emplace(it->first, true).second)
		{
			// this symbol hasn't been seen yet, what about the action name?
			auto mappedTo = mappedToSeenMap.find(it->second);
			if(mappedTo != mappedToSeenMap.cend())
			{
				// yes, it has!

				// can we combine? (dpad only)
				//if((it->first == "up/down" && addPrompts.at(mappedTo->second).first != "left/right") ||
					//(it->first == "left/right" && addPrompts.at(mappedTo->second).first != "up/down"))
				//{
					// yes!
					//addPrompts.at(mappedTo->second).first = "up/down/left/right";
					// don't need to add this to addPrompts since we just merged
				//}else{
					// no, we can't combine!
					addPrompts.push_back(*it);
				//}
			}else{
				// no, it hasn't!
				mappedToSeenMap.emplace(it->second, (int)addPrompts.size());
				addPrompts.push_back(*it);
			}
		}
	}

	// sort prompts so it goes [dpad_all] [dpad_u/d] [dpad_l/r] [a/b/x/y/l/r] [start/select]
	std::sort(addPrompts.begin(), addPrompts.end(), [](const HelpPrompt& a, const HelpPrompt& b) -> bool {

		static const char* map[] = {
			"up/down/left/right",
			"up/down",
			"left/right",
			BUTTON_OK, BUTTON_BACK, "x", "y", "l", "r",
			"start", "select",
			NULL
		};

		int i = 0;
		int aVal = 0;
		int bVal = 0;
		while(map[i] != NULL)
		{
			if(a.first == map[i])
				aVal = i;
			if(b.first == map[i])
				bVal = i;
			i++;
		}

		return aVal > bVal;
	});

	mHelp->setPrompts(addPrompts);
}

void Window::screensaverNeedsToGoToSleep()
{
	if (!isProcessing() && mAllowSleep && mScreenSaver->allowSleep())
	{
		// go to sleep
		if (mSleeping == false)
		{
			mSleeping = true;
			onSleep();
		}
	}
}

void Window::onSleep()
{
	Log::flush();
	Scripting::fireEvent("sleep");
}

void Window::onWake()
{
	Scripting::fireEvent("wake");
}

bool Window::isProcessing()
{
	return count_if(mGuiStack.cbegin(), mGuiStack.cend(), [](GuiComponent* c) { return c->isProcessing(); }) > 0;
}

void Window::startScreenSaver()
{
	if (isScreenSaverEnabled() && !mRenderScreenSaver)
	{
		Scripting::fireEvent("screensaver-start");
		for (auto extra : mScreenExtras)
			extra->onScreenSaverActivate();

		// Tell the GUI components the screensaver is starting
		for(auto i = mGuiStack.cbegin(); i != mGuiStack.cend(); i++)
			(*i)->onScreenSaverActivate();

		mScreenSaver->startScreenSaver();
		mRenderScreenSaver = true;
	}
}

bool Window::cancelScreenSaver()
{
	std::string screensaver_behavior = Settings::getInstance()->getString("ScreenSaverBehavior");
	if ((screensaver_behavior == "suspend") || !isScreenSaverEnabled())
		return false;

	if (mScreenSaver && mRenderScreenSaver)
	{
		LOG(LogInfo) << "Window::cancelScreenSaver() - canceling ...";
		mScreenSaver->stopScreenSaver();
		mRenderScreenSaver = false;
		mScreenSaver->resetCounts();
		Scripting::fireEvent("screensaver-stop");

		// Tell the GUI components the screensaver has stopped
		for(auto i = mGuiStack.cbegin(); i != mGuiStack.cend(); i++)
			(*i)->onScreenSaverDeactivate();

		for (auto extra : mScreenExtras)
			extra->onScreenSaverDeactivate();

		LOG(LogInfo) << "Window::cancelScreenSaver() - canceled";
		return true;
	}

	return false;
}

void Window::renderScreenSaver(bool checkSleep)
{
	if (isScreenSaverEnabled())
		mScreenSaver->renderScreenSaver();

	if (mRenderScreenSaver && checkSleep)
		screensaverNeedsToGoToSleep();
}

static std::mutex mNotificationMessagesLock;

void Window::displayNotificationMessage(std::string message, int duration)
{
	std::unique_lock<std::mutex> lock(mNotificationMessagesLock);

	if (duration <= 0)
	{
		duration = Settings::getInstance()->getInt("audio.display_titles_time");
		if (duration <= 2 || duration > 120)
			duration = 10;

		duration *= 1000;
	}

	NotificationMessage msg;
	msg.first = message;
	msg.second = duration;
	mNotificationMessages.push_back(msg);
}

void Window::processNotificationMessages()
{
	std::unique_lock<std::mutex> lock(mNotificationMessagesLock);

	if (mNotificationMessages.empty())
		return;

	NotificationMessage msg = mNotificationMessages.back();
	mNotificationMessages.pop_back();

	LOG(LogDebug) << "Notification message : [\"" << msg.first.c_str() << "\", " << msg.second << "]";

	if (mInfoPopup)
		delete mInfoPopup;
	// TODO translate msg fields
	mInfoPopup = new GuiInfoPopup(this, msg.first, msg.second);
}

void Window::registerNotificationComponent(AsyncNotificationComponent* pc)
{
	std::unique_lock<std::mutex> lock(mNotificationMessagesLock);

	if (std::find(mAsyncNotificationComponent.cbegin(), mAsyncNotificationComponent.cend(), pc) != mAsyncNotificationComponent.cend())
		return;

	mAsyncNotificationComponent.push_back(pc);
}

void Window::unRegisterNotificationComponent(AsyncNotificationComponent* pc)
{
	std::unique_lock<std::mutex> lock(mNotificationMessagesLock);

	auto it = std::find(mAsyncNotificationComponent.cbegin(), mAsyncNotificationComponent.cend(), pc);
	if (it != mAsyncNotificationComponent.cend())
		mAsyncNotificationComponent.erase(it);
}

void Window::renderRegisteredNotificationComponents(const Transform4x4f& trans)
{
	std::unique_lock<std::mutex> lock(mNotificationMessagesLock);

#define PADDING_H  (Renderer::getScreenWidth()*0.01)

	float posY = Renderer::getScreenHeight() * 0.02f;

	for (auto child : mAsyncNotificationComponent)
	{
		float posX = Renderer::getScreenWidth()*0.99f - child->getSize().x();

		child->setPosition(posX, posY, 0);
		child->render(trans);

		posY += child->getSize().y() + PADDING_H;
	}
}

void Window::unregisterPostedFunctions(void* data)
{
	if (data == nullptr)
		return;

	for (auto it = mFunctions.cbegin(); it != mFunctions.cend(); )
	{
		if ((*it).container == data)
			it = mFunctions.erase(it);
		else
			it++;
	}
}

void Window::postToUiThread(const std::function<void()>& func, void* data)
{
	std::unique_lock<std::mutex> lock(mNotificationMessagesLock);

	PostedFunction pf;
	pf.func = func;
	pf.container = data;
	mFunctions.push_back(pf);
}

void Window::processPostedFunctions()
{
	std::unique_lock<std::mutex> lock(mNotificationMessagesLock);

	for (auto func : mFunctions)
	{
		TRYCATCH("processPostedFunction", func.func())
	}

	mFunctions.clear();
}

void Window::onThemeChanged(const std::shared_ptr<ThemeData>& theme)
{
	for (auto extra : mScreenExtras)
		delete extra;

	mScreenExtras.clear();
	mScreenExtras = ThemeData::makeExtras(theme, "screen", this);

	std::stable_sort(mScreenExtras.begin(), mScreenExtras.end(), [](GuiComponent* a, GuiComponent* b) { return b->getZIndex() > a->getZIndex(); });

	if (mBackgroundOverlay)
		mBackgroundOverlay->setImage(ThemeData::getMenuTheme()->Background.fadePath);

	if (mClock)
	{
		mClock->setFont(Font::get(FONT_SIZE_SMALL));
		mClock->setColor(0x777777FF);
		mClock->setVerticalAlignment(ALIGN_TOP);
		mClock->setHorizontalAlignment(ALIGN_RIGHT);
		
		// if clock element does not exist in screen view -> <view name="screen"><text name="clock"> 
		// skin it from system.helpsystem -> <view name="system"><helpsystem name="help"> )
		if (!theme->getElement("screen", "clock", "text"))
		{
			auto elem = theme->getElement("system", "help", "helpsystem");
			if (elem && elem->has("textColor"))
				mClock->setColor(elem->get<unsigned int>("textColor"));

			if (elem && (elem->has("fontPath") || elem->has("fontSize")))
				mClock->setFont(Font::getFromTheme(elem, ThemeFlags::ALL, Font::get(FONT_SIZE_MEDIUM)));
		}

		float clockSize = mClock->getFont()->getLetterWidth() * 10.f;
		float clockPosition = Renderer::getScreenWidth() - clockSize;
		mClock->setPosition(clockPosition, Renderer::getScreenHeight() * 0.9965 - mClock->getFont()->getHeight());
		mClock->setSize(clockSize, 0);

		mClock->applyTheme(theme, "screen", "clock", ThemeFlags::ALL ^ (ThemeFlags::TEXT));
	}

//	if (Settings::getInstance()->getBool("ShowControllerActivity") && mControllerActivity)
	//if (mControllerActivity)
		//mControllerActivity->applyTheme(theme, "screen", "controllerActivity", ThemeFlags::ALL ^ (ThemeFlags::TEXT));

	if (mBatteryIndicator != nullptr)
		mBatteryIndicator->applyTheme(theme, "screen", "batteryIndicator", ThemeFlags::ALL);

	if (Settings::getInstance()->getBool("VolumePopup") && (mVolumeInfo != nullptr))
		mVolumeInfo = std::make_shared<VolumeInfoComponent>(this);

	if (Settings::getInstance()->getBool("BrightnessPopup") && (mBrightnessInfo != nullptr))
		mBrightnessInfo = std::make_shared<BrightnessInfoComponent>(this);

}

float Window::getHelpComponentHeight()
{
	if (Settings::getInstance()->getBool("ShowHelpPrompts"))
	{
		if (mHelp && mHelp->isVisible() && mHelp->getGrid())
		{
			return mHelp->getGrid()->getSize().y();
		}
	}
	return 0.f;
}
