#include "SwitchComponent.h"

#include "resources/Font.h"


SwitchComponent::SwitchComponent(Window* window, bool state) : GuiComponent(window), mImage(window), mState(state)
{
	auto menuTheme = ThemeData::getMenuTheme();

	float height = Font::get(FONT_SIZE_MEDIUM)->getLetterHeight();	

	mImage.setImage(state ? menuTheme->Icons.on : menuTheme->Icons.off);
	mImage.setResize(0, height);
	mImage.setColorShift(menuTheme->Text.color);

	mSize = mImage.getSize();
}

void SwitchComponent::setColor(unsigned int color) {
	mImage.setColorShift(color);
}

void SwitchComponent::onSizeChanged()
{
	mImage.setSize(mSize);
}

bool SwitchComponent::input(InputConfig* config, Input input)
{
	if(config->isMappedTo(BUTTON_OK, input) && input.value)
	{
		mState = !mState;
		onStateChanged();
		return true;
	}

	return false;
}

void SwitchComponent::render(const Transform4x4f& parentTrans)
{
	Transform4x4f trans = parentTrans * getTransform();

	mImage.render(trans);

	renderChildren(trans);
}

bool SwitchComponent::getState() const
{
	return mState;
}

void SwitchComponent::setState(bool state)
{
	mState = state;
	onStateChanged();
}

std::string SwitchComponent::getValue() const
{
	return mState ?  "true" : "false";
}

void SwitchComponent::setValue(const std::string& statestring)
{
	mState = (statestring == "true" ? true : false);

	onStateChanged();
}

void SwitchComponent::onStateChanged()
{
	auto theme = ThemeData::getMenuTheme();
	mImage.setImage(mState ? theme->Icons.on : theme->Icons.off);

	if (mOnChangedCallback != nullptr)
		mOnChangedCallback();
}

std::vector<HelpPrompt> SwitchComponent::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts;
	prompts.push_back(HelpPrompt(BUTTON_OK, _("MODIFIER")));
	return prompts;
}
