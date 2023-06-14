#include "guis/GuiInputConfig.h"

#include "utils/StringUtil.h"
#include "components/ButtonComponent.h"
#include "components/MenuComponent.h"
#include "guis/GuiMsgBox.h"
#include "InputManager.h"
#include "Log.h"
#include "Window.h"
#include "EsLocale.h"
#include "Scripting.h"

//MasterVolUp and MasterVolDown are also hooked up, but do not appear on this screen.
//If you want, you can manually add them to es_input.cfg.

#define HOLD_TO_SKIP_MS 1000

void GuiInputConfig::initInputConfigStructure(bool isXboxController, bool isPsController)
{
	GUI_INPUT_CONFIG_LIST =
	{
		{ "a",                false, InputConfig::buttonLabel("a", isXboxController, isPsController), InputConfig::buttonImage("a", isXboxController, isPsController) },
		{ "b",                true,  InputConfig::buttonLabel("b", isXboxController, isPsController), InputConfig::buttonImage("b", isXboxController, isPsController) },
		{ "x",                true,  InputConfig::buttonLabel("x", isXboxController, isPsController), InputConfig::buttonImage("x", isXboxController, isPsController) },
		{ "y",                true,  InputConfig::buttonLabel("y", isXboxController, isPsController), InputConfig::buttonImage("y", isXboxController, isPsController) },

		{ "start",            true,  "START",                          ":/help/button_start.svg" },
		{ "select",           true,  "SELECT",                         ":/help/button_select.svg" },

		{ "up",               false, "UP",                             ":/help/dpad_up.svg" },
		{ "down",             false, "DOWN",                           ":/help/dpad_down.svg" },
		{ "left",             false, "LEFT",                           ":/help/dpad_left.svg" },
		{ "right",            false, "RIGHT",                          ":/help/dpad_right.svg" },

		{ "l1",               true,  "L1",                             ":/help/button_l.svg" },
		{ "r1",               true,  "R1",                             ":/help/button_r.svg" },

		{ "joystick1up",      true,  "LEFT ANALOG UP",                 ":/help/analog_up.svg" },
		{ "joystick1left",    true,  "LEFT ANALOG LEFT",               ":/help/analog_left.svg" },
		{ "joystick2up",      true,  "RIGHT ANALOG UP",                ":/help/analog_up.svg" },
		{ "joystick2left",    true,  "RIGHT ANALOG LEFT",              ":/help/analog_left.svg" },

		{ "l2",               true,  "L2",                             ":/help/button_lt.svg" },
		{ "r2",               true,  "R2",                             ":/help/button_rt.svg" },
		{ "l3",               true,  "L3",                             ":/help/analog_thumb.svg" },
		{ "r3",               true,  "R3",                             ":/help/analog_thumb.svg" },

		{ "hotkey",           true,  "HOTKEY",      ":/help/button_hotkey.svg" }
	};
}

