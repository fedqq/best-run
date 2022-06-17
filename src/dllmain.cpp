#include "includes.h"
#include "extensions2.h"
#include <fstream>

using namespace gd;
using std::string;
using std::stringstream;

struct runObj {
	int start = 0;
	int end = 0;
	int amount = 1;

	string getString() const {
		stringstream stream;
		if (start != 0) stream << start << " - " << end << "% ";
		else stream << end << "%";
		if (amount != 1) stream << "x" << amount;
		return stream.str();
	}
};

using vector = std::vector<runObj>;

std::map<int, vector> runs = std::map<int, vector>();
GameManager* gm = GameManager::sharedState();

struct {

	bool mainToggle;
	int runAmount = 5;
	int minimumPercent;
	int lastStartingPercent;
	int currentStart;
	int currentEnd;
	MegaHackExt::Colour col;
	GLubyte opacity;

} state;

enum position : int
{
	BOTTOM_LEFT = 0,
	TOP_LEFT = 2,
	BOTTOM_RIGHT = 1,
	TOP_RIGHT = 3
} pos;

inline bool inPlayLayer() { return gm->getPlayLayer() != nullptr; }
inline vector& getVec() { return runs[gm->getPlayLayer()->m_level->m_nLevelID]; }

void addRun(runObj r) {

	if(r.start == r.end) return;

	auto& vect = getVec();

	for (auto& obj : vect) {
		if ((obj.start == r.start) && (obj.end == r.end)) {
			obj.amount++;
			return;
		}
	}

	vect.resize(vect.size() + 1);
	vect.push_back(r);
	std::sort(vect.begin(), vect.end(), [](runObj r, runObj r2)
		{
			return (r.end - r.start) < (r2.end - r2.start);
		}
	);
	std::reverse(vect.begin(), vect.end());
}

string getVectorText(bool full) {

	int i = 0;
	stringstream stream;
	for (auto& obj : getVec()) {
		if (obj.end != 0) {
			i++;
			stream << obj.getString() << ", ";
			if (!full && i == state.runAmount) break;
		}
	}
	string str = (i == 1 || i == 0) ? string("Best Run: ") += stream.str() : string("Best Runs: ") += stream.str();
	if (i != 0) {
		str.pop_back();
		str.pop_back();
	}
	else {
		str += "0%";
	}

	return str;
}

void updateLabel() {

	if (!inPlayLayer()) return;

	CCLabelBMFont* label = dynamic_cast<CCLabelBMFont*>(gm->getPlayLayer()->getChildByTag(286759));
	if (!label) {
		label = CCLabelBMFont::create(" ", "bigFont.fnt");
		gm->getPlayLayer()->addChild(label, 999, 286759);
	}

	CCPoint point;
	CCPoint anchor;
	const auto winSize = CCDirector::sharedDirector()->getWinSize();
	switch (pos)
	{
	case BOTTOM_LEFT:
		point = CCPoint{ 0, 0 };
		anchor = CCPoint{ 0, 0 };
		break;
	case BOTTOM_RIGHT:
		point = CCPoint{ winSize.width,  0 };
		anchor = CCPoint{ 1, 0 };
		break;
	case TOP_LEFT:
		point = CCPoint{ 0, winSize.height };
		anchor = CCPoint{ 0, 1 };
		break;
	case TOP_RIGHT:
		point = CCPoint{ winSize.width,  winSize.height };
		anchor = CCPoint{ 1, 1 };
		break;
	}

	label->setScale(0.4f);
	label->setAnchorPoint(anchor);
	label->setPosition(point);
	label->setString(getVectorText(false).c_str());
	label->setColor({ state.col.r, state.col.g, state.col.b });
	label->setOpacity(state.opacity);

	if (state.mainToggle) {
		label->setVisible(true);
	}
	else {
		label->setVisible(false);
	}
}

namespace callbacks {

