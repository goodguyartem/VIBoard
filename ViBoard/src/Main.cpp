#include "Exceptions.h"
#include "Log.h"
#include "Application.h"

#include <imgui.h>

#include <SDL3/SDL.h>

#include <string>
#include <exception>
#include <fstream>
#include <stdlib.h>

using namespace std::string_literals;

namespace vi {
	namespace {
		void init() {
			SDL_SetLogPriorities(SDL_LOG_PRIORITY_INFO);
			if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
				throw ExternalError("Failed to initialize SDL: "s + SDL_GetError());
			}

			IMGUI_CHECKVERSION();
			ImGui::CreateContext();

			ImGuiIO& io = ImGui::GetIO();
			io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
			io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
			io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		}

		void quit() noexcept {
			SDL_Quit();
			ImGui::DestroyContext();
		}

		void onError(const char* error) noexcept {
			assert(error);
			std::string message = "An unhandled error has occured!\n"
				"Please report this on https://www.github.com/goodguyartem/viboard.";
			if (error[0] != '\0') {
				message += "\n\nError: ";
				message += error;
			}

			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "FATAL CRASH!", message.c_str(), nullptr);
		}
	}
}

int main() {
	static const auto instancePath = vi::storagePath / "ViBoard.instance";
	std::error_code ec;
	if (std::filesystem::exists(instancePath) && !std::filesystem::remove(instancePath, ec)) {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Unable to launch!", "The program is already running! How silly of you...", nullptr);
		return EXIT_SUCCESS;
	}
	std::ofstream instanceCheck(instancePath, std::ofstream::trunc);

	SDL_SetAppMetadata("ViBoard", "Beta 1.4.1", nullptr);

	int exit = EXIT_FAILURE;
	try {
		vi::init();

		vi::Application app;
		app.run();
		exit = EXIT_SUCCESS;

	} catch (const std::exception& e) {
		VI_CRITICAL("Fatal crash! %s", e.what());
		vi::onError(e.what());
	} catch (...) {
		VI_CRITICAL("Fatal crash! (catch-all)");
		vi::onError("unknown exception");
	}

	instanceCheck.close();
	std::filesystem::remove(instancePath);

	vi::quit();
	return exit;
}