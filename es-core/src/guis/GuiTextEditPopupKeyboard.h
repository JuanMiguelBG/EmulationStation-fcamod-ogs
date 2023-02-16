#include "GuiComponent.h"

#include "components/NinePatchComponent.h"
#include "components/ButtonComponent.h"
#include "components/ComponentGrid.h"
#include "components/TextEditComponent.h"
#include "components/TextComponent.h"
#include <functional>

class GuiTextEditPopupKeyboard : public GuiComponent
{
public:
	GuiTextEditPopupKeyboard(Window* window, const std::string& title, const std::string& initValue,
		const std::function<bool(const std::string&)>& okCallback, bool multiLine,
		const std::function<void(const std::string&)>& backCallback = nullptr);

	GuiTextEditPopupKeyboard(Window* window, const std::string& title, const std::string& initValue,
		const std::function<bool(const std::string&)>& okCallback, bool multiLine, const char* acceptPromptText = "OK",
		const std::function<void(const std::string&)>& backCallback = nullptr);

	bool input(InputConfig* config, Input input);
	//void update(int deltatime) override;
	void onSizeChanged();
	std::vector<HelpPrompt> getHelpPrompts() override;

private:
	class KeyboardButton
	{
	public:
		std::shared_ptr<ButtonComponent> button;
		const std::string key;
		const std::string shiftedKey;
		const std::string altedKey;
		KeyboardButton(const std::shared_ptr<ButtonComponent> b, const std::string& k, const std::string& sk, const std::string& ak) : button(b), key(k), shiftedKey(sk), altedKey(ak) {};
	};

	std::shared_ptr<ButtonComponent> makeButton(const std::string& key, const std::string& shiftedKey, const std::string& altedKey);
	std::vector<KeyboardButton> keyboardButtons;

	std::shared_ptr<ButtonComponent> mShiftButton;
	std::shared_ptr<ButtonComponent> mAltButton;
	std::shared_ptr<ButtonComponent> mInputCursorLeft;
	std::shared_ptr<ButtonComponent> mInputCursorRight;

	void shiftKeys();
	void altKeys();

	NinePatchComponent mBackground;
	ComponentGrid mGrid;

	std::shared_ptr<TextComponent> mTitle;
	std::shared_ptr<TextEditComponent> mText;
	std::shared_ptr<ComponentGrid> mKeyboardGrid;

	std::function<bool(const std::string&)> mOkCallback;
	std::function<void(const std::string&)> mBackCallback;

	bool mMultiLine;
	bool mShift = false;
	bool mAlt = false;

	std::string mAcceptPromptText;
};
