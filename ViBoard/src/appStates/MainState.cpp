#include "MainState.h"
#include "../Application.h"
#include "../Log.h"
#include "../Exceptions.h"
#include "../platform/HotKey.h"
#include "../ImGuiConfig.h"

#include <SDL3/SDL.h>

#include <imgui.h>
#include <imgui_internal.h>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <fstream>
#include <unordered_set>

using namespace std::string_literals;
namespace fs = std::filesystem;

namespace vi {
	namespace {
		constexpr ImVec2 buttonSize(0.0f, 32.0f);

		void browseFiles(void* userData, const char* const* fileList, int filter) noexcept {
			SDL_assert(userData);
			if (!fileList || *fileList == nullptr || !fs::exists(*fileList)) {
				return;
			}

			VI_INFO("Folder opened: %s", fileList[0]);
			auto& data = *reinterpret_cast<BrowseUserData*>(userData);
			data.result.path = fs::absolute(fileList[0]);

			for (const auto& entry : fs::directory_iterator(data.result.path)) {
				if (!entry.is_regular_file() || !isSupported(entry.path().extension())) {
					continue;
				}

				try {
					data.result.sounds.emplace_back(entry.path());
				} catch (const std::exception& e) {
					const std::string message = std::format(
						"Unable to load sound \"{}\".\n"
						"Please ensure that the file is in correct format and that the program has read permission for that directory.\n\n"
						"Error: {}",
						entry.path().string(),
						e.what()
					);

					SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Failed to load sound!", message.c_str(), data.window);
				}
			}
			data.ready = true;
		}

		void refresh(Soundboard& board) noexcept {
			if (!fs::exists(board.path)) {
				board.sounds.clear();
				return;
			}

			std::unordered_set<fs::path> files;
			for (const auto& entry : fs::directory_iterator(board.path)) {
				if (entry.is_regular_file() && isSupported(entry.path().extension())) {
					files.insert(entry);
				}
			}

			for (auto soundIt = board.sounds.begin(); soundIt != board.sounds.end();) {
				const auto compareIt = files.find(soundIt->getPath());
				if (compareIt != files.end()) {
					files.erase(compareIt);
					soundIt++;
				} else {
					soundIt = board.sounds.erase(soundIt);
					if (soundIt == board.sounds.end()) {
						break;
					}
				}
			}

			for (const auto& file : files) {
				board.sounds.emplace_back(file);
			}
		}

		nlohmann::json serializeHotkey(HotkeyId id) noexcept {
			if (!isValidHotkey(id)) {
				return nullptr;
			}

			const Hotkey& hotkey = getHotkey(id);
			nlohmann::json json;
			json["scancode"] = hotkey.scancode;
			json["raw"] = hotkey.raw;
			json["mod"] = hotkey.mod;
			return json;
		}

		Hotkey deserializeHotkey(const nlohmann::json& json) {
			Hotkey hotkey;
			json["scancode"].get_to(hotkey.scancode);
			json["raw"].get_to(hotkey.raw);
			json["mod"].get_to(hotkey.mod);

			if (hotkey.scancode < SDL_SCANCODE_UNKNOWN || hotkey.scancode >= SDL_SCANCODE_COUNT || !ensureInSupportedRange(hotkey.mod)) {
				throw IOError("Bad hotkey.");
			}
			return hotkey;
		}

		const std::filesystem::path settingsPath = storagePath / "settings.json";
		const std::filesystem::path imGuiPath = storagePath / "imgui.ini";
	}

	MainState::MainState(Application& app)
		: app(&app) {

		if (!fs::exists(storagePath)) {
			fs::create_directory(storagePath);
			ImGui::LoadIniSettingsFromDisk("res/default.ini");
			loadExampleSoundboard();
		} else {
			if (fs::exists(settingsPath)) {
				try {
					deserialize();
				} catch (std::exception& e) {
					SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
						"Error loading settings!",
						("An occured while loading user settings. Some settings may be reset to their defaults.\n\n"
							"Error: "s + e.what()).c_str(),
						app.getWindow());
				}
			}
			ImGui::LoadIniSettingsFromDisk(fs::exists(imGuiPath) ? imGuiPath.string().c_str() : "res/default.ini");
		}

		if (minimizeToTray) {
			createTray();
		}
		if (!minimizeToTray || !startMinimized) {
			SDL_ShowWindow(app.getWindow());
		}

