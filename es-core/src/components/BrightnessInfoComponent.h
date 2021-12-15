#pragma once

#include <mutex>
#include "GuiComponent.h"

class ComponentGrid;
class NinePatchComponent;
class TextComponent;
class Window;

class BrightnessInfoComponent : public GuiComponent
{
public:
	BrightnessInfoComponent(Window* window, bool actionLine = true);
	~BrightnessInfoComponent();

	void render(const Transform4x4f& parentTrans) override;
	void update(int deltaTime) override;

	void setBrightness(int brightness){ mBrightness = brightness; }
	void reset() { mBrightness = -1; }

	void lock() { mLocked = true; }
	void unlock() { mLocked = false; }

private:
	bool isLocked();

	NinePatchComponent* mFrame;
	TextComponent*      mLabel;

	int mBrightness;

	int mCheckTime;
	int mDisplayTime;
	bool mLocked = false;
};
