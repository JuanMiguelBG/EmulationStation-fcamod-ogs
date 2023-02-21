#include "guis/UpdatableGuiSettings.h"

#include "Window.h"
#include "GuiComponent.h"


UpdatableGuiSettings::UpdatableGuiSettings(Window* window, const std::string title) : GuiSettings(window, title)
{
}

UpdatableGuiSettings::~UpdatableGuiSettings()
{
	clear();
}

void UpdatableGuiSettings::addUpdatableComponent(GuiComponent *component)
{
	mUpdatables.push_back(component);
}

void UpdatableGuiSettings::update(int deltaTime)
{
	for (GuiComponent *updatable : mUpdatables)
		updatable->update(deltaTime);
}
