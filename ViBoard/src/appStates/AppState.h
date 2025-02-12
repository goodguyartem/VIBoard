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