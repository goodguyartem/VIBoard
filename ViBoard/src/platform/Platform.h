#pragma once

#include <SDL3/SDL.h>

#include <stdint.h>

namespace vi {
	void initPlatform();
	void quitPlatform() noexcept;

	bool isLaunchingOnStartup();
	bool setLaunchOnStartup(bool launch, SDL_Window* window);

	void sendKeyPress(uint16_t scancode, bool pressed) noexcept;
}