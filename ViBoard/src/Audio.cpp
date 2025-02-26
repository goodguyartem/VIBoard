#include "Audio.h"
#include "Log.h"

#ifdef VI_MSVC
#pragma warning(push)
#pragma warning(disable: 4267)
#pragma warning(disable: 4244)
#endif

#define MINIMP3_IMPLEMENTATION
#include <minimp3_ex.h>

#ifdef VI_MSVC
#pragma warning(pop)
#endif

#include <utility>
#include <stdlib.h>

namespace fs = std::filesystem;

namespace vi {
	Sound::Sound(fs::path path) {
		load(std::move(path));
	}

	Sound::Sound(Sound&& other) noexcept
		: path(std::move(other.path)),
		spec(other.spec),
		buffer(other.buffer),
		len(other.len),
		deleter(other.deleter),
		hotkeyId(other.hotkeyId) {
		
		other.spec = {};
		other.buffer = nullptr;
		other.deleter = nullptr;
		other.hotkeyId = nullHotkey;
	}

	Sound& Sound::operator=(Sound&& other) noexcept {
		path = std::move(other.path);
		
		spec = other.spec;
		other.spec = {};
		buffer = other.buffer;
		other.buffer = nullptr;
		len = other.len;
		deleter = other.deleter;
		other.deleter = nullptr;

		hotkeyId = other.hotkeyId;
		other.hotkeyId = nullHotkey;
		
		return *this;
	}

	Sound::~Sound() {
		if (deleter) {
			deleter(buffer);
			buffer = nullptr;
		}
		try {
			if (isValidHotkey(hotkeyId)) {
				unregisterHotkey(hotkeyId);
			}
		} catch (const std::exception& e) {
			VI_ERROR("%s", e.what());
		}
	}

	void Sound::load(fs::path path) {
		const fs::path ext = path.extension();
		if (ext == ".mp3") {
			loadMp3(std::move(path));
		} else if (ext == ".wav") {
			loadWav(std::move(path));
		} else {
			throw IOError("Unsupported file type: " + ext.string());
		}
	}

	void Sound::loadMp3(fs::path path) {
		assert(path.extension() == ".mp3");
		mp3dec_t mp3d;
		mp3dec_file_info_t info;
		if (mp3dec_load(&mp3d, path.string().c_str(), &info, NULL, NULL)) {
			if (info.buffer) {
				free(info.buffer);
			}
			throw IOError("Error loading " + path.string());
		}

		spec.freq = info.hz;
		spec.channels = info.channels;
		spec.format = SDL_AUDIO_S16;

		buffer = reinterpret_cast<uint8_t*>(info.buffer);
		len = static_cast<uint32_t>(info.samples * sizeof(mp3d_sample_t));

		deleter = free;
		this->path = std::move(path);
	}

	void Sound::loadWav(std::filesystem::path path) {
		assert(path.extension() == ".wav");
		if (!SDL_LoadWAV(path.string().c_str(), &spec, &buffer, &len)) {
			throw IOError(SDL_GetError());
		}
		deleter = SDL_free;
		this->path = std::move(path);
	}

	void Sound::play(SDL_AudioStream* stream, size_t gainIndex) const {
		SDL_PauseAudioStreamDevice(stream);
		SDL_ClearAudioStream(stream);

		SDL_SetAudioStreamFormat(stream, &spec, nullptr);
		if (gains[gainIndex].use) {
			SDL_SetAudioStreamGain(stream, gains[gainIndex].gain);
		}

		if (!SDL_PutAudioStreamData(stream, buffer, len) || !SDL_ResumeAudioStreamDevice(stream)) {
			throw ExternalError(SDL_GetError());
		}
	}

	void from_json(const nlohmann::json& json, GainOverride& gain) {
		json["gain"].get_to(gain.gain);
		if (gain.gain < 0.0f || gain.gain > 2.0f) {
			throw IOError("Bad gain override.");
		}
		json["use"].get_to(gain.use);
	}
}