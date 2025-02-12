#pragma once

#include "appStates/AppState.h"

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include <imgui.h>

#include <vector>
#include <memory>
#include <functional>
#include <fstream>

namespace vi {
	using WindowOwner = std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)>;
	using TrayOwner = std::unique_ptr<SDL_Tray, decltype(&SDL_DestroyTray)>;

	using SurfaceOwner = std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)>;

	class Application {
	public:
		std::vector<std::unique_ptr<AppState>> states;
		std::vector<ImFont*> fonts;

		TrayOwner tray{nullptr, SDL_DestroyTray};

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
			const SDL_WindowFlags flags = SDL_GetWindowFlags(window.get());
			return flags & (SDL_WINDOW_MINIMIZED | SDL_WINDOW_HIDDEN);
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