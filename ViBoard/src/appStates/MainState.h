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

		BrowseUserData browseData{app->getWindow()};
		int theme = 0;
		
		bool minimizeToTray = false;
		bool openOnStartup = isLaunchingOnStartup();
		bool startMinimized = false;

		void showSoundboards() noexcept;
		void showOptions() noexcept;
		void showKeyAssign() noexcept;
		void showSoundVolumeMenu() noexcept;
		void showWelcomeScreen() noexcept;

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

		void resetAudioStream(size_t index, bool makePreferred) noexcept;
		void tryPlay(const Sound& sound) noexcept;
		void stop() const noexcept;

		void serialize();
		void deserialize();

		void setTheme() const noexcept;
		void loadExampleSoundboard() noexcept;

		void onExitError(const char* error) noexcept;

		void createTray() noexcept;
	};
}