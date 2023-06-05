#pragma once
#ifndef ES_CORE_COMPONENTS_MENU_COMPONENT_H
#define ES_CORE_COMPONENTS_MENU_COMPONENT_H

#include "components/ComponentGrid.h"
#include "components/ComponentList.h"
#include "components/NinePatchComponent.h"
#include "components/TextComponent.h"
#include "utils/StringUtil.h"

class ButtonComponent;
class ImageComponent;

float getHelpComponentHeight(Window* window, bool addHelpComponentHeight);
std::shared_ptr<ComponentGrid> makeButtonGrid(Window* window, const std::vector< std::shared_ptr<ButtonComponent> >& buttons, bool addHelpComponentHeight = true);
std::shared_ptr<ComponentGrid> makeMultiDimButtonGrid(Window* window, const std::vector< std::vector< std::shared_ptr<ButtonComponent> > >& buttons, float outerWidth);
std::shared_ptr<ImageComponent> makeArrow(Window* window);

#define TITLE_VERT_PADDING (Renderer::getScreenHeight()*0.0637f)
#define TITLE_WITHSUB_VERT_PADDING (Renderer::getScreenHeight()*0.05f)
#define SUBTITLE_VERT_PADDING (Renderer::getScreenHeight()*0.019f)

class MenuComponent : public GuiComponent
{
public:
	MenuComponent(Window* window, 
		const std::string title, bool computeHelpComponentSize);

	MenuComponent(Window* window, 
		const std::string title, const std::shared_ptr<Font>& titleFont = Font::get(FONT_SIZE_LARGE),
		const std::string subTitle = "", bool computeHelpComponentSize = true);

	void onSizeChanged() override;

	inline void addRow(const ComponentListRow& row, bool setCursorHere = false, bool doUpdateSize = true, const std::string userData = "") { mList->addRow(row, setCursorHere, true, userData); if (doUpdateSize) updateSize(); }
	inline void clear() { mList->clear(); }

	void addWithLabel(const std::string& label, const std::shared_ptr<GuiComponent>& comp, const std::string iconName = "", bool setCursorHere = false, bool invert_when_selected = true);
	void addWithDescription(const std::string& label, const std::string& description, const std::function<void()>& func = nullptr, const std::string iconName = "", const std::string userData = "") { addWithDescription(label, description, nullptr, func, iconName, false, true, false, userData); };
	void addWithDescription(const std::string& label, const std::string& description, const std::shared_ptr<GuiComponent>& comp, const std::function<void()>& func = nullptr, const std::string iconName = "", bool setCursorHere = false, bool invert_when_selected = true, bool multiLine = false, const std::string userData = "");
	void addEntry(const std::string name, const std::function<void()>& func, const std::string userData = "") { addEntry(name, false, func, "", false, true, false, userData, true); };
	void addEntry(const std::string name, bool add_arrow, const std::function<void()>& func, const std::string iconName = "", bool setCursorHere = false, bool invert_when_selected = true, bool onButtonRelease = false, const std::string userData = "", bool doUpdateSize = true);
	void addComponent(const std::shared_ptr<GuiComponent>& comp, const std::function<void()>& func, const std::string iconName = "", bool setCursorHere = false, bool invert_when_selected = true, bool onButtonRelease = false, const std::string userData = "", bool doUpdateSize = true);

	void addGroup(const std::string& label) { mList->addGroup(label); updateSize(); }

	void addButton(const std::string& label, const std::string& helpText, const std::function<void()>& callback);

	void setTitle(const std::string title, const std::shared_ptr<Font>& font = nullptr);
	void setSubTitle(const std::string text);

	inline void setCursorToList() { mGrid.setCursorTo(mList); }
	inline void setCursorToButtons() { assert(mButtonGrid); mGrid.setCursorTo(mButtonGrid); }

	inline int size() const { return mList->size(); }
	inline bool hasElements() const { return mList->size() > 0; }
	inline int getCursor() const { return mList->getCursorId(); }
	inline std::string getSelected() { return mList->getSelected(); }

	virtual std::vector<HelpPrompt> getHelpPrompts() override;

	float getButtonGridHeight() const;

	void setMaxHeight(float maxHeight) 
	{ 
		if (mMaxHeight == maxHeight)
			return;

		mMaxHeight = maxHeight;
		updateSize();
	}

//	void setPosition(float x, float y, float z = 0.0f) override;

	void updateSize();
	void clearButtons();

	std::shared_ptr<ComponentList> getList() { return mList; };

	bool isComputeHelpComponentSize() { return mComputeHelpComponentSize; };

private:
	void updateGrid();

	NinePatchComponent mBackground;
	ComponentGrid mGrid;

	std::shared_ptr<ComponentGrid> mHeaderGrid;
	std::shared_ptr<TextComponent> mTitle;
	std::shared_ptr<TextComponent> mSubtitle;
	std::shared_ptr<ComponentList> mList;
	std::shared_ptr<ComponentGrid> mButtonGrid;
	std::vector< std::shared_ptr<ButtonComponent> > mButtons;

	float mMaxHeight;
	bool mComputeHelpComponentSize;
};

#endif // ES_CORE_COMPONENTS_MENU_COMPONENT_H