GuiInputConfig::GuiInputConfig(Window* window, InputConfig* target, bool reconfigureAll, const std::function<void()>& okCallback) : GuiComponent(window),
	mBackground(window, ":/frame.png"), mGrid(window, Vector2i(1, 6)),
	mTargetConfig(target), mHoldingInput(false), mBusyAnim(window)
{
	initInputConfigStructure(isXboxController(target), isPsController(target));

	auto theme = ThemeData::getMenuTheme();
	mBackground.setImagePath(theme->Background.path); // ":/frame.png"
	mBackground.setEdgeColor(theme->Background.color);
	mBackground.setCenterColor(theme->Background.centerColor);
	mBackground.setCornerSize(theme->Background.cornerSize);

	mGrid.setSeparatorColor(theme->Text.separatorColor);

	LOG(LogInfo) << "Configuring device " << target->getDeviceId() << " (" << target->getDeviceName() << "), default input: " << Utils::String::boolToString(target->isDefaultInput()) <<  '.';

	if(reconfigureAll)
		target->clear();

	mConfiguringAll = reconfigureAll;
	mConfiguringRow = mConfiguringAll;

	addChild(&mBackground);
	addChild(&mGrid);

	// 0 is a spacer row
	mGrid.setEntry(std::make_shared<GuiComponent>(mWindow), Vector2i(0, 0), false);

	mTitle = std::make_shared<TextComponent>(mWindow, _("CONFIGURING"), Font::get(FONT_SIZE_LARGE), 0x555555FF, ALIGN_CENTER);
	mGrid.setEntry(mTitle, Vector2i(0, 0), false, true);

	char strbuf[256];
	if(target->getDeviceId() == DEVICE_KEYBOARD)
	  strncpy(strbuf, _("KEYBOARD").c_str(), 256); 
	else if(target->getDeviceId() == DEVICE_CEC)
	  strncpy(strbuf, _("CEC").c_str(), 256); 
	else {
	  snprintf(strbuf, 256, _("GAMEPAD %i").c_str(), target->getDeviceId() + 1); 
	}

	// get device name
	std::string name = target->getDeviceName();
	if (Settings::getInstance()->getBool("bluetooth.use.alias"))
	{
		std::string alias = Settings::getInstance()->getString(name + ".bluetooth.input_gaming.alias");
		if (!alias.empty())
			name = alias;
	}

	mSubtitle1 = std::make_shared<TextComponent>(mWindow, Utils::String::toUpper(std::string(strbuf).append(" - ").append(name)), theme->Text.font, theme->Title.color, ALIGN_CENTER);
	mGrid.setEntry(mSubtitle1, Vector2i(0, 1), false, true);

	mSubtitle2 = std::make_shared<TextComponent>(mWindow, _("HOLD ANY BUTTON TO SKIP"), theme->TextSmall.font, theme->TextSmall.color, ALIGN_CENTER);
	mGrid.setEntry(mSubtitle2, Vector2i(0, 2), false, true);

	// 3 is a spacer row

	mList = std::make_shared<ComponentList>(mWindow);
	mGrid.setEntry(mList, Vector2i(0, 4), true, true);
	for(int i = 0; i < GUI_INPUT_CONFIG_LIST.size(); i++)
	{
		ComponentListRow row;

		// icon
		auto icon = std::make_shared<ImageComponent>(mWindow);
		icon->setImage(GUI_INPUT_CONFIG_LIST[i].icon);
		icon->setColorShift(ThemeData::getMenuTheme()->Text.color);
		icon->setResize(0, Font::get(FONT_SIZE_MEDIUM)->getLetterHeight() * 1.25f);
		row.addElement(icon, false);

		// spacer between icon and text
		auto spacer = std::make_shared<GuiComponent>(mWindow);
		spacer->setSize(16, 0);
		row.addElement(spacer, false);

		std::string input_name = Utils::String::toUpper(GUI_INPUT_CONFIG_LIST[i].dispName);
		if ((input_name != "SELECT") && (input_name != "START"))
			input_name = _(input_name);

		auto text = std::make_shared<TextComponent>(mWindow, input_name, theme->Text.font, theme->Text.color);
		text->setAutoScroll(Settings::getInstance()->getBool("AutoscrollMenuEntries"));
		row.addElement(text, true);

		auto mapping = std::make_shared<TextComponent>(mWindow, _("-NOT DEFINED-"), theme->Text.font, theme->TextSmall.color, ALIGN_RIGHT);
		mapping->setAutoScroll(Settings::getInstance()->getBool("AutoscrollMenuEntries"));
		setNotDefined(mapping); // overrides text and color set above
		row.addElement(mapping, true);
		mMappings.push_back(mapping);

		row.input_handler = [this, i, mapping](InputConfig* config, Input input) -> bool
		{
			// ignore input not from our target device
			if(config != mTargetConfig)
				return false;

			// if we're not configuring, start configuring when A is pressed
			if(!mConfiguringRow)
			{
				if(config->isMappedTo(BUTTON_OK, input) && input.value)
				{
					mList->stopScrolling();
					mConfiguringRow = true;
					setPress(mapping);
					return true;
				}

				// we're not configuring and they didn't press A to start, so ignore this
				return false;
			}

			if (mHoldingInput)
				mAllInputs.push_back(input);

			// filter for input quirks specific to Sony DualShock 3
			if(filterTrigger(input, config))
				return false;

			// we are configuring
			if(input.value != 0)
			{
				// input down
				// if we're already holding something, ignore this, otherwise plan to map this input
				if(mHoldingInput)
					return true;

				mHoldingInput = true;
				mHeldInput = input;
				mHeldTime = 0;
				mHeldInputId = i;

				mAllInputs.clear();
				mAllInputs.push_back(input);

				return true;
			}else{
				// input up
				// make sure we were holding something and we let go of what we were previously holding
				if(!mHoldingInput || mHeldInput.device != input.device || mHeldInput.id != input.id || mHeldInput.type != input.type)
					return true;

				mHoldingInput = false;

				if (mHeldInput.type == InputType::TYPE_BUTTON)
				{
					auto altAxis = mAllInputs.where([&](Input x) { return x.device == mHeldInput.device && x.type == InputType::TYPE_AXIS; });
					if (altAxis.size() >= 2)
					{
						auto groups = altAxis.groupBy([](Input x) { return x.id; });
						if (groups.size() == 1)
							mHeldInput = altAxis[0];
					}
				}

				if(assign(mHeldInput, i))
					rowDone(); // if successful, move cursor/stop configuring - if not, we'll just try again

				mAllInputs.clear();
				return true;
			}
		};

		mList->addRow(row);
	}

	// only show "HOLD TO SKIP" if this input is skippable
	mList->setCursorChangedCallback([this](CursorState /*state*/) {
		bool skippable = GUI_INPUT_CONFIG_LIST[mList->getCursorId()].skippable;
		mSubtitle2->setOpacity(skippable * 255);
	});

	// make the first one say "PRESS ANYTHING" if we're re-configuring everything
	if(mConfiguringAll)
		setPress(mMappings.front());

	// buttons
	std::vector< std::shared_ptr<ButtonComponent> > buttons;
	std::function<void()> okFunction = [this, okCallback] {
		InputManager::getInstance()->writeDeviceConfig(mTargetConfig); // save
		if(okCallback)
			okCallback();

		Scripting::fireAsyncEvent("control-mapped", std::to_string(mTargetConfig->getDeviceId()), mTargetConfig->getDeviceName(),
							 mTargetConfig->getDeviceGUIDString(), Utils::String::boolToString(mTargetConfig->isDefaultInput()));
		delete this;
	};
	buttons.push_back(std::make_shared<ButtonComponent>(mWindow, _("OK"), _("OK"), [this, okFunction] { 
		// check if the hotkey enable button is set. if not prompt the user to use select or nothing.
		Input input;

		if (mTargetConfig->isDefaultInput()) // system_hk is F button
			mTargetConfig->mapInput("system_hk", Input(mTargetConfig->getDeviceId(), TYPE_BUTTON, 10, 1, true));

		if (!mTargetConfig->getInputByName("hotkey", &input)) { 
			mWindow->pushGui(new GuiMsgBox(mWindow,
				_("NO HOTKEY BUTTON HAS BEEN ASSIGNED. THIS IS REQUIRED FOR EXITING GAMES WITH A CONTROLLER. DO YOU WANT TO USE THE SELECT BUTTON AS YOUR HOTKEY?"),  
				_("SET SELECT AS HOTKEY"), [this, okFunction] { 
					Input input;
					mTargetConfig->getInputByName("Select", &input);
					mTargetConfig->mapInput("hotkey", input); 
					okFunction();
					},
				_("DO NOT ASSIGN HOTKEY"), [this, okFunction] { 
					// for a disabled hotkey enable button, set to a key with id 0,
					// so the input configuration script can be backwards compatible.
					mTargetConfig->mapInput("hotkey", Input(DEVICE_KEYBOARD, TYPE_KEY, 0, 1, true)); 
					okFunction();
				}
			));
		} else {
			okFunction();
		}
	}));
	buttons.push_back(std::make_shared<ButtonComponent>(mWindow, _("CANCEL"), _("CANCEL"), [this] { delete this; }));
	mButtonGrid = makeButtonGrid(mWindow, buttons);
	mGrid.setEntry(mButtonGrid, Vector2i(0, 5), true, false);

	float width = Renderer::getScreenWidth(),
		  height = Renderer::getScreenHeight(),
		  x = 0.f,
		  y = 0.f;
	if (!Renderer::isSmallScreen())
	{
		width = width * 0.6f;
		height = height * 0.75f;
		x = (Renderer::getScreenWidth() - mSize.x()) / 2;
		y = (Renderer::getScreenHeight() - mSize.y()) / 2;
	}

	setSize(width, height);
	setPosition(x, y);
}

