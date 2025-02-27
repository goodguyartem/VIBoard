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

#ifdef VI_PLATFORM_WINDOWS

#include "Hotkey.h"
#include "../Log.h"
#include "../Exceptions.h"
#include "../Application.h"

#include <vector>
#include <assert.h>

#include <Windows.h>

namespace vi {
	extern std::vector<Hotkey> hotkeys;

	namespace {
		unsigned int SdlModToWin32(SDL_Keymod sdlMod) noexcept {
			assert(ensureInSupportedRange(sdlMod));

			unsigned int win32Mod = 0;
			if (sdlMod & SDL_KMOD_CTRL) {
				win32Mod |= MOD_CONTROL;
			}
			if (sdlMod & SDL_KMOD_ALT) {
				win32Mod |= MOD_ALT;
			}
			if (sdlMod & SDL_KMOD_SHIFT) {
				win32Mod |= MOD_SHIFT;
			}
			return win32Mod;
		}

		uint16_t getVk(const Hotkey& hotkey) noexcept {
			if (hotkey.mod & SDL_KMOD_NUM) {
				switch (hotkey.scancode) {
				case SDL_SCANCODE_KP_0: return VK_NUMPAD0;
				case SDL_SCANCODE_KP_1: return VK_NUMPAD1;
				case SDL_SCANCODE_KP_2: return VK_NUMPAD2;
				case SDL_SCANCODE_KP_3: return VK_NUMPAD3;
				case SDL_SCANCODE_KP_4: return VK_NUMPAD4;
				case SDL_SCANCODE_KP_5: return VK_NUMPAD5;
				case SDL_SCANCODE_KP_6: return VK_NUMPAD6;
				case SDL_SCANCODE_KP_7: return VK_NUMPAD7;
				case SDL_SCANCODE_KP_8: return VK_NUMPAD8;
				case SDL_SCANCODE_KP_9: return VK_NUMPAD9;
				case SDL_SCANCODE_KP_PERIOD: return VK_DECIMAL;
				case SDL_SCANCODE_NUMLOCKCLEAR: return VK_NUMLOCK;
				}
			}
			return MapVirtualKey(hotkey.raw, MAPVK_VSC_TO_VK);
		}
	}

	HotkeyId registerHotkey(const Hotkey& hotkey) {
		assert(hotkey.raw != 0); // Hotkey must have a valid scancode.

		const HotkeyId id = static_cast<HotkeyId>(hotkeys.size());
		if (!RegisterHotKey(nullptr, id, SdlModToWin32(hotkey.mod) | MOD_NOREPEAT, getVk(hotkey))) {
			throw ExternalError("Failed to register hotkey.");
		}
		hotkeys.push_back(hotkey);
		return id;
	}

	void unregisterHotkey(HotkeyId id) {
		assert(isValidHotkey(id));
		if (!UnregisterHotKey(nullptr, id)) {
			throw ExternalError("Failed to unregister hotkey.");
		}
		// We could also remember that this id was freed, then reuse it for next time.
		// However, in practice, it's nearly impossible to run out of IDs or memory by registering too many hotkeys.
		hotkeys[id] = Hotkey();
	}

	void processHotkeyPresses(const Application& app) noexcept {
		if (app.isInactive()) {
			WaitMessage();
		}

		MSG msg(0);
		if (PeekMessage(&msg, nullptr, 0, 0, PM_NOREMOVE) && msg.message == WM_HOTKEY) { // System events get cleared by SDL in main event loop.
			assert(isValidHotkey(static_cast<HotkeyId>(msg.wParam)));
			hotkeys[msg.wParam].callback();
		}
	}
}

#endif