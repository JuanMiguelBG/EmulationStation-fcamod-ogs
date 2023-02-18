#pragma once
#ifndef ES_APP_COMPONENTS_FLAG_OPTION_LIST_COMPONENT_H
#define ES_APP_COMPONENTS_FLAG_OPTION_LIST_COMPONENT_H

#include "GuiComponent.h"
#include "Log.h"

#include "GuiComponent.h"
#include "Log.h"
#include "Window.h"
#include "EsLocale.h"
#include "ThemeData.h"
#include "utils/FileSystemUtil.h"

//Used to display a list of options.
//Can select one options.

// * <- curEntry Flag ->

// always
// * press a -> open full list

#define CHECKED_PATH ":/checkbox_checked.svg"
#define UNCHECKED_PATH ":/checkbox_unchecked.svg"

template<typename T>
class FlagOptionListComponent : public GuiComponent
{
private:
	struct FlagOptionListData
	{
		std::string name;
		std::string flag;

		T object;
		bool selected;

		std::string group;
	};

	class FlagOptionListPopup : public GuiComponent
	{
	private:
		MenuComponent mMenu;
		FlagOptionListComponent<T>* mParent;

	public:
		FlagOptionListPopup(Window* window, FlagOptionListComponent<T>* parent, const std::string& title,
			const std::function<void(T& data, ComponentListRow& row)> callback = nullptr) : GuiComponent(window),
			mMenu(window, title.c_str(), true), mParent(parent)
		{
			auto menuTheme = ThemeData::getMenuTheme();
			auto font = menuTheme->Text.font;
			auto color = menuTheme->Text.color;

			ComponentListRow row;

			// for select all/none
			std::vector<ImageComponent*> checkboxes;

			for(auto it = mParent->mEntries.begin(); it != mParent->mEntries.end(); it++)
			{
				row.elements.clear();

				FlagOptionListData& e = *it;

				if (callback != nullptr)
				{
					callback(e.object, row);
					row.makeAcceptInputHandler([this, &e]
						{
							e.selected = !e.selected;
							mParent->onSelectedChanged();
						});
				}
				else
				{
					auto text_comp = std::make_shared<TextComponent>(mWindow, Utils::String::toUpper(it->name), font, color);
					text_comp->setAutoScroll(Settings::getInstance()->getBool("AutoscrollMenuEntries"));
					row.addElement(text_comp, true);

					auto spacer = std::make_shared<GuiComponent>(mWindow);
					spacer->setSize(Renderer::getScreenWidth() * 0.005f, 0);
					row.addElement(spacer, false);

					auto flag = std::make_shared<ImageComponent>(mWindow);
					flag->setImage(it->flag);
					flag->setResize(Vector2f(0, font->getLetterHeight()));
					row.addElement(flag, false);

					// input handler for non-multiselect
					// update selected value and close
					row.makeAcceptInputHandler([this, &e]
						{
							mParent->mEntries.at(mParent->getSelectedId()).selected = false;
							e.selected = true;
							mParent->onSelectedChanged();
							delete this;
						});
				}

				if (!e.group.empty())
					mMenu.addGroup(e.group);

				// also set cursor to this row if we're not multi-select and this row is selected
				mMenu.addRow(row, it->selected);
			}

			mMenu.addButton(_("BACK"), _("ACCEPT"), [this] { delete this; });

			mMenu.setPosition(
				(Renderer::getScreenWidth() - mMenu.getSize().x()) / 2,
				(Renderer::getScreenHeight() - mMenu.getSize().y()) / 2);
				//Renderer::getScreenHeight() * 0.15f);
			addChild(&mMenu);
		}

		bool input(InputConfig* config, Input input) override
		{
			if(config->isMappedTo(BUTTON_BACK, input) && input.value != 0)
			{
				delete this;
				return true;
			}

			return GuiComponent::input(config, input);
		}

