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

#include "appStates/AppState.h"

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include <imgui.h>

#include <vector>
#include <memory>
#include <functional>
#include <filesystem>

namespace vi {
	using WindowOwner = std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)>;
	using TrayOwner = std::unique_ptr<SDL_Tray, decltype(&SDL_DestroyTray)>;

	using SurfaceOwner = std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)>;

	extern const std::filesystem::path storagePath;

	class Application {
	public:
		std::vector<std::unique_ptr<AppState>> states;
		std::vector<ImFont*> fonts;

		TrayOwner tray{nullptr, SDL_DestroyTray};
		bool canSleep = true;

		void run();
		
		void quit() noexcept {
			running = false;
		}

		void setMenuBarCallback(std::function<void()> callback) noexcept {
			menuBarCallback = callback;
		}

		SDL_Window* getWindow() const noexcept {
			return window.get();
		}

		ImGuiID getDockspaceId() const noexcept {
			return dockspaceId;
		}

		SDL_Surface* getIcon() const noexcept {
			return icon.get();
		}

		SDL_Surface* getTrayIcon() const noexcept {
			return trayIcon.get();
		}

		bool isInactive() const noexcept {
			return canSleep && (SDL_GetWindowFlags(window.get()) & (SDL_WINDOW_MINIMIZED | SDL_WINDOW_HIDDEN) || inBackground);
		}

	private:
		WindowOwner window{nullptr, SDL_DestroyWindow};
		SDL_GLContext glContext = nullptr;
		std::function<void()> menuBarCallback;

		ImGuiID dockspaceId = 0;
		SurfaceOwner icon{IMG_Load("res/Icon.png"), SDL_DestroySurface};
		SurfaceOwner trayIcon{IMG_Load("res/TrayIcon.png"), SDL_DestroySurface};

		bool running = false;
		bool inBackground = false;

		void init();

		void pollEvents() noexcept;
		void update();
		void render() const;
	};
}