void GuiInputConfig::onSizeChanged()
{
	GuiComponent::onSizeChanged();

	mBackground.fitTo(mSize, Vector3f::Zero(), Vector2f(-32, -32));

	// update grid
	mGrid.setSize(mSize);
	
	float h = (mTitle->getFont()->getHeight() + // *0.75f
		mSubtitle1->getFont()->getHeight() +
		mSubtitle2->getFont()->getHeight() +
		0.03f +
		mButtonGrid->getSize().y()) / mSize.y();

	int cnt = (1.0 - h) / (mList->getRowHeight(0) / mSize.y());

	mGrid.setRowHeightPerc(0, mTitle->getFont()->getHeight() / mSize.y()); // *0.75f
	mGrid.setRowHeightPerc(1, mSubtitle1->getFont()->getHeight() / mSize.y());
	mGrid.setRowHeightPerc(2, mSubtitle2->getFont()->getHeight() / mSize.y());
	//mGrid.setRowHeightPerc(3, 0.03f);
	mGrid.setRowHeightPerc(4, (mList->getRowHeight(0) * cnt + 2) / mSize.y());
	mGrid.setRowHeightPerc(5, mButtonGrid->getSize().y() / mSize.y());

	mBusyAnim.setSize(mSize);
}

void GuiInputConfig::update(int deltaTime)
{
	if(mConfiguringRow && mHoldingInput && GUI_INPUT_CONFIG_LIST[mHeldInputId].skippable)
	{
		int prevSec = mHeldTime / 1000;
		mHeldTime += deltaTime;
		int curSec = mHeldTime / 1000;

		if(mHeldTime >= HOLD_TO_SKIP_MS)
		{
			setNotDefined(mMappings.at(mHeldInputId));
			clearAssignment(mHeldInputId);
			mHoldingInput = false;
			rowDone();
		}else{
			if(prevSec != curSec)
			{
				// crossed the second boundary, update text
				int hold_time = HOLD_TO_SKIP_MS/1000 - curSec;
				const auto& text = mMappings.at(mHeldInputId);
				char strbuf[256];
				snprintf(strbuf, 256, EsLocale::nGetText("HOLD FOR %iS TO SKIP", "HOLD FOR %iS TO SKIP", hold_time).c_str(), hold_time); 
				text->setText(strbuf);
				text->setColor(ThemeData::getMenuTheme()->Text.color);
			}
		}
	}
}

