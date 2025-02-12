#include "Hotkey.h"

#include <vector>
#include <assert.h>

namespace vi {
	std::vector<Hotkey> hotkeys;

	bool isValidHotkey(HotkeyId id) noexcept {
		if (id < 0 || id >= static_cast<int>(hotkeys.size())) {
			return false;
		}
		return hotkeys[id].scancode != SDL_SCANCODE_UNKNOWN;
	}

	Hotkey& getHotkey(HotkeyId id) noexcept {
		assert(isValidHotkey(id));
		return hotkeys[id];
	}

	std::string modsToString(SDL_Keymod mod) noexcept {
		assert(ensureInSupportedRange(mod));
		std::string string;
		if (mod & SDL_KMOD_CTRL) {
			string += "Ctrl+";
		}
		if (mod & SDL_KMOD_ALT) {
			string += "Alt+";
		}
		if (mod & SDL_KMOD_SHIFT) {
			string += "Shift+";
		}
		return string;
	}

	std::string getHotkeyName(HotkeyId id) noexcept {
		if (!isValidHotkey(id)) {
			return "None";
		}
		const Hotkey& key = getHotkey(id);
		return modsToString(key.mod) + SDL_GetKeyName(SDL_SCANCODE_TO_KEYCODE(key.scancode));
	}
}