	using namespace MegaHackExt;
	using std::to_string;

	void MH_CALL mainCheckbox(CheckBox*, bool b) {
		gm->setGameVariable("bestRunToggle", b);
		state.mainToggle = b;
		updateLabel();
	}

	void MH_CALL positionDropdown(ComboBox*, int i, const char*) {
		pos = static_cast<position>(i);
		gm->setIntGameVariable("bestRunPos", i);
		updateLabel();
	}

	void MH_CALL MinimumInput(TextBox* t, const char* text) {
		
		//Remove all the non number chars and the first zeroes
		string str = string(text);

		str.erase(std::remove_if(
			str.begin(),
			str.end(),
			[](char c) { return !std::isdigit(c); }),
			str.end()
		);

		if (str.length() != 0) str.erase(0, str.find_first_not_of('0'));
		if (str.length() == 0 || str == " ") str = "0";
		state.minimumPercent = std::stoi(str);
		t->set(str.c_str());
		gm->setIntGameVariable("bestRunMin", state.minimumPercent);
		updateLabel();

		auto& vect = getVec();
		for (auto& obj : vect) {
			if (obj.end - obj.start < state.minimumPercent) {
				obj = { 0, 0 };
			}
			std::sort(vect.begin(), vect.end(), [](runObj r, runObj r2) {
				return r.end < r2.end;
				});
			std::reverse(vect.begin(), vect.end());
		}

	}

	void MH_CALL runAmountInput(TextBox* t, const char* text) {

		string str = string(text);
		str.erase(std::remove_if(str.begin(), str.end(),
			[](char c)
			{ return !std::isdigit(c); }),
			str.end());

		if (str.length() != 0) str.erase(0, str.find_first_not_of('0'));
		if (str.length() == 0 || str == "0" || str == " ") str = "1";
		state.runAmount = std::stoi(str);
		t->set(str.c_str());
		gm->setIntGameVariable("bestRunRuns", state.runAmount);
		updateLabel();
	}

	void MH_CALL clearBtn(Button*) {
		if (!inPlayLayer()) return;
		runs[gm->getPlayLayer()->m_level->m_nLevelID].clear();
		updateLabel();
	}

	void MH_CALL opacityInput(TextBox* t, const char* text) {
		string str = string(text);
		str.erase(std::remove_if(str.begin(), str.end(),
			[](char c)
			{ return !std::isdigit(c); }),
			str.end());

		if (str.length() != 0) str.erase(0, str.find_first_not_of('0'));
		if (str.length() == 0 || str == "0" || str == " ") str = "5";
		if (std::stoi(str) > 255) str = "255";
		state.opacity = std::stoi(str);
		t->set(str.c_str());
		gm->setIntGameVariable("bestRunOpacity", state.opacity);
		updateLabel();
	}

	void MH_CALL colorPickerInput(ColourPicker* c, Colour color) {
		gm->setIntGameVariable("bestRunR", color.r);
		gm->setIntGameVariable("bestRunG", color.g);
		gm->setIntGameVariable("bestRunB", color.b);
		state.col = color;
		updateLabel();
	}

	//I have no idea if this is the correct way to make the callbacks but it works so...
	class Alert : public FLAlertLayerProtocol {
		void FLAlert_Clicked(gd::FLAlertLayer*, bool confirmBtn) override {
			if (!confirmBtn || !inPlayLayer()) return;

			if(state.currentStart < state.currentEnd) addRun({ state.currentStart, state.currentEnd });
			updateLabel();
		}
	};

	class StartInputDelegate : public TextInputDelegate {
		virtual void textChanged(CCTextInputNode* c) {
			if (string(c->getString()).length() == 0) {
				state.currentStart = 0;
			}
			else {
				state.currentStart = std::stoi(c->getString());
			}
		}
	};