// move cursor to the next thing if we're configuring all,
// or come out of "configure mode" if we were only configuring one row
void GuiInputConfig::rowDone()
{
	if(mConfiguringAll)
	{
		if(!mList->moveCursor(1)) // try to move to the next one
		{
			// at bottom of list, done
			mConfiguringAll = false;
			mConfiguringRow = false;
			mGrid.moveCursor(Vector2i(0, 1));
		}else{
			// on another one
			setPress(mMappings.at(mList->getCursorId()));
		}
	}else{
		// only configuring one row, so stop
		mConfiguringRow = false;
	}
}

void GuiInputConfig::setPress(const std::shared_ptr<TextComponent>& text)
{
	text->setText(_("PRESS ANYTHING"));
	text->setAutoScroll(Settings::getInstance()->getBool("AutoscrollMenuEntries"));
	text->setColor(0x656565FF);
}

void GuiInputConfig::setNotDefined(const std::shared_ptr<TextComponent>& text)
{
	text->setText(_("-NOT DEFINED-"));
	text->setAutoScroll(Settings::getInstance()->getBool("AutoscrollMenuEntries"));
	text->setColor(0x999999FF);
}

void GuiInputConfig::setAssignedTo(const std::shared_ptr<TextComponent>& text, Input input)
{
	text->setText(_(Utils::String::toUpper(input.string())));
	text->setAutoScroll(Settings::getInstance()->getBool("AutoscrollMenuEntries"));
	text->setColor(ThemeData::getMenuTheme()->Text.color);
}