		setTheme();
	}

	MainState::~MainState() noexcept {
		try {
			serialize();
			ImGui::SaveIniSettingsToDisk(imGuiPath.string().c_str());
		} catch (std::exception& e) {
			onExitError(e.what());
		} catch (...) {
			onExitError("");
		}
	}

	void MainState::onEvent(const SDL_Event& event) noexcept {
		switch (event.type) {
		case SDL_EVENT_AUDIO_DEVICE_ADDED: {
			if (!SDL_IsAudioDevicePlayback(event.adevice.which)) {
				return;
			}

			const char* name = SDL_GetAudioDeviceName(event.adevice.which);
			audioDevices.push_back(event.adevice.which);
			deviceNames.push_back(name);

			for (size_t i = 0; i < playback.size(); i++) {
				PlaybackConfig& config = playback[i];
				const bool preferred = config.preferred == name;

				if (!config.stream || preferred) {
					if (preferred) {
						config.deviceIndex = static_cast<int>(audioDevices.size() - 1);
					}
					resetAudioStream(i, false);
				}
				if (!dualPlayback) {
					break;
				}
			}
			break;
		}

		case SDL_EVENT_AUDIO_DEVICE_REMOVED: {
			if (!SDL_IsAudioDevicePlayback(event.adevice.which)) {
				return;
			}

			const auto it = std::find(audioDevices.begin(), audioDevices.end(), event.adevice.which);
			if (it == audioDevices.end()) {
				break;
			}
			for (size_t i = 0; i < playback.size(); i++) {
				PlaybackConfig& config = playback[i];
				if (config.stream && config.deviceIndex == it - audioDevices.begin()) {
					config.stream.reset();
					if (config.deviceIndex >= static_cast<int>(audioDevices.size() - 1)) {
						config.deviceIndex = static_cast<int>(audioDevices.size() - 2);
					}
				}
			}
			audioDevices.erase(it);
			deviceNames.erase(deviceNames.begin() + std::distance(it, audioDevices.begin()));

			if (!playback[0].stream) {
				resetAudioStream(0, false);
			}
			if (dualPlayback && !playback[1].stream) {
				resetAudioStream(1, false);
			}
			break;
		}

		case SDL_EVENT_KEY_DOWN:
			if (keyAssign.assigning) {
				const SDL_Keymod mod = stripUnsupportedMods(event.key.mod);
				const SDL_Keymod mainMod = (mod & ~SDL_KMOD_NUM);

				if (event.key.scancode >= SDL_SCANCODE_LCTRL
					|| (event.key.scancode == SDL_SCANCODE_F12 && mainMod == SDL_KMOD_NONE)
					|| (event.key.scancode == SDL_SCANCODE_NUMLOCKCLEAR && (mainMod == SDL_KMOD_LCTRL || mainMod == SDL_KMOD_RCTRL))
					|| (event.key.scancode == SDL_SCANCODE_SCROLLLOCK && (mainMod == SDL_KMOD_LCTRL || mainMod == SDL_KMOD_RCTRL))) {
					break;
				}
				if (event.key.scancode == SDL_SCANCODE_ESCAPE && mainMod == SDL_KMOD_NONE) {
					keyAssign.assigning = false;
					break;
				}

				assert(keyAssign.id);

				if (isValidHotkey(*keyAssign.id)) {
					unregisterHotkey(*keyAssign.id);
					*keyAssign.id = nullHotkey;
				}

				if (event.key.scancode != SDL_SCANCODE_DELETE || mainMod != SDL_KMOD_NONE) {
					Hotkey hotkey;
					hotkey.scancode = event.key.scancode;
					hotkey.raw = event.key.raw;
					hotkey.mod = mod;
					hotkey.callback = keyAssign.action;
					*keyAssign.id = registerHotkey(hotkey);
				}

				keyAssign.assigning = false;
			} else if (pttAssign.assigning) {
				if (event.key.scancode == SDL_SCANCODE_DELETE) {
					pttScancode = SDL_SCANCODE_UNKNOWN;
					pttRaw = 0;
				} else if (event.key.scancode != SDL_SCANCODE_ESCAPE) {
					pttScancode = event.key.scancode;
					pttRaw = event.key.raw;
				}
				pttAssign.assigning = false;
			}
			break;
		}
	}

	void MainState::update() noexcept {
		if (!app->canSleep && SDL_GetAudioStreamAvailable(playback[0].stream.get()) == 0 && SDL_GetAudioStreamAvailable(playback[1].stream.get()) == 0) {
			app->canSleep = true;
		}

		if (showWelcome) {
			ImGui::PushStyleVarX(ImGuiStyleVar_FramePadding, 8.0f);
			showWelcomeScreen();
			ImGui::PopStyleVar();
		}

		ImGui::BeginDisabled(audioDevices.size() == 0);
		ImGui::PushStyleVarX(ImGuiStyleVar_FramePadding, 6.0f);
		if (keyAssign.showMenu) {
			showKeyAssign();
		} else if (soundVolumeMenu.showMenu) {
			showSoundVolumeMenu();
		} else if (pttAssign.showMenu) {
			showPushToTalkAssign();
		}
		ImGui::PopStyleVar();

		ImGui::PushStyleVarX(ImGuiStyleVar_FramePadding, 8.0f);
		showSoundboards();
		showOptions();

		ImGui::EndDisabled();
		ImGui::PopStyleVar();

		if (pttScancode != SDL_SCANCODE_UNKNOWN) {
			bool sendPtt = false;
			if (usePtt) {
				for (const PlaybackConfig& config : playback) {
					if (config.stream && SDL_GetAudioStreamAvailable(config.stream.get()) != 0) {
						sendPtt = true;
						break;
					}
				}
			}
			if (sendPtt) {
				sendKeyPress(pttRaw, true);
				pttActive = true;
			} else if (pttActive) {
				sendKeyPress(pttRaw, false);
				pttActive = false;
			}
		}
	}

	void MainState::showSoundboards() noexcept {
		ImGui::BeginDisabled(!playback[0].stream);

		if (browseData.ready) {
			soundboards.push_back(std::move(browseData.result));
			browseData.ready = false;
		}

		for (size_t boardIndex = 0; boardIndex < soundboards.size();) {
			Soundboard& board = soundboards[boardIndex];

			bool keep = true;
			const std::string boardName = board.path.filename().string();
			ImGui::SetNextWindowSize(ImVec2(928, 719), ImGuiCond_FirstUseEver);

			ImGui::PushID(static_cast<int>(boardIndex));
			ImGui::Begin(boardName.c_str(), &keep);

			constexpr ImVec2 soundButtonSize(180.0f, 64.0f);
			const int columns = std::max(1, static_cast<int>((ImGui::GetWindowContentRegionMax().x - 32.0f) / soundButtonSize.x));

			if (ImGui::BeginTable("soundTable", columns)) {
				ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(10.0f, 10.0f));
				int c = 0;
				for (size_t soundIndex = 0; soundIndex < board.sounds.size(); soundIndex++) {
					ImGui::TableNextColumn();

					Sound& sound = board.sounds[soundIndex];
					std::string name = sound.getPath().filename().string();
					if (*sound.getHotkeyId() != nullHotkey) {
						name += std::format("\n({})", getHotkeyName(*sound.getHotkeyId()));
					}

					if (ImGui::Button(name.c_str(), soundButtonSize)) {
						tryPlay(sound);
					} else if (ImGui::BeginPopupContextItem(name.c_str(), ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverExistingPopup)) {
						if (ImGui::MenuItem("Add hotkey")) {
							keyAssign.showMenu = true;
							keyAssign.id = sound.getHotkeyId();
							keyAssign.action = [this, boardIndex, soundIndex]() {
								tryPlay(soundboards[boardIndex].sounds[soundIndex]);
								};
						} else if (ImGui::MenuItem(("Set volume"))) {
							soundVolumeMenu.showMenu = true;
							soundVolumeMenu.sound = soundIndex;
							soundVolumeMenu.board = boardIndex;
						}
						ImGui::EndPopup();
					}

					if (++c >= columns) {
						ImGui::TableNextRow();
						c = 0;
					}
				}
				ImGui::PopStyleVar();
				ImGui::EndTable();
			}

			bool refreshRequested = false;
			if (ImGui::BeginPopupContextWindow(nullptr, ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverExistingPopup)) {
				if (ImGui::MenuItem("Refresh")) {
					refreshRequested = true;
				} else if (ImGui::MenuItem("Open folder location")) {
					if (!SDL_OpenURL(board.path.string().c_str())) {
						//showErrorMessage("Error", "Unable to open folder location. Make sure the directory exists and is accessible.", app->getWindow());
						SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Unable to open folder location. Make sure the directory exists and is accessible.", app->getWindow());
					}
				}
				ImGui::EndPopup();
			}

			ImGui::End();
			ImGui::PopID();

			if (!keep) {
				soundboards.erase(soundboards.begin() + boardIndex);
			} else {
				boardIndex++;
				if (refreshRequested) {
					refresh(board);
				}
			}
		}
		ImGui::EndDisabled();
	}

	void MainState::showOptions() noexcept {
		ImGui::Begin("Options", nullptr);
		const float selectablesWidth = 290.0f;

		ImGui::PushTextWrapPos(ImGui::GetWindowWidth() - 16.0f);

		ImGui::Text("Output device");
		ImGui::SetNextItemWidth(selectablesWidth);
		if (ImGui::Combo("##output1", &playback[0].deviceIndex, deviceNames.data(), static_cast<int>(deviceNames.size()), 10)) {
			resetAudioStream(0, true);
		}
		if (!playback[0].stream) {
			showStreamErrorText();
		}

		if (ImGui::Checkbox("Add secondary output", &dualPlayback)) {
			if (!dualPlayback) {
				playback[1].stream.reset();
			} else if (!playback[1].stream) {
				resetAudioStream(1, false);
			}
		}

		if (dualPlayback) {
			ImGui::Text("Secondary output device");
			ImGui::SetNextItemWidth(selectablesWidth);
			if (ImGui::Combo("##output2", &playback[1].deviceIndex, deviceNames.data(), static_cast<int>(deviceNames.size()), 10)) {
				resetAudioStream(1, true);
			}
			if (!playback[1].stream) {
				showStreamErrorText();
			}
		}

		ImGui::NewLine();
		if (ImGui::Button("New soundboard", buttonSize)) {
			SDL_ShowOpenFolderDialog(browseFiles, &browseData, app->getWindow(), nullptr, false);
		}
		ImGui::NewLine();

		ImGui::Text("Volume (gain)");
		showGainSlider(0);
		if (dualPlayback) {
			showGainSlider(1);
		}

		ImGui::BeginDisabled(SDL_GetAudioStreamAvailable(playback[0].stream.get()) == 0);
		if (ImGui::Button("Stop", buttonSize)) {
			stop();
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		if (ImGui::Button("Add stop hotkey", buttonSize)) {
			keyAssign.showMenu = true;
			keyAssign.id = &stopHotkey;
			keyAssign.action = [this]() {
				stop();
				};
		}
		const std::string stopHotkeyLabel = std::format("Stop hotkey: {}.", getHotkeyName(stopHotkey));
		ImGui::Text(stopHotkeyLabel.c_str());
		ImGui::NewLine();

		ImGui::Checkbox("Send push-to-talk key", &usePtt);
		
		ImVec4 textCol = ImGui::GetStyleColorVec4(ImGuiCol_Text);
		textCol.w = 0.7f;
		ImGui::PushStyleColor(ImGuiCol_Text, textCol);
		ImGui::Text("Send your assigned key to the operating system when playing a sound to trigger your game's push-to-talk.");
		ImGui::NewLine();
		ImGui::PopStyleColor();

		if (ImGui::Button("Add push-to-talk key", buttonSize)) {
			pttAssign.showMenu = true;
		}
		const ImVec2 addPttButtonSize = ImGui::GetItemRectSize();

		const std::string pttLabel = std::format("Current key: {}.", getPushToTalkKeyName());
		ImGui::Text(pttLabel.c_str());

		if (ImGui::Button("Add toggle hotkey", addPttButtonSize)) {
			keyAssign.showMenu = true;
			keyAssign.id = &pttToggleHotkey;
			keyAssign.action = [this]() {
				usePtt = !usePtt;
			};
		}
		const std::string pttToggleHotkeyLabel = std::format("Push-to-talk toggle: {}.", getHotkeyName(pttToggleHotkey));
		ImGui::Text(pttToggleHotkeyLabel.c_str());

		ImGui::NewLine();
		ImGui::Text("Theme");
		if (ImGui::Combo("##theme", &theme, "Light\0Dark\0ImGUI Dark\0ImGUI Light")) {
			setTheme();
		}
		ImGui::NewLine();

		if (ImGui::Checkbox("Minimize to tray", &minimizeToTray)) {
			if (minimizeToTray) {
				createTray();
			} else {
				app->tray.reset();
			}
		}
		ImGui::BeginDisabled(!minimizeToTray);
		ImGui::Checkbox("Start minimized", &startMinimized);
		ImGui::EndDisabled();

		if (ImGui::Checkbox("Open on startup", &openOnStartup)) {
			openOnStartup = setLaunchOnStartup(openOnStartup, app->getWindow());
		}

		ImGui::NewLine();

		if (ImGui::Button("Check for updates", buttonSize)) {
			SDL_OpenURL("https://github.com/goodguyartem/VIBoard/releases");
		}
		ImGui::SameLine();
		ImGui::BeginDisabled();
		if (ImGui::Button("Show changelogs", ImGui::GetItemRectSize())) {
		}
		ImGui::EndDisabled();

		if (ImGui::Button("Show welcome screen", buttonSize)) {
			showWelcome = true;
			loadExampleSoundboard();
		}

		ImGui::End();
	}

	void MainState::showKeyAssign() noexcept {
		ImGui::OpenPopup("Assign a Hotkey");
		if (ImGui::BeginPopupModal("Assign a Hotkey", &keyAssign.showMenu, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings)) {
			ImGui::Text("You can assign a keybind to trigger a button even when the program is minimized.");
			ImGui::Text("Current key:");

			if (!keyAssign.assigning) {
				ImGui::PushFont(app->fonts[3]);

				assert(keyAssign.id);
				const std::string name = getHotkeyName(*keyAssign.id);
				ImGui::Text("%s", name.c_str());

				ImGui::PopFont();
#ifdef VI_PLATFORM_WINDOWS
				if (isValidHotkey(*keyAssign.id)) {
					const Hotkey& hotkey = getHotkey(*keyAssign.id);
					if ((hotkey.scancode == SDL_SCANCODE_NUMLOCKCLEAR || hotkey.scancode == SDL_SCANCODE_SCROLLLOCK) && (hotkey.mod & ~SDL_KMOD_NUM) != SDL_KMOD_NONE) {
						ImGui::TextColored(ImVec4(0.8f, 0.1f, 0.1f, 1.0f), "Current hotkey may not behave as expected due to system behaviour.");
					}
				}
#endif

				ImGui::NewLine();

				if (ImGui::Button("OK")) {
					keyAssign.showMenu = false;
				}
				ImGui::SameLine();
				if (ImGui::Button("Change key")) {
					keyAssign.assigning = true;
				}

			} else {
				ImGui::PushFont(app->fonts[3]);

				const std::string mods = modsToString(stripUnsupportedMods(SDL_GetModState()));
				ImGui::Text("%s", mods.c_str());

				ImGui::PopFont();
				ImGui::NewLine();

				ImGui::Text("Press key(s) you would like to assign, or [Del] to remove.");
				if (ImGui::Button("Cancel")) {
					keyAssign.assigning = false;
				}
			}
			ImGui::EndPopup();
		}
	}

	void MainState::showPushToTalkAssign() noexcept {
		ImGui::SetNextWindowSizeConstraints(ImVec2(600.0f, 0.0f), ImVec2(600.0f, 700.0f));
		ImGui::OpenPopup("Assign a push-to-talk key");
		if (ImGui::BeginPopupModal("Assign a push-to-talk key", &pttAssign.showMenu, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings)) {
			ImGui::PushTextWrapPos(ImGui::GetWindowWidth() - 16.0f);
			ImGui::Text("The program can try to trigger a game's push-to-talk functionality by sending a key you specify here to your operating system.\n");
			ImGui::Text("This may not work in all games. If your game is running with admin privileges, you may have to also run ViBoard as an admin.");
			ImGui::NewLine();
			ImGui::Text("Current key:");

			if (!pttAssign.assigning) {
				ImGui::PushFont(app->fonts[3]);
				const std::string name = getPushToTalkKeyName();
				ImGui::Text("%s", name.c_str());
				ImGui::PopFont();

				ImGui::NewLine();
				if (ImGui::Button("OK")) {
					pttAssign.showMenu = false;
				}
				ImGui::SameLine();
				if (ImGui::Button("Change key")) {
					pttAssign.assigning = true;
				}

			} else {
				ImGui::NewLine();
				ImGui::Text("Press a key you would like to assign, or [Del] to remove.");
				
				if (ImGui::Button("Cancel")) {
					pttAssign.assigning = false;
				}
			}
			ImGui::PopTextWrapPos();
			ImGui::EndPopup();
		}
	}

	void MainState::showSoundVolumeMenu() noexcept {
		Sound& sound = soundboards[soundVolumeMenu.board].sounds[soundVolumeMenu.sound];
		ImGui::OpenPopup("Sound Volume (Gain)");
		if (ImGui::BeginPopupModal("Sound Volume (Gain)", &soundVolumeMenu.showMenu, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings)) {
			const std::string soundName = sound.getPath().filename().string();
			ImGui::Text("Set a custom volume for %s.", soundName.c_str());
			ImGui::NewLine();

			showGainOverrideSlider(sound, 0);
			if (dualPlayback) {
				showGainOverrideSlider(sound, 1);
			}
			ImGui::NewLine();

			if (ImGui::Button("OK")) {
				soundVolumeMenu.showMenu = false;
			}
			ImGui::EndPopup();
		}
	}

	void MainState::showWelcomeScreen() noexcept {
		ImGui::Begin("Welcome", &showWelcome);
		ImGui::PushTextWrapPos(ImGui::GetWindowWidth() - 16.0f);

		ImGui::PushFont(app->fonts[2]);
		ImGui::Text("Welcome to ViBoard!");
		ImGui::PopFont();
		ImGui::PushFont(app->fonts[4]);
		ImGui::Text("Version Beta 1.4.0");
		ImGui::PopFont();
		ImGui::NewLine();

		ImGui::PushFont(app->fonts[1]);
		ImGui::SeparatorText("Getting Started");
		ImGui::PopFont();
		ImGui::Text("Select your preferred output device in the options tab. "
			"If you would like to combine your soundboard with your microphone input, "
			"download the virtual audio device from VB-CABLE "
			"and select the 16ch cable as your secondary output device.");

		ImGui::NewLine();
		if (ImGui::Button("Download VB-Cable", buttonSize)) {
			SDL_OpenURL("https://vb-audio.com/Cable/");
		}
		ImGui::NewLine();

		ImGui::Text("You also need to configure your microphone to send its audio to the virtual device.");
		ImGui::Text("On Windows, you can do this by going into Settings > System > Sound, and clicking \"More sound settings\" near the bottom.\n"
			"In the Sound pop-up window, click on the \"Recording\" tab, then double-click your preferred microphone.\n"
			"Under the \"Listen\" tab, tick \"Listen to this device\", and select VB-Audio as the output device.");

#ifdef VI_PLATFORM_WINDOWS
		ImGui::NewLine();
		if (ImGui::Button("Open sound settings", buttonSize)) {
			SDL_OpenURL("ms-settings:sound");
		}
#endif
		ImGui::NewLine();
		ImGui::Text("Your virtual VB-Cable device should now be receiving your microphone input.\n"
			"VB-Cable works by sending anything that goes to VB-Cable output (virtual speaker device) to VB-Cable input (virtual microphone device).");
		ImGui::Text("Be sure that your game or the application you are using is using VB-Cable as its input device rather than your microphone.");
		ImGui::NewLine();

		ImGui::PushFont(app->fonts[1]);
		ImGui::SeparatorText("Soundboards");
		ImGui::PopFont();

		ImGui::Text("Sound effects are organized by folders. Each folder you add goes to a seperate soundboard window.\n"
			"You can drag around any window anywhere you'd like, and even dock them or add them as tabs by dragging them to another window's title bar.");
		ImGui::NewLine();
		ImGui::Text("Right-click on a sound to assign a keyboard shortcut to it or adjust its volume.\n"
			"Play around with the Example Soundboard tab to get a feel for how it works.");
		ImGui::Text("Currently only .mp3 and .wav files are supported, with support for .ogg on the way.");
		ImGui::NewLine();

		ImGui::PushFont(app->fonts[1]);
		ImGui::SeparatorText("Bug Reports");
		ImGui::PopFont();
		ImGui::Text("As this is currently Beta software, there are bound to be issues. You can submit any issues or ideas to www.github.com/goodguyartem/viboard.");
		ImGui::Text("Written by goodguyartem. Have fun!");
		ImGui::NewLine();

		if (ImGui::Button("Go to page", buttonSize)) {
			SDL_OpenURL("https://www.github.com/goodguyartem/viboard");
		}
		ImGui::NewLine();

		ImGui::PopTextWrapPos();
		ImGui::End();
	}

	void MainState::showGainOverrideSlider(Sound& sound, size_t index) noexcept {
		GainOverride gain = sound.getGainOverride(index);
		ImGui::Checkbox(std::format("Output {}", index + 1).c_str(), &gain.use);
		ImGui::BeginDisabled(!gain.use);

		ImGui::PushID(static_cast<int>(index));
		ImGui::SliderFloat("##gainOverrideSlider", &gain.gain, 0.0f, 2.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
		ImGui::PopID();

		ImGui::EndDisabled();
		sound.setGainOverride(index, gain);
	}

	void MainState::resetAudioStream(size_t index, bool makePreferred) noexcept {
		PlaybackConfig& config = playback[index];
		AudioStreamOwner& stream = config.stream;

		assert(config.deviceIndex < static_cast<int>(audioDevices.size()));
		stream.reset(SDL_OpenAudioDeviceStream(audioDevices[config.deviceIndex], nullptr, nullptr, nullptr));
		if (makePreferred) {
			config.preferred = deviceNames[config.deviceIndex];
		}

		VI_INFO("Reset audio stream %zu.", index);

		if (!stream) {
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
				"Failed to open audio stream!",
				std::format("Could not open an audio stream using device {}.\n"
					"There may be an issue with the device, please select a different device.\n\n"
					"Note: if you are using VB-Cable, be sure to select the 16ch variant.\n",
					deviceNames[config.deviceIndex]).c_str(),
				app->getWindow());
			return;
		}

		SDL_SetAudioStreamGain(stream.get(), config.gain);
	}

	void MainState::tryPlay(const Sound& sound) noexcept {
		try {
			const bool playS0 = playback[0].stream != nullptr;
			const bool playS1 = dualPlayback && playback[1].stream && playback[0].deviceIndex != playback[1].deviceIndex;

			if (playS0 || playS1) {
				if (playS0) {
					sound.play(playback[0].stream.get(), 0);
				}
				if (playS1) {
					sound.play(playback[1].stream.get(), 1);
				}
				if (pttScancode != SDL_SCANCODE_UNKNOWN && usePtt) {
					app->canSleep = false;
				}
			}
		} catch (const std::exception& e) {
			const std::string message = std::format(
				"Unable to play sound \"{}\".\n"
				"Please ensure the sound is in correct format or try a different playback device.\n\n"
				"Error: {}",
				sound.getPath().filename().string(),
				e.what()
			);

			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Failed to play sound!", message.c_str(), app->getWindow());
		}
	}

	void MainState::stop() const noexcept {
		for (const PlaybackConfig& config : playback) {
			if (config.stream) {
				SDL_AudioStream* stream = config.stream.get();
				SDL_PauseAudioStreamDevice(stream);
				SDL_ClearAudioStream(stream);
			}
		}
	}

	void MainState::serialize() {
		using namespace nlohmann;
		json file;

		file["showWelcome"] = showWelcome;

		json& boardArray = file["soundboards"];
		for (const Soundboard& board : soundboards) {
			json& boardJson = boardArray.emplace_back();
			boardJson["path"] = board.path;

			json& soundsMap = boardJson["sounds"];
			for (const Sound& sound : board.sounds) {
				json& soundJson = soundsMap[sound.getPath().string()];

				for (size_t i = 0; i < playback.size(); i++) {
					soundJson["gains"].emplace_back(sound.getGainOverride(i));
				}

				if (isValidHotkey(*sound.getHotkeyId())) {
					soundJson["hotkey"] = serializeHotkey(*sound.getHotkeyId());
				} else {
					soundJson["hotkey"] = nullptr;
				}
			}
		}

		for (const PlaybackConfig& config : playback) {
			json& configJson = file["playback"].emplace_back();
			configJson["preferred"] = config.preferred;
			configJson["gain"] = config.gain;
		}
		file["dualPlayback"] = dualPlayback;

		if (isValidHotkey(stopHotkey)) {
			file["stopHotkey"] = serializeHotkey(stopHotkey);
		} else {
			file["stopHotkey"] = nullptr;
		}
		file["theme"] = theme;

		file["minimizeToTray"] = minimizeToTray;
		file["startMinimized"] = startMinimized;

		file["pttScancode"] = pttScancode;
		file["pttRaw"] = pttRaw;
		file["usePtt"] = usePtt;
		file["pttToggleHotkey"] = serializeHotkey(pttToggleHotkey);

		const bool maximized = SDL_GetWindowFlags(app->getWindow()) & SDL_WINDOW_MAXIMIZED;
		file["maximized"] = maximized;
		if (!maximized) {
			SDL_Rect windowBounds;
			if (SDL_GetWindowPosition(app->getWindow(), &windowBounds.x, &windowBounds.y)) {
				file["windowX"] = windowBounds.x;
				file["windowY"] = windowBounds.y;
			}
			if (SDL_GetWindowSize(app->getWindow(), &windowBounds.w, &windowBounds.h)) {
				file["windowWidth"] = windowBounds.w;
				file["windowHeight"] = windowBounds.h;
			}
		} else {
			file["windowX"] = 0;
			file["windowY"] = 0;
			file["windowWidth"] = 0;
			file["windowHeight"] = 0;
		}

		std::ofstream stream(settingsPath, std::ofstream::trunc);
		stream << file;
		if (!stream) {
			throw IOError("File stream in error state after write.");
		}
	}

	void MainState::deserialize() {
		using namespace nlohmann;
		std::ifstream stream(settingsPath);

		json file;
		stream >> file;
		if (!stream) {
			throw IOError("Stream in error state after read.");
		}

		file["showWelcome"].get_to(showWelcome);

		for (const json& boardJson : file["soundboards"]) {
			fs::path boardPath = boardJson["path"].get<fs::path>();
			if (!fs::exists(boardPath)) {
				soundboards.emplace_back(std::move(boardPath));
				continue;
			}

			const std::string pathStr = boardPath.string();
			const char* pathCStr = pathStr.c_str();

			browseFiles(&browseData, &pathCStr, 0);
			soundboards.push_back(std::move(browseData.result));
			browseData.ready = false;

			for (size_t i = 0; i < soundboards.back().sounds.size(); i++) {
				Sound& sound = soundboards.back().sounds[i];

				const json& soundsJson = boardJson["sounds"];
				const auto it = soundsJson.find(sound.getPath());
				if (it == soundsJson.end()) {
					continue;
				}

				for (size_t i = 0; i < playback.size(); i++) {
					sound.setGainOverride(i, it->at("gains")[i].get<GainOverride>());
				}

				if (!it->at("hotkey").is_null()) {
					Hotkey hotkey = deserializeHotkey(it->at("hotkey"));
					const size_t soundboardIndex = soundboards.size() - 1;
					hotkey.callback = [this, i, soundboardIndex]() {
						tryPlay(soundboards[soundboardIndex].sounds[i]);
						};
					*sound.getHotkeyId() = registerHotkey(hotkey);
				}
			}
		}

		for (size_t i = 0; i < playback.size(); i++) {
			PlaybackConfig& config = playback[i];
			const json& configJson = file["playback"][i];

			configJson["preferred"].get_to(config.preferred);

			const float gain = configJson["gain"].get<float>();
			if (gain < 0.0f || gain > 2.0f) {
				throw IOError("Bad gain.");
			}
			config.gain = gain;
		}

		file["dualPlayback"].get_to(dualPlayback);
		if (!file["stopHotkey"].is_null()) {
			Hotkey hotkey = deserializeHotkey(file["stopHotkey"]);
			hotkey.callback = [this]() {
				stop();
				};
			stopHotkey = registerHotkey(hotkey);
		}

		const int theme = file["theme"].get<int>();
		if (theme < 0 || theme > 3) {
			throw IOError("Bad theme.");
		}
		this->theme = theme;

		file["minimizeToTray"].get_to(minimizeToTray);
		file["startMinimized"].get_to(startMinimized);

		file["pttScancode"].get_to(pttScancode);
		file["pttRaw"].get_to(pttRaw);
		file["usePtt"].get_to(usePtt);

		if (!file["pttToggleHotkey"].is_null()) {
			Hotkey hotkey = deserializeHotkey(file["pttToggleHotkey"]);
			hotkey.callback = [this]() {
				usePtt = !usePtt;
			};
			pttToggleHotkey = registerHotkey(hotkey);
		}

		if (file["maximized"].get<bool>()) {
			SDL_MaximizeWindow(app->getWindow());
		} else {
			SDL_Rect windowBounds;
			file["windowX"].get_to(windowBounds.x);
			file["windowY"].get_to(windowBounds.y);
			file["windowWidth"].get_to(windowBounds.w);
			file["windowHeight"].get_to(windowBounds.h);
		
			SDL_SetWindowPosition(app->getWindow(), windowBounds.x, windowBounds.y);
			SDL_SetWindowSize(app->getWindow(), windowBounds.w, windowBounds.h);
		}
	}

	void MainState::setTheme() const noexcept {
		switch (theme) {
		case 0:
			setLightTheme();
			break;
		case 1:
			setDarkTheme();
			break;
		case 2:
			ImGui::StyleColorsDark();
			break;
		case 3:
			ImGui::StyleColorsLight();
			break;
		case 4:
			assert(false);
			break;
		}
	}

	void MainState::loadExampleSoundboard() noexcept {
		for (const Soundboard& board : soundboards) {
			if (fs::relative(board.path) == "res/Example Soundboard") {
				return;
			}
		}
		static const char* fileList = "res/Example Soundboard";
		browseFiles(&browseData, &fileList, 0);
	}

	void MainState::onExitError(const char* error) noexcept {
		assert(error);
		std::string message = "Unable to save user settings.\n"
			"They will be reset on next program launch.\n"
			"Please make sure the program has the necessary write permissions.";
		if (error[0] != '\0') {
			message += "\n\nError: ";
			message += error;
		}

		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error saving settings!", message.c_str(), app->getWindow());
	}

	void MainState::createTray() noexcept {
		app->tray.reset(SDL_CreateTray(app->getTrayIcon(), VI_EXECUTEABLE_NAME));
		if (!app->tray) {
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error creating system tray", "An error has occured while trying to create a system tray icon.", app->getWindow());
			minimizeToTray = false;
			return;
		}

		SDL_TrayMenu* menu = SDL_CreateTrayMenu(app->tray.get());
		SDL_TrayEntry* showEntry = SDL_InsertTrayEntryAt(menu, -1, "Show", SDL_TRAYENTRY_BUTTON);

		SDL_SetTrayEntryCallback(showEntry,
			[](void* userData, SDL_TrayEntry* entry) {
				const Application& app = *static_cast<Application*>(userData);

				SDL_ShowWindow(app.getWindow());
				SDL_RestoreWindow(app.getWindow());
			},
			app);

		SDL_TrayEntry* quitEntry = SDL_InsertTrayEntryAt(menu, -1, "Quit", SDL_TRAYENTRY_BUTTON);

		SDL_SetTrayEntryCallback(quitEntry,
			[](void* userData, SDL_TrayEntry* entry) {
				static_cast<Application*>(userData)->quit();
			},
			app);
	}
}