		std::vector<HelpPrompt> getHelpPrompts() override
		{
			auto prompts = mMenu.getHelpPrompts();
			prompts.push_back(HelpPrompt(BUTTON_BACK, _("BACK")));
			return prompts;
		}
	};

public:
	FlagOptionListComponent(Window* window, const std::string& name) : GuiComponent(window), mName(name),
		 mText(window), mLeftArrow(window), mRightArrow(window), mFlag(window)
	{
		auto theme = ThemeData::getMenuTheme();

		mAddRowCallback = nullptr;

		mText.setFont(theme->Text.font);
		mText.setColor(theme->Text.color);
		mText.setHorizontalAlignment(ALIGN_CENTER);
		addChild(&mText);

		mFlag.setResize(0, mText.getFont()->getLetterHeight());
		mLeftArrow.setResize(0, mText.getFont()->getLetterHeight());
		mRightArrow.setResize(0, mText.getFont()->getLetterHeight());

		mLeftArrow.setImage(ThemeData::getMenuTheme()->Icons.option_arrow); //  ":/option_arrow.svg"
		mLeftArrow.setColorShift(theme->Text.color);
		mLeftArrow.setFlipX(true);
		addChild(&mLeftArrow);

		mRightArrow.setImage(ThemeData::getMenuTheme()->Icons.option_arrow); // ":/option_arrow.svg");
		mRightArrow.setColorShift(theme->Text.color);
		addChild(&mRightArrow);

		std::string flagPath = ":/flags/" + Settings::getInstance()->getString("Language") + ".png";
		if (!ResourceManager::getInstance()->fileExists(flagPath))
			flagPath = ":/flags/no_flag.png";

		mFlag.setImage(flagPath); // ":/flags/xx.png");
		addChild(&mFlag);

		setSize(mLeftArrow.getSize().x() + mRightArrow.getSize().x() + mFlag.getSize().x() + (space * 2), theme->Text.font->getHeight());
	}

	virtual void setColor(unsigned int color)
	{
		mText.setColor(color);
		mLeftArrow.setColorShift(color);
		mRightArrow.setColorShift(color);
	}

	// handles positioning/resizing of text and arrows
	void onSizeChanged() override
	{
		mLeftArrow.setResize(0, mText.getFont()->getLetterHeight());
		mRightArrow.setResize(0, mText.getFont()->getLetterHeight());
		mFlag.setResize(0, mText.getFont()->getLetterHeight());

		if(mSize.x() < (mLeftArrow.getSize().x() + mRightArrow.getSize().x() + mFlag.getSize().x()))
			LOG(LogWarning) << "FlagOptionListComponent::onSizeChanged() - too narrow!";

		mText.setSize(mSize.x() - mLeftArrow.getSize().x() - mRightArrow.getSize().x() - mFlag.getSize().x() - (space * 2), mText.getFont()->getHeight());


		// position
		mLeftArrow.setPosition(0, (mSize.y() - mLeftArrow.getSize().y()) / 2);
		mText.setPosition(mLeftArrow.getPosition().x() + mLeftArrow.getSize().x(), (mSize.y() - mText.getSize().y()) / 2);
		mFlag.setPosition(mText.getPosition().x() + mText.getSize().x() + space, (mSize.y() - mFlag.getSize().y()) / 2);
		mRightArrow.setPosition(mFlag.getPosition().x() + mFlag.getSize().x() + (space * 2), (mSize.y() - mRightArrow.getSize().y()) / 2);
	}

	bool input(InputConfig* config, Input input) override
	{
		if(input.value != 0)
		{
			if(config->isMappedTo(BUTTON_OK, input))
			{
				open();
				return true;
			}

			if(config->isMappedLike("left", input))
			{
				// move selection to previous
				unsigned int i = getSelectedId();
				int next = (int)i - 1;
				if(next < 0)
					next += (int)mEntries.size();

				mEntries.at(i).selected = false;
				mEntries.at(next).selected = true;
				onSelectedChanged();
				return true;

			}else if(config->isMappedLike("right", input))
			{
				if (mEntries.size() == 0)
					return true;

				// move selection to next
				unsigned int i = getSelectedId();
				int next = (i + 1) % mEntries.size();
				mEntries.at(i).selected = false;
				mEntries.at(next).selected = true;
				onSelectedChanged();
				return true;
			}

		}
		return GuiComponent::input(config, input);
	}

	std::vector<T> getSelectedObjects()
	{
		std::vector<T> ret;
		for(auto it = mEntries.cbegin(); it != mEntries.cend(); it++)
		{
			if(it->selected)
				ret.push_back(it->object);
		}

		return ret;
	}

	T getSelected()
	{
		auto selected = getSelectedObjects();
		assert(selected.size() == 1);
		return selected.at(0);
	}

	void add(const std::string& name, const T& obj, bool selected, const std::string flag)
	{
		for (auto sysIt = mEntries.cbegin(); sysIt != mEntries.cend(); sysIt++)
			if (sysIt->name == name)
				return;

		FlagOptionListData e;
		e.name = name;
		e.object = obj;
		e.selected = selected;
		e.flag = flag;
		if (!ResourceManager::getInstance()->fileExists(flag))
			e.flag = ":/flags/no_flag.png";

		e.group = mGroup;
		mGroup = "";

		if (selected)
			firstSelected = obj;

		mEntries.push_back(e);
		onSelectedChanged();
	}

