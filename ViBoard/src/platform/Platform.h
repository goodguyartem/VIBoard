#pragma once

#include <SDL3/SDL.h>

namespace vi {
	void initPlatform();
	void quitPlatform() noexcept;

	bool isLaunchingOnStartup();
	bool setLaunchOnStartup(bool launch, SDL_Window* window);
}