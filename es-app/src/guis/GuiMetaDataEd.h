#pragma once
#ifndef ES_APP_GUIS_GUI_META_DATA_ED_H
#define ES_APP_GUIS_GUI_META_DATA_ED_H

#include "components/ComponentGrid.h"
#include "components/NinePatchComponent.h"
#include "scrapers/Scraper.h"
#include "components/MenuComponent.h"
#include "components/OptionListComponent.h"
#include "GuiComponent.h"
#include "MetaData.h"

class ComponentList;
class TextComponent;
class FileData;

class GuiMetaDataEd : public GuiComponent
{
public:
	GuiMetaDataEd(Window* window, MetaDataList* md, const std::vector<MetaDataDecl>& mdd, ScraperSearchParams params, 
		const std::string& header, std::function<void()> savedCallback, std::function<void()> deleteFunc, FileData* file);
	
	bool input(InputConfig* config, Input input) override;
	void onSizeChanged() override;
	virtual std::vector<HelpPrompt> getHelpPrompts() override;

private:
	bool isStatistic(const std::string name);

	void save();
	void fetch();
	void fetchDone(const ScraperSearchResult& result);
	void close(bool closeAllWindows, bool ignoreChanges = false);

	NinePatchComponent mBackground;
	ComponentGrid mGrid;
	
	std::shared_ptr<TextComponent> mTitle;
	std::shared_ptr<TextComponent> mSubtitle;
	std::shared_ptr<ComponentGrid> mHeaderGrid;
	std::shared_ptr<ComponentList> mList;
	std::shared_ptr<ComponentGrid> mButtons;

	ScraperSearchParams mScraperParams;

	// typedef OptionListComponent<char> CoreList;
	//std::shared_ptr<CoreList> mCoreList;

	std::vector< std::shared_ptr<GuiComponent> > mEditors;

	std::vector<MetaDataDecl> mMetaDataDecl;
	MetaDataList* mMetaData;
	std::function<void()> mSavedCallback;
	std::function<void()> mDeleteFunc;
};

#endif // ES_APP_GUIS_GUI_META_DATA_ED_H
