#pragma once
#ifndef ES_CORE_GUIS_GUI_TEXT_EDIT_POPUP_H
#define ES_CORE_GUIS_GUI_TEXT_EDIT_POPUP_H

#include "components/ComponentGrid.h"
#include "components/NinePatchComponent.h"
#include "GuiComponent.h"

class TextComponent;
class TextEditComponent;

class GuiTextEditPopup : public GuiComponent
{
public:
	GuiTextEditPopup(Window* window, const std::string& title, const std::string& initValue,
		const std::function<bool(const std::string&)>& okCallback, bool multiLine,
		const std::function<void(const std::string&)>& cancelCallback = nullptr);

	GuiTextEditPopup(Window* window, const std::string& title, const std::string& initValue,
		const std::function<bool(const std::string&)>& okCallback, bool multiLine,
		const char* acceptBtnText = "OK", const char* acceptPromptText = "OK",
		const std::function<void(const std::string&)>& cancelCallback = nullptr,
		const std::function<void(const std::string&)>& backCallback = nullptr,
		const char* cancelBtnText = "CANCEL", const char* cancelPromptText = "DISCARD CHANGES");

	bool input(InputConfig* config, Input input);
	void onSizeChanged();
	std::vector<HelpPrompt> getHelpPrompts() override;

private:
	NinePatchComponent mBackground;
	ComponentGrid mGrid;

	std::shared_ptr<TextComponent> mTitle;
	std::shared_ptr<TextEditComponent> mText;
	std::shared_ptr<ComponentGrid> mButtonGrid;

	std::function<void(const std::string&)> mBackCallback;

	bool mMultiLine;
};

#endif // ES_CORE_GUIS_GUI_TEXT_EDIT_POPUP_H
