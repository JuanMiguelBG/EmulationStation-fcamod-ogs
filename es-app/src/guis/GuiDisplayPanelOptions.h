#pragma once
#ifndef ES_APP_GUIS_GUI_DISPLAY_PANEL_OPTIONS_H
#define ES_APP_GUIS_GUI_DISPLAY_PANEL_OPTIONS_H

#include "components/ComponentGrid.h"
#include "components/NinePatchComponent.h"
#include "components/MenuComponent.h"
#include "GuiComponent.h"

class ComponentList;
class TextComponent;

class GuiDisplayPanelOptions : public GuiComponent
{
public:
	GuiDisplayPanelOptions(Window* window, bool cursor = false);
	
	void addRow(const ComponentListRow& row, bool setCursorHere = false, bool doUpdateSize = true);
	void addWithLabel(const std::string& label, const std::shared_ptr<GuiComponent>& comp, bool setCursorHere = false, bool invert_when_selected = true);
	void addEntry(const std::string name, const std::function<void()>& func, bool setCursorHere = false, bool invert_when_selected = true, bool doUpdateSize = true);

	bool input(InputConfig* config, Input input) override;
	void onSizeChanged() override;
    void updateSize();
	virtual std::vector<HelpPrompt> getHelpPrompts() override;

	static void openDisplayPanelOptions(Window* window, bool cursor = false);

private:
	void resetDisplayPanelSettings(Window* window, GuiComponent *gui);

	NinePatchComponent mBackground;
	ComponentGrid mGrid;
	
	std::shared_ptr<TextComponent> mTitle;
	std::shared_ptr<ComponentGrid> mHeaderGrid;
	std::shared_ptr<ComponentList> mList;
	std::shared_ptr<ComponentGrid> mImagesGrid;
	std::shared_ptr<ComponentGrid> mButtons;
};

#endif // ES_APP_GUIS_GUI_DISPLAY_PANEL_OPTIONS_H
