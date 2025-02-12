#pragma once

#include <SDL3/SDL.h>

#include <functional>
#include <string>
#include <stdint.h>

namespace vi {
	class Application;

	struct Hotkey {
		SDL_Scancode scancode = SDL_SCANCODE_UNKNOWN;
		uint16_t raw = 0;
		SDL_Keymod mod = SDL_KMOD_NONE;
		std::function<void()> callback;
	};

	using HotkeyId = int;
	inline constexpr HotkeyId nullHotkey = -1;

	HotkeyId registerHotkey(const Hotkey& hotKey);
	void unregisterHotkey(HotkeyId id);

	bool isValidHotkey(HotkeyId id) noexcept;
	Hotkey& getHotkey(HotkeyId id) noexcept;

	void processHotkeyPresses(const Application& app) noexcept;

	std::string modsToString(SDL_Keymod mod) noexcept;
	std::string getHotkeyName(HotkeyId id) noexcept;

	// Strips modifiers the program does not support, such as the scroll lock key.
	inline SDL_Keymod stripUnsupportedMods(SDL_Keymod mod) noexcept {
		return ((mod & SDL_KMOD_CTRL) | (mod & SDL_KMOD_ALT) | (mod & SDL_KMOD_SHIFT) | (mod & SDL_KMOD_NUM));
	}

	inline bool ensureInSupportedRange(SDL_Keymod mod) noexcept {
		return mod == stripUnsupportedMods(mod);
	}
}