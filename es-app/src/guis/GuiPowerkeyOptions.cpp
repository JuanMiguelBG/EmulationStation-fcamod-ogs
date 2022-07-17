#include "guis/GuiPowerkeyOptions.h"

#include "components/SliderComponent.h"
#include "components/SwitchComponent.h"
#include "components/OptionListComponent.h"
#include "Window.h"
#include "ApiSystem.h"


GuiPowerkeyOptions::GuiPowerkeyOptions(Window* window) : GuiSettings(window, _("POWERKEY BUTTON SETTINGS").c_str())
{
	initializeMenu();
}

GuiPowerkeyOptions::~GuiPowerkeyOptions()
{
}

void GuiPowerkeyOptions::initializeMenu()
{
	// powerkey action
	std::string powerkey_action_value = ApiSystem::getInstance()->getPowerkeyAction();
	auto powerkey_list = std::make_shared< OptionListComponent< std::string > >(mWindow, _("ACTION TO EXECUTE"), false);

	powerkey_list->add(_("shutdown"), "shutdown", powerkey_action_value == "shutdown");
	powerkey_list->add(_("suspend"), "suspend", powerkey_action_value == "suspend");
	powerkey_list->add(_("disabled"), "disabled", powerkey_action_value == "disabled");
	addWithLabel(_("ACTION TO EXECUTE"), powerkey_list);

	if (!powerkey_list->hasSelection())
	{
		powerkey_list->selectFirstItem();
		powerkey_action_value = powerkey_list->getSelected();
	}

	// push powerkey double to shutdown
	bool double_push_value = ApiSystem::getInstance()->isPowerkeyState();
	auto double_push = std::make_shared<SwitchComponent>(mWindow, double_push_value );
	addWithLabel(_("PUSH TWO TIMES TO EXECUTE ACTION"), double_push);

	// max interval time
	auto time_interval = std::make_shared<SliderComponent>(mWindow, 1.f, 10.f, 1.f, "s", true);
	int time_interval_value = ApiSystem::getInstance()->getPowerkeyTimeInterval();
	time_interval->setValue( (float) time_interval_value );
	addWithLabel(_("TIME INTERVAL"), time_interval);

	addSaveFunc([powerkey_list, powerkey_action_value, double_push, double_push_value, time_interval, time_interval_value]
		{
			std::string powerkey_action_new_value = powerkey_list->getSelected();
			bool double_push_new_value = double_push->getState();
			int time_interval_new_value = (int) Math::round( time_interval->getValue() );

			if ( (powerkey_action_value != powerkey_action_new_value) || (double_push_value != double_push_new_value) || (time_interval_value != time_interval_new_value) )
			{
				ApiSystem::getInstance()->setPowerkeyValues(powerkey_action_new_value, double_push_new_value, time_interval_new_value);
			}
		});
}
