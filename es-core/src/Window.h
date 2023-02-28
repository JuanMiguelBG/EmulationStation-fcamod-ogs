#pragma once
#ifndef ES_CORE_WINDOW_H
#define ES_CORE_WINDOW_H

#include "HelpPrompt.h"
#include "InputConfig.h"
#include "Settings.h"

#include <memory>
#include <functional>

#include "components/BrightnessInfoComponent.h"
#include "components/VolumeInfoComponent.h"

class FileData;
class Font;
class GuiComponent;
class HelpComponent;
class ImageComponent;
class InputConfig;
class TextCache;
class Transform4x4f;
class TextureResource;
class AsyncNotificationComponent;
class ThemeData;
class TextComponent;
class ControllerActivityComponent;
class BatteryIndicatorComponent;

struct HelpStyle;

class Window
{
public:
	class ScreenSaver {
	public:
		virtual void startScreenSaver() = 0;
		virtual void stopScreenSaver() = 0;
		virtual void nextVideo() = 0;
		virtual void renderScreenSaver() = 0;
		virtual bool allowSleep() = 0;
		virtual void update(int deltaTime) = 0;
		virtual bool isScreenSaverActive() = 0;
		virtual FileData* getCurrentGame() = 0;
		virtual void launchGame() = 0;
		virtual void resetCounts() = 0;
		virtual bool isEnabled() = 0;
	};

	class InfoPopup {
	public:
		virtual void render(const Transform4x4f& parentTrans) = 0;
		virtual void stop() = 0;
		virtual ~InfoPopup() {};
	};

	Window();
	~Window();

	void pushGui(GuiComponent* gui);
	void removeGui(GuiComponent* gui);
	GuiComponent* peekGui();
	inline int getGuiStackSize() { return (int)mGuiStack.size(); }

	void textInput(const char* text);
	void input(InputConfig* config, Input input);
	void update(int deltaTime);
	void render();

	bool init(bool initRenderer = true);
	void deinit(bool deinitRenderer = true);

	void normalizeNextUpdate();

	inline bool isSleeping() const { return mSleeping; }
	bool getAllowSleep();
	void setAllowSleep(bool sleep);

	void endRenderLoadingScreen();
	void renderLoadingScreen(std::string text, float percent = -1, unsigned char opacity = 255);
	void renderGameLoadingScreen(float opacity=1, bool swapBuffers=true);

	void loadCustomImageLoadingScreen(std::string imagePath, std::string customText);

	void renderHelpPromptsEarly(); // used to render HelpPrompts before a fade
	void setHelpPrompts(const std::vector<HelpPrompt>& prompts, const HelpStyle& style);

	void setScreenSaver(ScreenSaver* screenSaver) { mScreenSaver = screenSaver; }
	bool isScreenSaverEnabled() { return mScreenSaver && mScreenSaver->isEnabled(); }
//	void setInfoPopup(InfoPopup* infoPopup) { delete mInfoPopup; mInfoPopup = infoPopup; }
	inline void stopInfoPopup() { if (mInfoPopup) mInfoPopup->stop(); };

	void startScreenSaver();
	bool cancelScreenSaver();
	void renderScreenSaver(bool checkSleep = false);

	void displayNotificationMessage(std::string message, int duration = -1);

	void registerNotificationComponent(AsyncNotificationComponent* pc);
	void unRegisterNotificationComponent(AsyncNotificationComponent* pc);

	void postToUiThread(const std::function<void()>& func, void* data = nullptr);
	void unregisterPostedFunctions(void* data);
	void reactivateGui();

	void onThemeChanged(const std::shared_ptr<ThemeData>& theme);

	VolumeInfoComponent* getVolumeInfoComponent() { return mVolumeInfo.get(); };
	BrightnessInfoComponent* getBrightnessInfoComponent() { return mBrightnessInfo.get(); };

	HelpComponent* getHelpComponent() { return mHelp; };

	float getHelpComponentHeight();

private:
	void processPostedFunctions();

	void renderRegisteredNotificationComponents(const Transform4x4f& trans);
	std::vector<AsyncNotificationComponent*> mAsyncNotificationComponent;

	struct PostedFunction
	{
		std::function<void()> func;
		void* container;
	};

	std::vector<PostedFunction> mFunctions;

	typedef std::pair<std::string, int> NotificationMessage;
	std::vector<NotificationMessage> mNotificationMessages;
	void processNotificationMessages();

	void screensaverNeedsToGoToSleep();
	void onSleep();
	void onWake();

	// Returns true if at least one component on the stack is processing
	bool isProcessing();

	HelpComponent*  mHelp;
	ImageComponent* mBackgroundOverlay;
	ScreenSaver*    mScreenSaver;
	InfoPopup*      mInfoPopup;
	bool            mRenderScreenSaver;

	std::shared_ptr<TextureResource> mSplash;
	std::string                      mCustomSplash;
	
	std::vector<GuiComponent*>       mScreenExtras;
	std::vector<GuiComponent*>       mGuiStack;

	std::vector< std::shared_ptr<Font> > mDefaultFonts;

	int mFrameTimeElapsed;
	int mFrameCountElapsed;
	int mAverageDeltaTime;

	std::unique_ptr<TextCache> mFrameDataText;

	// clock // batocera
	int mClockElapsed;
	
	std::shared_ptr<TextComponent> mClock;
	//std::shared_ptr<ControllerActivityComponent> mControllerActivity;
	std::shared_ptr<BatteryIndicatorComponent> mBatteryIndicator;
	std::shared_ptr<VolumeInfoComponent> mVolumeInfo;
	std::shared_ptr<BrightnessInfoComponent> mBrightnessInfo;

	bool mNormalizeNextUpdate;

	bool mAllowSleep;
	bool mSleeping;
	unsigned int mTimeSinceLastInput;

	bool mRenderedHelpPrompts;

};

#endif // ES_CORE_WINDOW_H
