#pragma once

#include <string>
#include "components/ComponentGrid.h"

class TextComponent;
class Window;

class MultiLineMenuEntry : public ComponentGrid
{
public:
	MultiLineMenuEntry(Window* window, const std::string& text, const std::shared_ptr<TextComponent>& comp_subtext, bool multiLine = false);
	MultiLineMenuEntry(Window* window, const std::string& text, const std::string& mSubText, bool multiLine = false);

	void setColor(unsigned int color) override;
	void onSizeChanged() override;

protected:
	bool mMultiLine;
	bool mSizeChanging;
	std::shared_ptr<TextComponent> mText;
	std::shared_ptr<TextComponent> mSubText;
private:
	void init(const std::string& text, bool multiLine, bool updateSubText);
};
