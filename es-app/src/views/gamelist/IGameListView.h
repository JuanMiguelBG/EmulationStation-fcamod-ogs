#pragma once
#ifndef ES_APP_VIEWS_GAME_LIST_IGAME_LIST_VIEW_H
#define ES_APP_VIEWS_GAME_LIST_IGAME_LIST_VIEW_H

#include "renderers/Renderer.h"
#include "MetaData.h"
#include "FileData.h"
#include "GuiComponent.h"

class ThemeData;
class Window;

// This is an interface that defines the minimum for a GameListView.
class IGameListView : public GuiComponent
{
public:
	IGameListView(Window* window, FolderData* root) : GuiComponent(window), mRoot(root)
		{ setSize((float)Renderer::getScreenWidth(), (float)Renderer::getScreenHeight()); }

	virtual ~IGameListView() {}

	// Called when a new file is added, a file is removed, a file's metadata changes, or a file's children are sorted.
	// NOTE: FILE_SORTED is only reported for the topmost FileData, where the sort started.
	//       Since sorts are recursive, that FileData's children probably changed too.
	virtual void onFileChanged(FileData* file, FileChangeType change) = 0;

	// Called whenever the theme changes.
	virtual void onThemeChanged(const std::shared_ptr<ThemeData>& theme) = 0;

	void setTheme(const std::shared_ptr<ThemeData>& theme);
	inline const std::shared_ptr<ThemeData>& getTheme() const { return mTheme; }

	virtual FileData* getCursor() = 0;
	virtual void setCursor(FileData*) = 0;

	virtual bool input(InputConfig* config, Input input) override;
	virtual void remove(FileData* game) = 0;

	virtual const char* getName() const = 0;
	virtual void launch(FileData* game) = 0;

	virtual HelpStyle getHelpStyle() override;

	void render(const Transform4x4f& parentTrans) override;

	virtual void setThemeName(std::string name);

	virtual std::vector<std::string> getEntriesLetters() = 0;

protected:
	virtual std::string getMetadata(FileData* file, MetaDataId metaDataId);

	std::string mCustomThemeName;

	FolderData* mRoot;
	std::shared_ptr<ThemeData> mTheme;

	std::string valueOrUnknown(const std::string value) { return value.empty() ? _("Unknown") : value; };
	std::string valueOrOne(const std::string value)
	{
		auto split = value.rfind("+");
		if (split != std::string::npos)
			return value.substr(0, split);

		split = value.rfind("-");
		if (split != std::string::npos)
			return value.substr(split + 1);

		std::string ret = value;

		int count = Utils::String::toInteger(value);

		if (count >= 10) ret = "9";
		else if (count == 0) ret = "1";

		return ret;
	};
};

#endif // ES_APP_VIEWS_GAME_LIST_IGAME_LIST_VIEW_H
