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

#include "AppState.h"
#include "../Audio.h"
#include "../platform/Hotkey.h"
#include "../platform/Platform.h"
#include "../Application.h"

#include <SDL3/SDL.h>

#include <vector>
#include <filesystem>
#include <format>
#include <array>
#include <atomic>

namespace vi {
	struct Soundboard {
		std::filesystem::path path;
		std::vector<Sound> sounds;
	};

	struct BrowseUserData {
		SDL_Window* window = nullptr;
		Soundboard result;
		std::atomic_bool ready = false;
	};

	struct KeybindAssign {
		bool showMenu = false;
		bool assigning = false;
		HotkeyId* id = nullptr;
		std::function<void()> action;
	};

	struct PushToTalkAssign {
		bool showMenu = false;
		bool assigning = false;
	};

	struct SoundVolumeMenu {
		bool showMenu = false;
		size_t board = 0;
		size_t sound = 0;
	};

	struct PlaybackConfig {
		AudioStreamOwner stream{nullptr, SDL_DestroyAudioStream};
		// Must be int for ImGUI compatibility.
		int deviceIndex = 0;
		// Storing a copy of the preferred device name as it may not currently be available.
		std::string preferred;
		float gain = 1.0f;
	};

	class MainState : public AppState {
	public:
		MainState(Application& app);
		virtual ~MainState() noexcept;

		void onEvent(const SDL_Event& event) noexcept override;
		void update() noexcept override;

	private:
		Application* app;
		std::vector<Soundboard> soundboards;
		std::array<PlaybackConfig, 2> playback;
		bool dualPlayback = false;
		
		std::vector<SDL_AudioDeviceID> audioDevices;
		std::vector<const char*> deviceNames;

		bool showWelcome = true;

		KeybindAssign keyAssign;
		SoundVolumeMenu soundVolumeMenu;
		HotkeyId stopHotkey = nullHotkey;
		
		PushToTalkAssign pttAssign;
		SDL_Scancode pttScancode = SDL_SCANCODE_UNKNOWN;
		uint16_t pttRaw = 0;
		bool pttActive = false;
		
		bool usePtt = false;
		HotkeyId pttToggleHotkey = nullHotkey;

		BrowseUserData browseData{app->getWindow()};
		int theme = 0;
		
		bool minimizeToTray = false;
		bool openOnStartup = isLaunchingOnStartup();
		bool startMinimized = false;

		void showSoundboards() noexcept;
		void showOptions() noexcept;
		void showKeyAssign() noexcept;
		void showPushToTalkAssign() noexcept;
		void showSoundVolumeMenu() noexcept;
		void showWelcomeScreen() noexcept;

		std::string getPushToTalkKeyName() const noexcept {
			return pttScancode == SDL_SCANCODE_UNKNOWN ? "None" : SDL_GetScancodeName(pttScancode);
		}

		void showStreamErrorText() const noexcept {
			ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "Could not create an audio stream. Please choose a different device.");
		}

		void showGainSlider(size_t index) noexcept {
			PlaybackConfig& config = playback[index];
			if (ImGui::SliderFloat(std::format("Output {}", index + 1).c_str(), &config.gain, 0.0f, 2.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp)) {
				SDL_SetAudioStreamGain(config.stream.get(), config.gain);
			}
		}
		void showGainOverrideSlider(Sound& sound, size_t index) noexcept;

		void tryPlay(const Sound& sound) noexcept;
		void stop() noexcept;

		void serialize();
		void deserialize();

		void setTheme() const noexcept;
		void loadExampleSoundboard() noexcept;

		void onExitError(const char* error) noexcept;

		void createTray() noexcept;
	};
}