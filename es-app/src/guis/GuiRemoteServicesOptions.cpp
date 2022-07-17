#include "guis/GuiRemoteServicesOptions.h"

#include "components/SwitchComponent.h"
#include "guis/GuiMsgBox.h"
#include "Window.h"
#include "ApiSystem.h"
#include "guis/GuiLoading.h"
#include "Log.h"


GuiRemoteServicesOptions::GuiRemoteServicesOptions(Window* window) : GuiSettings(window, _("REMOTE SERVICES SETTINGS").c_str())
{
	initializeMenu(window);
}

GuiRemoteServicesOptions::~GuiRemoteServicesOptions()
{

}

void GuiRemoteServicesOptions::configRemoteService(Window* window, RemoteServiceInformation service, bool isActive, bool isStartOnBoot)
{
		if ((service.isActive != isActive) || (service.isStartOnBoot != isStartOnBoot))
		{
			service.isActive = isActive;
			service.isStartOnBoot = isStartOnBoot;
			Settings::getInstance()->setBool("wait.process.loading", true);
			char title[64];
			snprintf(title, 64, _("CONFIGURING THE '%s' SERVICE...").c_str(), service.name.c_str());
			window->pushGui(new GuiLoading<bool>(window, title,
				[service]
				{
					return ApiSystem::getInstance()->configRemoteService(service);
				},
				[window, service](bool success)
				{
					Settings::getInstance()->setBool("wait.process.loading", false);
					char msg[64];
					if (success)
					{
						snprintf(msg, 64, _("SERVICE '%s' - SUCCESFULLY CONFIGURED.").c_str(), service.name.c_str());
						window->pushGui(new GuiMsgBox(window, msg));
					}
					else
					{
						snprintf(msg, 64, _("SERVICE '%s' - CONFIGURATION ERROR.").c_str(), service.name.c_str());
						window->pushGui(new GuiMsgBox(window, msg));
					}
				}));
		}
}

void GuiRemoteServicesOptions::initializeMenu(Window* window)
{
	// NTP
	addGroup(_("NTP"));
	RemoteServiceInformation ntp = ApiSystem::getInstance()->getNtpStatus();

	auto ntp_active = std::make_shared<SwitchComponent>(window, ntp.isActive);
	addWithLabel(_("ACTIVATE"), ntp_active);

	auto ntp_boot = std::make_shared<SwitchComponent>(window, ntp.isStartOnBoot);
	addWithLabel(_("BOOT WITH SYSTEM"), ntp_boot);

	addSaveFunc([this, window, ntp, ntp_active, ntp_boot]
	{
		configRemoteService(window, ntp, ntp_active->getState(), ntp_boot->getState());
	});

	// SAMBA
	addGroup(_("SAMBA"));
	RemoteServiceInformation samba = ApiSystem::getInstance()->getSambaStatus();

	auto samba_active = std::make_shared<SwitchComponent>(window, samba.isActive);
	addWithLabel(_("ACTIVATE"), samba_active);

	auto samba_boot = std::make_shared<SwitchComponent>(window, samba.isStartOnBoot);
	addWithLabel(_("BOOT WITH SYSTEM"), samba_boot);

	addSaveFunc([this, window, samba, samba_active, samba_boot]
	{
		configRemoteService(window, samba, samba_active->getState(), samba_boot->getState());
	});

	// NetBIOS
	addGroup(_("NetBIOS"));
	RemoteServiceInformation netbios = ApiSystem::getInstance()->getNetBiosStatus();

	auto netbios_active = std::make_shared<SwitchComponent>(window, netbios.isActive);
	addWithLabel(_("ACTIVATE"), netbios_active);

	auto netbios_boot = std::make_shared<SwitchComponent>(window, netbios.isStartOnBoot);
	addWithLabel(_("BOOT WITH SYSTEM"), netbios_boot);

	addSaveFunc([this, window, netbios, netbios_active, netbios_boot]
	{
		configRemoteService(window, netbios, netbios_active->getState(), netbios_boot->getState());
	});

	// SSH
	addGroup(_("SSH"));
	RemoteServiceInformation ssh = ApiSystem::getInstance()->getSshStatus();

	auto ssh_active = std::make_shared<SwitchComponent>(window, ssh.isActive);
	addWithLabel(_("ACTIVATE"), ssh_active);

	auto ssh_boot = std::make_shared<SwitchComponent>(window, ssh.isStartOnBoot);
	addWithLabel(_("BOOT WITH SYSTEM"), ssh_boot);

	addSaveFunc([this, window, ssh, ssh_active, ssh_boot]
	{
		configRemoteService(window, ssh, ssh_active->getState(), ssh_boot->getState());
	});
}
