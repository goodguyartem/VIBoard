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

#include "platform/HotKey.h"
#include "Exceptions.h"

#include <SDL3/SDL.h>

#include <nlohmann/json.hpp>

#include <filesystem>
#include <memory>
#include <array>
#include <assert.h>

namespace vi {
	using AudioStreamOwner = std::unique_ptr<SDL_AudioStream, decltype(&SDL_DestroyAudioStream)>;

	struct GainOverride {
		float gain = 1.0f;
		bool use = false;
	};

	class Sound {
	public:
		Sound() = default;
		Sound(std::filesystem::path path);

		Sound(const Sound&) = delete;
		Sound& operator=(const Sound&) = delete;

		Sound(Sound&& other) noexcept;
		Sound& operator=(Sound&& other) noexcept;

		~Sound();

		void load(std::filesystem::path path);		
		void loadMp3(std::filesystem::path path);
		void loadWav(std::filesystem::path path);

		void play(SDL_AudioStream* stream, size_t gainIndex) const;

		const std::filesystem::path& getPath() const noexcept {
			return path;
		}

		GainOverride getGainOverride(size_t index) const noexcept {
			assert(index < gains.size());
			return gains[index];
		}

		void setGainOverride(size_t index, GainOverride gain) noexcept {
			assert(index < gains.size());
			gains[index] = gain;
		}

		const HotkeyId* getHotkeyId() const noexcept {
			return &hotkeyId;
		}

		HotkeyId* getHotkeyId() noexcept {
			return &hotkeyId;
		}

	private:
		std::filesystem::path path;
		
		SDL_AudioSpec spec{};
		uint8_t* buffer = nullptr;
		uint32_t len = 0;
		void(*deleter)(void* ptr) = nullptr;

		std::array<GainOverride, 2> gains;
		HotkeyId hotkeyId = nullHotkey;
	};

	inline bool isSupported(const std::filesystem::path& ext) noexcept {
		return ext == ".mp3" || ext == ".wav";
	}

	inline void to_json(nlohmann::json& json, GainOverride gain) noexcept {
		json["gain"] = gain.gain;
		json["use"] = gain.use;
	}

	void from_json(const nlohmann::json& json, GainOverride& gain);
}