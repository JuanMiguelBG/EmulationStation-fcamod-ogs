#include "components/ButtonComponent.h"

#include "resources/Font.h"
#include "utils/StringUtil.h"

ButtonComponent::ButtonComponent(Window* window, const std::string& text, const std::string& helpText, const std::function<void()>& func, bool upperCase) : GuiComponent(window),
	mBox(window, ThemeData::getMenuTheme()->Icons.button),
	mFont(Font::get(FONT_SIZE_MEDIUM)), 
	mFocused(false), 
	mEnabled(true), 
	mTextColorFocused(0xFFFFFFFF), mTextColorUnfocused(0x777777FF)
{
	auto menuTheme = ThemeData::getMenuTheme();

	mFont = menuTheme->Text.font;
	mTextColorUnfocused = menuTheme->Text.color;
	mTextColorFocused = menuTheme->Text.selectedColor;	
	mColor = menuTheme->Text.color;
	mColorFocused = menuTheme->Text.selectorColor;
	mRenderNonFocusedBackground = true;
	
	if (Renderer::isSmallScreen())
		mBox.setCornerSize(8, 8);

	setPressedFunc(func);
	setText(text, helpText, upperCase);
	updateImage();
}

void ButtonComponent::onSizeChanged()
{
	auto sz = mBox.getCornerSize();
	mBox.fitTo(mSize, Vector3f::Zero(), Vector2f(-sz.x() * 2, -sz.y() * 2));
}

void ButtonComponent::setPressedFunc(std::function<void()> f)
{
	mPressedFunc = f;
}

bool ButtonComponent::input(InputConfig* config, Input input)
{
	if(config->isMappedTo(BUTTON_OK, input) && input.value != 0)
	{
		if(mPressedFunc && mEnabled)
			mPressedFunc();
		return true;
	}

	return GuiComponent::input(config, input);
}

void ButtonComponent::setText(const std::string& text, const std::string& helpText, bool upperCase)
{
	mText = upperCase ? Utils::String::toUpper(text) : text;
	mHelpText = helpText;
	
	mTextCache = std::unique_ptr<TextCache>(mFont->buildTextCache(mText, 0, 0, getCurTextColor()));

	float minWidth = mFont->sizeText("DELETE").x() + 12;
	setSize(Math::max(mTextCache->metrics.size.x() + 12, minWidth), mTextCache->metrics.size.y());

	updateHelpPrompts();
}

void ButtonComponent::onFocusGained()
{
	if (mFocused)
		return;

	mFocused = true;
	updateImage();
}

void ButtonComponent::onFocusLost()
{
	if (!mFocused)
		return;

	mFocused = false;
	updateImage();
}

void ButtonComponent::setEnabled(bool enabled)
{
	mEnabled = enabled;
	updateImage();
}

void ButtonComponent::updateImage()
{
	if(!mEnabled || !mPressedFunc)
	{
		mBox.setImagePath(ThemeData::getMenuTheme()->Icons.button_filled);
		mBox.setCenterColor(0x770000FF);
		mBox.setEdgeColor(0x770000FF);
		return;
	}

	// If a new color has been set.  
	if (mNewColor) {
		mBox.setImagePath(ThemeData::getMenuTheme()->Icons.button_filled);
		mBox.setCenterColor(mModdedColor);
		mBox.setEdgeColor(mModdedColor);
		return;
	}

	mBox.setCenterColor(getCurBackColor());
	mBox.setEdgeColor(getCurBackColor());
	mBox.setImagePath(mFocused ? ThemeData::getMenuTheme()->Icons.button_filled : ThemeData::getMenuTheme()->Icons.button);	
	//mBox.setImagePath(mFocused ? ":/button_filled.png" : ":/button.png");
}

void ButtonComponent::render(const Transform4x4f& parentTrans)
{
	Transform4x4f trans = parentTrans * getTransform();

	if (mRenderNonFocusedBackground || mFocused)
		mBox.render(trans);

	if(mTextCache)
	{
		Vector3f centerOffset((mSize.x() - mTextCache->metrics.size.x()) / 2, (mSize.y() - mTextCache->metrics.size.y()) / 2, 0);
		trans = trans.translate(centerOffset);

		Renderer::setMatrix(trans);
		mTextCache->setColor(getCurTextColor());
		mFont->renderTextCache(mTextCache.get());
		trans = trans.translate(-centerOffset);
	}

	renderChildren(trans);
}

unsigned int ButtonComponent::getCurTextColor() const
{
	return mFocused ? mTextColorFocused : mTextColorUnfocused;
}

unsigned int ButtonComponent::getCurBackColor() const
{
	return mFocused ? mColorFocused : mColor;
}

std::vector<HelpPrompt> ButtonComponent::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts;
	prompts.push_back(HelpPrompt(BUTTON_OK, _( (mHelpText.empty() ? mText.c_str() : mHelpText.c_str()) ) ));
	return prompts;
}

void ButtonComponent::setPadding(const Vector4f padding)
{
	if (mPadding == padding)
		return;

	mPadding = padding;
	onSizeChanged();
}
