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