#include "guis/GuiTextEditPopupKeyboard.h"
#include "components/MenuComponent.h"
#include "utils/StringUtil.h"
#include "Log.h"
#include "EsLocale.h"
#include "SystemConf.h"
#include "Settings.h"

#define OSK_WIDTH (Renderer::isSmallScreen() ? Renderer::getScreenWidth() : Renderer::getScreenWidth() * 0.78f)
#define OSK_HEIGHT (Renderer::isSmallScreen() ? Renderer::getScreenHeight() : Renderer::getScreenHeight() * 0.60f)

#define OSK_PADDINGX (Renderer::getScreenWidth() * 0.02f)
#define OSK_PADDINGY (Renderer::getScreenWidth() * 0.01f)

#define BUTTON_GRID_HORIZ_PADDING (Renderer::getScreenWidth()*0.0052083333)

std::vector<std::vector<const char*>> kbUs {

	{ "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "_", "+", "DEL" },
	{ "!", "@", "#", "$", "%", "^", "&", "*", "(", ")", "-", "=", "DEL" },
	{ "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "_", "+", "DEL" },

	{ "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "{", "}", "OK" },
	{ "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "[", "]", "OK" },
	{ "à", "ä", "è", "ë", "ì", "ï", "ò", "ö", "ù", "ü", "¨", "¿", "OK" },

	{ "a", "s", "d", "f", "g", "h", "j", "k", "l", ";", "\"", "|", "-rowspan-" },
	{ "A", "S", "D", "F", "G", "H", "J", "K", "L", ":", "'", "\\", "-rowspan-" },
	{ "á", "â", "é", "ê", "í", "î", "ó", "ô", "ú", "û", "ñ", "¡", "-rowspan-" },

	{ "~", "z", "x", "c", "v", "b", "n", "m", ",", ".", "?", "ALT", "-colspan-" },
	{ "`", "Z", "X", "C", "V", "B", "N", "M", "<", ">", "/", "ALT", "-colspan-" },
	{ "€", "", "", "", "", "", "", "", "", "", "", "ALT", "-colspan-" },

	{ "SHIFT", "-colspan-", "SPACE", "-colspan-", "-colspan-", "-colspan-", "-colspan-", "CUR_LEFT", "CUR_RIGHT", "RESET", "-colspan-", "CANCEL", "-colspan-" },
	{ "SHIFT", "-colspan-", "SPACE", "-colspan-", "-colspan-", "-colspan-", "-colspan-", "CUR_LEFT", "CUR_RIGHT", "RESET", "-colspan-", "CANCEL", "-colspan-" },
	{ "SHIFT", "-colspan-", "SPACE", "-colspan-", "-colspan-", "-colspan-", "-colspan-", "CUR_LEFT", "CUR_RIGHT", "RESET", "-colspan-", "CANCEL", "-colspan-" }
};

std::vector<std::vector<const char*>> kbFr {
	{ "&", "é", "\"", "'", "(", "#", "è", "!", "ç", "à", ")", "-", "DEL" },
	{ "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "@", "_", "DEL" },
	{ "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "@", "_", "DEL" },

	{ "a", "z", "e", "r", "t", "y", "u", "i", "o", "p", "^", "$", "OK" },
	{ "A", "Z", "E", "R", "T", "Y", "U", "I", "O", "P", "¨", "*", "OK" },
	{ "à", "ä", "ë", "ì", "ï", "ò", "ö", "ü", "\\", "|", "§", "°", "OK" },

	{ "q", "s", "d", "f", "g", "h", "j", "k", "l", "m", "ù", "`", "-rowspan-" },
	{ "Q", "S", "D", "F", "G", "H", "J", "K", "L", "M", "%", "£", "-rowspan-" },
	{ "á", "â", "ê", "í", "î", "ó", "ô", "ú", "û", "ñ", "¡", "¿", "-rowspan-" },

	{ "<", "w", "x", "c", "v", "b", "n", ",", ";", ":", "=", "ALT", "-colspan-" },
	{ ">", "W", "X", "C", "V", "B", "N", "?", ".", "/", "+", "ALT", "-colspan-" },
	{ "€", "", "", "", "", "", "", "", "", "", "", "ALT", "-colspan-" },

	{ "SHIFT", "-colspan-", "SPACE", "-colspan-", "-colspan-", "-colspan-", "-colspan-", "CUR_LEFT", "CUR_RIGHT", "RESET", "-colspan-", "CANCEL", "-colspan-" },
	{ "SHIFT", "-colspan-", "SPACE", "-colspan-", "-colspan-", "-colspan-", "-colspan-", "CUR_LEFT", "CUR_RIGHT", "RESET", "-colspan-", "CANCEL", "-colspan-" },
	{ "SHIFT", "-colspan-", "SPACE", "-colspan-", "-colspan-", "-colspan-", "-colspan-", "CUR_LEFT", "CUR_RIGHT", "RESET", "-colspan-", "CANCEL", "-colspan-" }
};

std::vector<std::vector<const char*>> kbEs {
	{ "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "_", "+", "DEL" },
	{ "!", "@", "#", "$", "%", "^", "&", "*", "(", ")", "-", "=", "DEL" },
	{ "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "_", "+", "DEL" },

	{ "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "[", "]", "OK" },
	{ "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "{", "}", "OK" },
	{ "à", "ä", "é", "ë", "ì", "ï", "ú", "í", "ó", "ò", "¿", "?", "OK" },

	{ "a", "s", "d", "f", "g", "h", "j", "k", "l", "ñ", ";", ":", "-rowspan-" },
	{ "A", "S", "D", "F", "G", "H", "J", "K", "L", "Ñ", "'", "\"", "-rowspan-" },
	{ "á", "â", "è", "ê", "ö", "î", "ù", "ô", "ü", "û", "|", "€", "-rowspan-" },

	{ "<", "z", "x", "c", "v", "b", "n", "m", ",", ".", "¡", "ALT", "-colspan-" },
	{ ">", "Z", "X", "C", "V", "B", "N", "M", "~", "`", "\\", "ALT", "-colspan-" },
	{ "£", "$", "¨", "/", "º", "ª", "@", "#", "*", "", "", "ALT", "-colspan-" },

	{ "SHIFT", "-colspan-", "SPACE", "-colspan-", "-colspan-", "-colspan-", "-colspan-", "CUR_LEFT", "CUR_RIGHT", "RESET", "-colspan-", "CANCEL", "-colspan-" },
	{ "SHIFT", "-colspan-", "SPACE", "-colspan-", "-colspan-", "-colspan-", "-colspan-", "CUR_LEFT", "CUR_RIGHT", "RESET", "-colspan-", "CANCEL", "-colspan-" },
	{ "SHIFT", "-colspan-", "SPACE", "-colspan-", "-colspan-", "-colspan-", "-colspan-", "CUR_LEFT", "CUR_RIGHT", "RESET", "-colspan-", "CANCEL", "-colspan-" }
};


GuiTextEditPopupKeyboard::GuiTextEditPopupKeyboard(Window* window, const std::string& title, const std::string& initValue,
	const std::function<bool(const std::string&)>& okCallback, bool multiLine, const std::string acceptBtnText)
	: GuiComponent(window), mBackground(window, ":/frame.png"), mGrid(window, Vector2i(1, 6)), mMultiLine(multiLine)
{
	setTag("popup");

	mOkCallback = okCallback;

	auto theme = ThemeData::getMenuTheme();
	mBackground.setImagePath(theme->Background.path);
	mBackground.setEdgeColor(theme->Background.color);
	mBackground.setCenterColor(theme->Background.centerColor);
	mBackground.setCornerSize(theme->Background.cornerSize);

	addChild(&mBackground);
	addChild(&mGrid);

	mTitle = std::make_shared<TextComponent>(mWindow, Utils::String::toUpper(title), theme->Title.font, theme->Title.color, ALIGN_CENTER);

	mKeyboardGrid = std::make_shared<ComponentGrid>(mWindow, Vector2i(kbUs[0].size(), kbUs.size() / 3));

	mText = std::make_shared<TextEditComponent>(mWindow);
	mText->setValue(initValue);

	if (!multiLine)
		mText->setCursor(initValue.size());

	// Header
	mGrid.setEntry(mTitle, Vector2i(0, 0), false, true);

	// Text edit add
	mGrid.setEntry(mText, Vector2i(0, 1), true, false, Vector2i(1, 1), GridFlags::BORDER_TOP);

	std::vector< std::vector< std::shared_ptr<ButtonComponent> > > buttonList;

	// Keyboard
	// Case for if multiline is enabled, then don't create the keyboard.
	if (!mMultiLine)
	{
		std::vector<std::vector<const char*>>* layout = &kbUs;

		std::string language = SystemConf::getInstance()->get("system.language");
		if (!language.empty())
		{
			auto shortNameDivider = language.find("_");
			if (shortNameDivider != std::string::npos)
				language = Utils::String::toLower(language.substr(0, shortNameDivider));
		}

		if (language == "fr")
			layout = &kbFr;
		else if (language == "es")
			layout = &kbEs;

		for (unsigned int i = 0; i < layout->size() / 3; i++)
		{
			std::vector<std::shared_ptr<ButtonComponent>> buttons;
			for (unsigned int j = 0; j < (*layout)[i].size(); j++)
			{
				std::string lower = (*layout)[3 * i][j];
				if (lower.empty() || lower == "-rowspan-" || lower == "-colspan-")
					continue;

				std::string upper = (*layout)[3 * i + 1][j];
				std::string alted = (*layout)[3 * i + 2][j];

				std::shared_ptr<ButtonComponent> button = nullptr;

				if (lower == "DEL")
				{
					lower = _U("\u232B");
					upper = _U("\u232B");
					alted = _U("\u232B");
				}
				else if (lower == "OK")
				{
					lower = _U("\u23CE");
					upper = _U("\u23CE");
					alted = _U("\u23CE");
				}
				else if (lower != "SHIFT" && lower.length() > 1)
				{
					lower = _(lower.c_str());
					upper = _(upper.c_str());
					alted = _(alted.c_str());
				}

				if (lower == "SHIFT")
				{
					// Special case for shift key
					mShiftButton = std::make_shared<ButtonComponent>(mWindow, _U("\u21E7"), _("SHIFTS FOR UPPER,LOWER, AND SPECIAL"), [this] { shiftKeys(); }, false);
					button = mShiftButton;
				}
				else if (lower == "ALT")
				{
					mAltButton = std::make_shared<ButtonComponent>(mWindow, _U("\u2387"), _("ALT GR"), [this] { altKeys(); }, false);
					button = mAltButton;
				}
				else if (lower == "CUR_LEFT")
				{
					mInputCursorLeft = std::make_shared<ButtonComponent>(mWindow, _U("\u25C4"), _("MOVE CURSOR LEFT"), [this] { mText->moveCursor(-1); }, false);
					button = mInputCursorLeft;
				}
				else if (lower == "CUR_RIGHT")
				{
					mInputCursorRight = std::make_shared<ButtonComponent>(mWindow, _U("\u25BA"), _("MOVE CURSOR RIGHT"), [this] { mText->moveCursor(+1); }, false);
					button = mInputCursorRight;
				}
				else
					button = makeButton(lower, upper, alted);

				button->setPadding(Vector4f(BUTTON_GRID_HORIZ_PADDING / 4.0f, BUTTON_GRID_HORIZ_PADDING / 4.0f, BUTTON_GRID_HORIZ_PADDING / 4.0f, BUTTON_GRID_HORIZ_PADDING / 4.0f));
				button->setRenderNonFocusedBackground(false);
				buttons.push_back(button);

				int colSpan = 1;
				for (unsigned int cs = j + 1; cs < (*layout)[i].size(); cs++)
				{
					if (std::string((*layout)[3 * i][cs]) == "-colspan-")
						colSpan++;
					else
						break;
				}

				int rowSpan = 1;
				for (unsigned int cs = (3 * i) + 3; cs < layout->size(); cs += 3)
				{
					if (std::string((*layout)[cs][j]) == "-rowspan-")
						rowSpan++;
					else
						break;
				}

				mKeyboardGrid->setEntry(button, Vector2i(j, i), true, true, Vector2i(colSpan, rowSpan));

				buttonList.push_back(buttons);
			}
		}
		// END KEYBOARD IF
	}

	// Add keyboard keys
	mGrid.setEntry(mKeyboardGrid, Vector2i(0, 2), true, true, Vector2i(2, 4));

	// Determine size from text size
	float textHeight = mText->getFont()->getHeight();
	if (multiLine)
		textHeight *= 6;

	mText->setSize(0, textHeight);

	mGrid.setUnhandledInputCallback([this](InputConfig* config, Input input) -> bool
	{
		if (config->isMappedLike("down", input))
		{
			mGrid.setCursorTo(mText);
			return true;
		}
		else if (config->isMappedLike("up", input))
		{
			mGrid.moveCursor(Vector2i(0, kbUs.size() / 2));
			return true;
		}
		else if (config->isMappedLike("left", input))
		{
			if (mGrid.getSelectedComponent() == mKeyboardGrid)
			{
				mKeyboardGrid->moveCursor(Vector2i(kbUs[0].size() - 1, 0));
				return true;
			}
		}
		else if (config->isMappedLike("right", input))
		{
			if (mGrid.getSelectedComponent() == mKeyboardGrid)
			{
				mKeyboardGrid->moveCursor(Vector2i(-kbUs[0].size() + 1, 0));
				return true;
			}
		}

		return false;
	});


	// If multiline, set all diminsions back to default, else draw size for keyboard.
	if (mMultiLine)
	{
		if (Renderer::isSmallScreen())
			setSize(OSK_WIDTH, Renderer::getScreenHeight());
		else
		{
			float width = OSK_WIDTH,
						height = mTitle->getFont()->getHeight() + textHeight + mKeyboardGrid->getSize().y() + 40;

			if (Settings::getInstance()->getBool("MenusAllHeight"))
			{
				bool change_height_ratio = Settings::getInstance()->getBool("ShowHelpPrompts");
				float height_ratio = 1.0f;
				if ( change_height_ratio )
				{
					height_ratio = 0.88f;
					if ( Settings::getInstance()->getBool("MenusOnDisplayTop") || Settings::getInstance()->getBool("MenusAllHeight") )
						height_ratio = 0.93f;
				}
				height = Renderer::getScreenHeight() * height_ratio;
			}

			if (Settings::getInstance()->getBool("MenusAllWidth"))
				width = Renderer::getScreenWidth();
			else if (Settings::getInstance()->getBool("AutoMenuWidth"))
			{
				float font_size = ThemeData::getMenuTheme()->Text.font->getSize(),
							ratio = 1.2f;

				if ((font_size >= FONT_SIZE_SMALL) && (font_size < FONT_SIZE_MEDIUM))
					ratio = 1.4f;
				else if ((font_size >= FONT_SIZE_MEDIUM) && (font_size < FONT_SIZE_LARGE))
					ratio = 1.7f;
				else if ((font_size >= FONT_SIZE_LARGE))
					ratio = 2.0f;

				width = (float)Math::min((int)(width * ratio), Renderer::getScreenWidth());
			}

			setSize(width, height);
		}

		float new_x = (Renderer::getScreenWidth() - mSize.x()) / 2,
					new_y = (Renderer::getScreenHeight() - mSize.y()) / 2;

		if (Settings::getInstance()->getBool("MenusAllWidth"))
			new_x = 0.f;
		if (Settings::getInstance()->getBool("MenusOnDisplayTop") || Settings::getInstance()->getBool("MenusAllHeight"))
			new_y = 0.f;

		setPosition(new_x, new_y);
	}
	else
	{
		//setSize(OSK_WIDTH, mTitle->getFont()->getHeight() + textHeight + 40 + (Renderer::getScreenHeight() * 0.085f) * 6);

			float width = OSK_WIDTH,
						height = OSK_HEIGHT;

			if (Settings::getInstance()->getBool("MenusAllHeight"))
			{
				bool change_height_ratio = Settings::getInstance()->getBool("ShowHelpPrompts");
				float height_ratio = 1.0f;
				if ( change_height_ratio )
				{
					height_ratio = 0.88f;
					if ( Settings::getInstance()->getBool("MenusOnDisplayTop") || Settings::getInstance()->getBool("MenusAllHeight") )
						height_ratio = 0.93f;
				}
				height = Renderer::getScreenHeight() * height_ratio;
			}

			if (Settings::getInstance()->getBool("MenusAllWidth"))
				width = Renderer::getScreenWidth();
			else if (Settings::getInstance()->getBool("AutoMenuWidth"))
			{
				float font_size = ThemeData::getMenuTheme()->Text.font->getSize(),
							ratio = 1.2f;

				if ((font_size >= FONT_SIZE_SMALL) && (font_size < FONT_SIZE_MEDIUM))
					ratio = 1.4f;
				else if ((font_size >= FONT_SIZE_MEDIUM) && (font_size < FONT_SIZE_LARGE))
					ratio = 1.7f;
				else if ((font_size >= FONT_SIZE_LARGE))
					ratio = 2.0f;

				width = (float)Math::min((int)(width * ratio), Renderer::getScreenWidth());
			}

		setSize(width, height);

		float new_x = (Renderer::getScreenWidth() - mSize.x()) / 2,
					new_y = (Renderer::getScreenHeight() - mSize.y()) / 2;

		if (Settings::getInstance()->getBool("MenusAllWidth"))
			new_x = 0.f;
		if (Settings::getInstance()->getBool("MenusOnDisplayTop") || Settings::getInstance()->getBool("MenusAllHeight"))
			new_y = 0.f;

		setPosition(new_x, new_y);
		animateTo(Vector2f(new_x, new_y));
	}
}


void GuiTextEditPopupKeyboard::onSizeChanged()
{
	mBackground.fitTo(mSize, Vector3f::Zero(), Vector2f(-32, -32));

	mText->setSize(mSize.x() - OSK_PADDINGX - OSK_PADDINGX, mText->getSize().y());

	// update grid
	mGrid.setRowHeightPerc(0, mTitle->getFont()->getHeight() / mSize.y());
	mGrid.setRowHeightPerc(2, mKeyboardGrid->getSize().y() / mSize.y());
	mGrid.setSize(mSize);

	auto pos = mKeyboardGrid->getPosition();
	auto sz = mKeyboardGrid->getSize();

	mKeyboardGrid->setSize(mSize.x() - OSK_PADDINGX - OSK_PADDINGX, sz.y() - OSK_PADDINGY); // Small margin between buttons
	mKeyboardGrid->setPosition(OSK_PADDINGX, pos.y());
}

bool GuiTextEditPopupKeyboard::input(InputConfig* config, Input input)
{
	if (GuiComponent::input(config, input))
		return true;

	if ((config->getDeviceId() == DEVICE_KEYBOARD && input.id == SDLK_ESCAPE))
	{
		delete this;
		return true;
	}

	if (input.value)
	{
		// pressing start
		if (config->isMappedTo("start", input))
		{
			if ((mOkCallback != nullptr) && mOkCallback(mText->getValue()))
				delete this;

			return true;
		}

		// pressing back when not text editing closes us
		if (config->isMappedTo(BUTTON_BACK, input))
		{
			delete this;
			return true;
		}

		// For deleting a chara (Left Top Button)
		if (config->isMappedLike(BUTTON_PU, input)) {
			bool editing = mText->isEditing();
			if (!editing)
				mText->startEditing();

			mText->textInput("\b");

			if (!editing)
				mText->stopEditing();
		}

		// For Adding a space (Right Top Button)
		if (config->isMappedLike(BUTTON_PD, input))
		{
			bool editing = mText->isEditing();
			if (!editing)
				mText->startEditing();

			mText->textInput(" ");

			if (!editing)
				mText->stopEditing();
		}

		// For Shifting (Y)
		if (config->isMappedTo("y", input))
			shiftKeys();

		if (config->isMappedTo("x", input))
		{
			bool editing = mText->isEditing();
			if (!editing)
				mText->startEditing();

			mText->setValue("");

			if (!editing)
				mText->stopEditing();
		}

		if ( (!Settings::getInstance()->getBool("InvertButtonsPU") && config->isMappedTo(BUTTON_L1, input))
				|| (Settings::getInstance()->getBool("InvertButtonsPU") && config->isMappedTo(BUTTON_L2, input)) )
			mText->moveCursor(-1);

		if ( (!Settings::getInstance()->getBool("InvertButtonsPD") && config->isMappedTo(BUTTON_R1, input))
				|| (Settings::getInstance()->getBool("InvertButtonsPD") && config->isMappedTo(BUTTON_R2, input)) )
			mText->moveCursor(1);
	}
	return false;
}

// Shifts the keys when user hits the shift button.
void GuiTextEditPopupKeyboard::shiftKeys()
{
	if (mAlt && !mShift)
		altKeys();

	mShift = !mShift;

	if (mShift)
	{
		mShiftButton->setRenderNonFocusedBackground(true);
		mShiftButton->setColorShift(0xFF0000FF);
	}
	else
	{
		mShiftButton->setRenderNonFocusedBackground(false);
		mShiftButton->removeColorShift();
	}

	for (auto & kb : keyboardButtons)
	{
		const std::string& text = mShift ? kb.shiftedKey : kb.key;
		auto sz = kb.button->getSize();
		kb.button->setText(text, text, false);
		kb.button->setSize(sz);
	}
}

void GuiTextEditPopupKeyboard::altKeys()
{
	if (mShift && !mAlt)
		shiftKeys();

	mAlt = !mAlt;

	if (mAlt)
	{
		mAltButton->setRenderNonFocusedBackground(true);
		mAltButton->setColorShift(0xFF0000FF);
	}
	else
	{
		mAltButton->setRenderNonFocusedBackground(false);
		mAltButton->removeColorShift();
	}

	for (auto & kb : keyboardButtons)
	{
		const std::string& text = mAlt ? kb.altedKey : kb.key;
		auto sz = kb.button->getSize();
		kb.button->setText(text, text, false);
		kb.button->setSize(sz);
	}
}

std::vector<HelpPrompt> GuiTextEditPopupKeyboard::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts = mGrid.getHelpPrompts();

	if (mOkCallback != nullptr)
		prompts.push_back(HelpPrompt("x", _("RESET")));

	prompts.push_back(HelpPrompt("y", _("SHIFT")));
	prompts.push_back(HelpPrompt("start", _("OK")));
	prompts.push_back(HelpPrompt(BUTTON_BACK, _("BACK")));
	prompts.push_back(HelpPrompt(BUTTON_R2, _("SPACE")));
	prompts.push_back(HelpPrompt(BUTTON_L2, _("DELETE")));
	return prompts;
}

std::shared_ptr<ButtonComponent> GuiTextEditPopupKeyboard::makeButton(const std::string& key, const std::string& shiftedKey, const std::string& altedKey)
{
	std::shared_ptr<ButtonComponent> button = std::make_shared<ButtonComponent>(mWindow, key, key, [this, key, shiftedKey, altedKey]
	{
		if (key == _U("\u23CE") || key == "OK")
		{
			if (mOkCallback(mText->getValue()))
				delete this;

			return;
		}
		else if (key == _U("\u232B") || key == "DEL")
		{
			mText->startEditing();
			mText->textInput("\b");
			mText->stopEditing();
			return;
		}
		else if (key == _("SPACE") || key == " ")
		{
			mText->startEditing();
			mText->textInput(" ");
			mText->stopEditing();
			return;
		}
		else if (key == _("RESET"))
		{
			mText->startEditing();
			mText->setValue("");
			mText->stopEditing();
			return;
		}
		else if (key == _("CANCEL"))
		{
			delete this;
			return;
		}

		if (mAlt && altedKey.empty())
			return;

		mText->startEditing();

		if (mAlt)
			mText->textInput(altedKey.c_str());
		else if (mShift)
			mText->textInput(shiftedKey.c_str());
		else
			mText->textInput(key.c_str());

		mText->stopEditing();
	}, false);

	KeyboardButton kb(button, key, shiftedKey, altedKey);
	keyboardButtons.push_back(kb);
	return button;
}
