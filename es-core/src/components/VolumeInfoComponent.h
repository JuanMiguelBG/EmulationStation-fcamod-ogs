#pragma once

#include <mutex>
#include "GuiComponent.h"

class ComponentGrid;
class NinePatchComponent;
class TextComponent;
class Window;

class VolumeInfoComponent : public GuiComponent
{
public:
	VolumeInfoComponent(Window* window, bool actionLine = true);
	~VolumeInfoComponent();

	void render(const Transform4x4f& parentTrans) override;
	void update(int deltaTime) override;

	void setVolume(int volume) { mVolume = volume; }
	void reset() { mVolume = -1; }

private:
	NinePatchComponent* mFrame;
	TextComponent*		mLabel;

	int mVolume;

	int mCheckTime;
	int mDisplayTime;
};