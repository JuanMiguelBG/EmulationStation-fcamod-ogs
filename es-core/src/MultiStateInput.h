#pragma once
#ifndef ES_CORE_MULTISTATEINPUT_H
#define ES_CORE_MULTISTATEINPUT_H

#include "GuiComponent.h"

class Window;

class MultiStateInput
{
public:
	MultiStateInput(const std::string& buttonName);

	bool isShortPressed(InputConfig* config, Input input);
	bool isLongPressed(int deltaTime);

private:
	std::string mButtonName;
	int mTimeHoldingButton;
	bool mIsPressed;
};

#endif // ES_CORE_MULTISTATEINPUT_H