	class EndInputDelegate : public TextInputDelegate {
		virtual void textChanged(CCTextInputNode* c) {
			if (string(c->getString()).length() == 0) {
				state.currentEnd = 0;
			}
			else {
				state.currentEnd = std::stoi(c->getString());
			}
		}
	};

	void MH_CALL manualAddBtn(Button*) {

		if (!inPlayLayer()) return;

		auto alertLayer = FLAlertLayer::create(new Alert(), "Add Run", "Cancel", "Confirm", " - ");

		auto startTextBox = CCTextInputNode::create(" ", alertLayer, "bigFont.fnt", 50, 30);
		state.currentStart = 0;
		startTextBox->setString("0");
		startTextBox->setMaxLabelLength(3);
		startTextBox->setAllowedChars("1234567890");
		startTextBox->setDelegate(new StartInputDelegate());
		startTextBox->setPosition(-50, 47);

		auto endTextBox = CCTextInputNode::create(" ", alertLayer, "bigFont.fnt", 50, 30);
		state.currentEnd = 50;
		endTextBox->setString("50");
		endTextBox->setMaxLabelLength(3);
		endTextBox->setAllowedChars("1234567890");
		endTextBox->setDelegate(new EndInputDelegate());
		endTextBox->setPosition(50, 47);

		alertLayer->getButtonMenu()->addChild(startTextBox);
		alertLayer->getButtonMenu()->addChild(endTextBox);
		alertLayer->show();
	};

	//Credit to some guy in the answers of this question: https://cplusplus.com/forum/beginner/14349/
	void toClipboard(const std::string& s) {
		OpenClipboard(GetActiveWindow());
		EmptyClipboard();
		HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, s.size() + 1);
		if (!hg) {
			CloseClipboard();
			return;
		}
		memcpy(GlobalLock(hg), s.c_str(), s.size() + 1);
		GlobalUnlock(hg);
		SetClipboardData(CF_TEXT, hg);
		CloseClipboard();
		GlobalFree(hg);
	}

	void MH_CALL clipboardBtn(Button*) {
		if (!inPlayLayer()) return;
		toClipboard(getVectorText(true));
	}
}

void createWindow() {

	using namespace MegaHackExt;
	using std::to_string;

	Colour c = { 
		(unsigned char) gm->getIntGameVariable("bestRunR"), 
		(unsigned char) gm->getIntGameVariable("bestRunG"), 
		(unsigned char) gm->getIntGameVariable("bestRunR") 
	};

	if (c.r == 0 && c.g == 0 && c.b == 0) c = { 255, 255, 255 };

	state.col = c;
	state.mainToggle = gm->getGameVariable("bestRunToggle");
	state.runAmount = gm->getIntGameVariable("bestRunRuns") == 0 ? 5 : gm->getIntGameVariable("bestRunRuns");
	state.minimumPercent = gm->getIntGameVariable("bestRunMin");
	state.opacity = gm->getIntGameVariable("bestRunOpacity") == 0 ? 64 : gm->getIntGameVariable("bestRunOpacity");

	pos = position(gm->getIntGameVariable("bestRunPos"));

	auto window = Window::Create("Best Run");

	const char* strs[] = { "Bottom-Left", "Bottom-Right", "Top-Left", "Top-Right", 0 };
	auto dropdown = ComboBox::Create("Position: ", "");
	dropdown->setValues(strs);
	dropdown->setCallback(callbacks::positionDropdown);
	dropdown->set(gm->getIntGameVariable("bestRunPos"));

	auto mainToggle = CheckBox::Create("Toggle");
	mainToggle->set(mainToggle);
	mainToggle->setCallback(callbacks::mainCheckbox);

	auto qualifying = TextBox::Create("");
	qualifying->setCallback(callbacks::MinimumInput);
	qualifying->set(to_string(gm->getIntGameVariable("bestRunMin")).c_str());

	auto amount = TextBox::Create("");
	amount->setCallback(callbacks::runAmountInput);
	amount->set(to_string(state.runAmount).c_str());

	auto clearBtn = Button::Create("Clear Runs");
	clearBtn->setCallback(callbacks::clearBtn);

	//absolute british spelling :eww:
	auto colorPicker = ColourPicker::Create(state.col);
	colorPicker->setCallback(callbacks::colorPickerInput);

	auto opacityInput = TextBox::Create("");
	opacityInput->set(to_string(state.opacity).c_str());
	opacityInput->setCallback(callbacks::opacityInput);

	auto manualBtn = Button::Create("Add Manual Run");
	manualBtn->setCallback(callbacks::manualAddBtn);

	auto copyBtn = Button::Create("Copy Text To Clipboard");
	copyBtn->setCallback(callbacks::clipboardBtn);

	window->add(copyBtn);
	window->add(colorPicker);
	window->add(HorizontalLayout::Create(Label::Create("Opacity: "), opacityInput));
	window->add(HorizontalLayout::Create(Label::Create("Run Amount: "), amount));
	window->add(HorizontalLayout::Create(Label::Create("Minimum %: "), qualifying));
	window->add(manualBtn);
	window->add(clearBtn);
	window->add(dropdown);
	window->add(mainToggle);

	Client::commit(window);
}

