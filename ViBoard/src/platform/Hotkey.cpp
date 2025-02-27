/*
	ViBoard - Lightweight free & open-source soundboard.
	Copyright (C) 2025-EndOfTime  goodguyartem <https://www.github.com/goodguyartem>

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

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