#pragma once

#include <mutex>
#include "GuiComponent.h"
#include "Log.h"

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
	void updateBrightness(int brightness)
	{
		setVisible(false);
		setBrightness(brightness);
		setVisible(true);
	}

	void reset() { mBrightness = 1; }

private:
	NinePatchComponent* mFrame;
	TextComponent*      mLabel;

	int mBrightness;

	int mCheckTime;
	int mDisplayTime;
};
