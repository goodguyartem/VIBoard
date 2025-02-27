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

namespace vi {
	class AppState {
	public:
		AppState() = default;
		virtual ~AppState() = default;

		virtual void onEvent(const SDL_Event& event) = 0;
		virtual void update() = 0;

	protected:
		AppState(const AppState& other) = default;
		AppState(AppState&& other) = default;

		AppState& operator=(const AppState& other) = default;
		AppState& operator=(AppState&& other) = default;
	};
}