void GuiInputConfig::error(const std::shared_ptr<TextComponent>& text, const std::string& msg)
{
	text->setText(_(msg));
	text->setAutoScroll(Settings::getInstance()->getBool("AutoscrollMenuEntries"));
	text->setColor(0x656565FF);
}

bool GuiInputConfig::assign(Input input, int inputId)
{
	// input is from InputConfig* mTargetConfig

	// if this input is mapped to something other than "nothing" or the current row, error
	// (if it's the same as what it was before, allow it)
	if (mTargetConfig->getMappedTo(input).size() > 0 && 
		!mTargetConfig->isMappedTo(GUI_INPUT_CONFIG_LIST[inputId].name, input) && 
		GUI_INPUT_CONFIG_LIST[inputId].name != "hotkey") 
	{
		error(mMappings.at(inputId), "ALREADY TAKEN");
		return false;
	}

	setAssignedTo(mMappings.at(inputId), input);

	input.configured = true;
	mTargetConfig->mapInput(GUI_INPUT_CONFIG_LIST[inputId].name, input);

	LOG(LogInfo) << "GuiInputConfig::assign() - Mapping [" << input.string() << "] -> " << GUI_INPUT_CONFIG_LIST[inputId].name;

	return true;
}

void GuiInputConfig::clearAssignment(int inputId)
{
	mTargetConfig->unmapInput(GUI_INPUT_CONFIG_LIST[inputId].name);
}

bool GuiInputConfig::filterTrigger(Input input, InputConfig* config)
{
/*
	// on Linux, some gamepads return both an analog axis and a digital button for the trigger;
	// we want the analog axis only, so this function removes the button press event

	if((
	  // match PlayStation joystick with 6 axes only
	  strstr(config->getDeviceName().c_str(), "PLAYSTATION") != NULL
	  || strstr(config->getDeviceName().c_str(), "PS3 Ga") != NULL
	  || strstr(config->getDeviceName().c_str(), "PS(R) Ga") != NULL
	  // BigBen kid's PS3 gamepad 146b:0902, matched on SDL GUID because its name "Bigben Interactive Bigben Game Pad" may be too generic
	  || strcmp(config->getDeviceGUIDString().c_str(), "030000006b1400000209000011010000") == 0
	  ) && InputManager::getInstance()->getAxisCountByDevice(config->getDeviceId()) == 6)
	{
		// digital triggers are unwanted
		if (input.type == TYPE_BUTTON && (input.id == 6 || input.id == 7))
			return true;
		// ignore analog values < 0
		if (input.type == TYPE_AXIS && (input.id == 2 || input.id == 5) && input.value < 0)
			return true;
	}
*/
	return false;
}

bool GuiInputConfig::isXboxController(InputConfig* config)
{
	if (Utils::String::contains(config->getDeviceName(), "Xbox") || Utils::String::contains(config->getDeviceName(), "X-Box")
	  || Utils::String::contains(config->getDeviceName(), "Microsoft") || Utils::String::contains(config->getDeviceGUIDString(), "0000005e040000"))
		return true;

	return false;
}

bool GuiInputConfig::isPsController(InputConfig* config)
{
	if (Utils::String::contains(config->getDeviceName(), "PLAYSTATION") || Utils::String::contains(config->getDeviceName(), "PS3 Ga")
	  || Utils::String::contains(config->getDeviceName(), "PS(R) Ga") || Utils::String::contains(config->getDeviceName(), "Sony")
	  || Utils::String::contains(config->getDeviceGUIDString(), "0000004c050000") || Utils::String::contains(config->getDeviceGUIDString(), "0000006b140000"))
		return true;

	return false;
}