void(__thiscall* PlayerObject_playerDestroyed)(PlayerObject* self, bool destroyed);
void __fastcall PlayerObject_playerDestroyedHook(PlayerObject* self, void*, bool destroyed) {
	PlayerObject_playerDestroyed(self, destroyed);
	float x = self->getPositionX();
	auto playlayer = gm->getPlayLayer();
	int percent = std::floor(x / playlayer->m_levelLength * 100.f);
	auto obj = runObj{ state.lastStartingPercent, percent };
	if (obj.end - obj.start > state.minimumPercent || obj.start != 0) addRun(obj);
	updateLabel();
}

void(__thiscall* PlayLayer_resetLevel)(PlayLayer*);
void __fastcall PlayLayer_resetLevel_H(PlayLayer* self) {
	PlayLayer_resetLevel(self);

	int num = (gm->getPlayLayer()->m_pPlayer1->getPositionX() / gm->getPlayLayer()->m_levelLength) * 100;

	//For some reason this happens when its at 0
	if (num == 57506100) {
		state.lastStartingPercent = 0;
	}
	else state.lastStartingPercent = num;
	if (num < 0) {
		num = 0;
	}
	updateLabel();
}

bool(__thiscall* PlayLayer_init)(gd::PlayLayer*, GJGameLevel*);
bool __fastcall PlayLayer_init_H(PlayLayer* self, void*, GJGameLevel* lvl) {
	bool ret = PlayLayer_init(self, lvl);
	updateLabel();
	return ret;
}

DWORD WINAPI thread_func(void* hModule) {

	MH_Initialize();

	auto base = reinterpret_cast<uintptr_t>(GetModuleHandle(0));
	createWindow();

	MH_CreateHook(
		reinterpret_cast<void*>(base + 0x1EFAA0),
		PlayerObject_playerDestroyedHook,
		reinterpret_cast<void**>(&PlayerObject_playerDestroyed)
	);

	MH_CreateHook(
		reinterpret_cast<void*>(base + 0x20BF00),
		PlayLayer_resetLevel_H,
		reinterpret_cast<void**>(&PlayLayer_resetLevel)
	);

	MH_CreateHook(
		reinterpret_cast<void*>(base + 0x1FB780),
		PlayLayer_init_H,
		reinterpret_cast<void**>(&PlayLayer_init)
	);

	MH_EnableHook(MH_ALL_HOOKS);

	return 0;
}

BOOL APIENTRY DllMain(HMODULE handle, DWORD reason, LPVOID reserved) {
	if (reason == DLL_PROCESS_ATTACH) {
		CreateThread(0, 0x100, thread_func, handle, 0, 0);
	}
	return TRUE;
}