	void addRange(const std::vector<std::string> values, const std::string selectedValue = "")
	{
		for (auto value : values)
			add(_(value.c_str()), value, selectedValue == value);

		if (!hasSelection())
			selectFirstItem();
	}

	void addRange(const std::vector<std::pair<std::string, T>> values, const T selectedValue)
	{
		for (auto value : values)
			add(value.first.c_str(), value.second, selectedValue == value.second);

		if (!hasSelection())
			selectFirstItem();
	}

	void addGroup(const std::string name)
	{
		mGroup = name;
	}

	void remove(const std::string& name)
	{
		for (auto sysIt = mEntries.cbegin(); sysIt != mEntries.cend(); sysIt++)
		{
			if (sysIt->name == name)
			{
				bool isSelect = sysIt->selected;

				mEntries.erase(sysIt);

				if (isSelect)
					selectFirstItem();

				break;
			}
		}
	}

	void selectNone()
	{
		for(unsigned int i = 0; i < mEntries.size(); i++)
		{
			mEntries.at(i).selected = false;
		}
		onSelectedChanged();
	}

	bool changed(){
	  auto selected = getSelectedObjects();
	  if(selected.size() != 1) return false;
	  return firstSelected != getSelected();
	}

	bool hasSelection()
	{
		for (unsigned int i = 0; i < mEntries.size(); i++)
			if (mEntries.at(i).selected)
				return true;

		return false;
	}

	void selectFirstItem()
	{
		for (unsigned int i = 0; i < mEntries.size(); i++)
			mEntries.at(i).selected = false;

		if (mEntries.size() > 0)
			mEntries.at(0).selected = true;

		onSelectedChanged();
	}

	void clear() {
		mEntries.clear();
	}

	inline void invalidate() {
		onSelectedChanged();
	}

	void setSelectedChangedCallback(const std::function<void(const T&)>& callback)
	{
		mSelectedChangedCallback = callback;
	}

	void setRowTemplate(std::function<void(T& data, ComponentListRow& row)> callback)
	{
		mAddRowCallback = callback;
	}

private:
	std::function<void(T& data, ComponentListRow& row)> mAddRowCallback;

	void open()
	{
		mWindow->pushGui(new FlagOptionListPopup(mWindow, this, mName, mAddRowCallback));
	}

	unsigned int getSelectedId()
	{
		for(unsigned int i = 0; i < mEntries.size(); i++)
		{
			if(mEntries.at(i).selected)
				return i;
		}

		LOG(LogWarning) << "FlagOptionListComponent::getSelectedId() - no selected element found, defaulting to 0";
		return 0;
	}

	void onSelectedChanged()
	{
		// display currently selected + l/r cursors
		for(auto it = mEntries.cbegin(); it != mEntries.cend(); it++)
		{
			if (it->selected)
			{
				mText.setText(Utils::String::toUpper(it->name));
				mText.setSize(0, mText.getSize().y());

				mFlag.setImage( it->flag );

				setSize(mText.getSize().x() + mLeftArrow.getSize().x() + mRightArrow.getSize().x() + mFlag.getSize().x() + (space * 2 ) + 24, mText.getSize().y());
				if (mParent) // hack since theres no "on child size changed" callback atm...
					mParent->onSizeChanged();
				break;
			}
		}

		if (mSelectedChangedCallback)
			mSelectedChangedCallback(mEntries.at(getSelectedId()).object);
	}

	std::vector<HelpPrompt> getHelpPrompts() override
	{
		std::vector<HelpPrompt> prompts;
		prompts.push_back(HelpPrompt("left/right", _("MODIFIER")));
		prompts.push_back(HelpPrompt(BUTTON_OK, _("SELECTIONNER")));
		return prompts;
	}

	std::string mName;
	std::string mGroup;

	T firstSelected;
	TextComponent mText;
	ImageComponent mFlag;
	ImageComponent mLeftArrow;
	ImageComponent mRightArrow;
	float space = Renderer::getScreenWidth() * 0.005f;

	std::vector<FlagOptionListData> mEntries;
	std::function<void(const T&)> mSelectedChangedCallback;
};

#endif // ES_APP_COMPONENTS_FLAG_OPTION_LIST_COMPONENT_H
