#include "Application.h"
#include "appStates/MainState.h"
#include "Exceptions.h"
#include "platform/Hotkey.h"
#include "platform/Platform.h"
#include "ImGuiConfig.h"

#include <backends/imgui_impl_sdl3.h>
#include "backends/imgui_impl_opengl3.h"

#ifdef IMGUI_IMPL_OPENGL_ES2
#include <SDL3/SDL_opengles2.h>
#else
#include <SDL3/SDL_opengl.h>
#endif

namespace fs = std::filesystem;

namespace vi {
	void Application::run() {
		init();
		running = true;

		while (running) {
			pollEvents();
			SDL_Delay(5);

			if (!isInactive()) {
				update();
				render();
			}
		}

		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplSDL3_Shutdown();
		SDL_GL_DestroyContext(glContext);

		quitPlatform();
	}

	void Application::init() {
#if defined(IMGUI_IMPL_OPENGL_ES2)
		// GL ES 2.0 + GLSL 100 (WebGL 1.0)
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(IMGUI_IMPL_OPENGL_ES3)
		// GL ES 3.0 + GLSL 300 es (WebGL 2.0)
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(__APPLE__)
		// GL 3.2 Core + GLSL 150
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
		// GL 3.0 + GLSL 130
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
		SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

		initPlatform();

		window.reset(SDL_CreateWindow("ViBoard Beta", 1280, 720, SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN));
		if (!window) {
			throw ExternalError(SDL_GetError());
		}

		glContext = SDL_GL_CreateContext(window.get());
		if (!glContext) {
			throw ExternalError(SDL_GetError());
		}

		ImGui_ImplSDL3_InitForOpenGL(window.get(), glContext);
		ImGui_ImplOpenGL3_Init();

		ImGuiStyle& style = ImGui::GetStyle();
		ImGuiIO& io = ImGui::GetIO();
		io.IniFilename = nullptr;
		style.FrameRounding = 6.0f;
		style.PopupRounding = 6.0f;
		style.WindowPadding = ImVec2(8.0f, 8.0f);

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			style.WindowRounding = 6.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		fonts.push_back(io.Fonts->AddFontFromFileTTF("res/fonts/Poppins-Regular.ttf", 19.0f));
		fonts.push_back(io.Fonts->AddFontFromFileTTF("res/fonts/Poppins-SemiBold.ttf", 32.0f));
		fonts.push_back(io.Fonts->AddFontFromFileTTF("res/fonts/Poppins-SemiBold.ttf", 46.0f));
		fonts.push_back(io.Fonts->AddFontFromFileTTF("res/fonts/Poppins-Regular.ttf", 28.0f));
		fonts.push_back(io.Fonts->AddFontFromFileTTF("res/fonts/Poppins-SemiBold.ttf", 21.0f));

		states.emplace_back(std::make_unique<MainState>(*this));

		SDL_SetWindowIcon(window.get(), icon.get());
	}

	void Application::pollEvents() noexcept {
		processHotkeyPresses(*this);

		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			ImGui_ImplSDL3_ProcessEvent(&event);
			states.back()->onEvent(event);

			switch (event.type) {
			case SDL_EVENT_QUIT:
				quit();
				break;

			case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
				if (tray) {
					SDL_HideWindow(window.get());
				}
				break;
			}
		}
	}

	void Application::update() {
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL3_NewFrame();
		ImGui::NewFrame();

		static ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_None;
		ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDocking;
		if (menuBarCallback) {
			windowFlags |= ImGuiWindowFlags_MenuBar;
		}
		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

		if (dockspaceFlags & ImGuiDockNodeFlags_PassthruCentralNode) {
			windowFlags |= ImGuiWindowFlags_NoBackground;
		}

		ImGui::PushFont(fonts[0]);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("DockSpace", nullptr, windowFlags);
		ImGui::PopStyleVar();

		ImGui::PopStyleVar(2);

		ImGuiIO& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
			dockspaceId = ImGui::GetID("OpenGLAppDockspace");
			ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), dockspaceFlags);
		}

		if (menuBarCallback && ImGui::BeginMenuBar()) {
			menuBarCallback();
			ImGui::EndMenuBar();
		}

		ImGui::PopFont();
		
		states.back()->update();
		ImGui::End();
	}

	void Application::render() const {
		ImGui::Render();
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		SDL_GL_MakeCurrent(window.get(), glContext);

		SDL_GL_SwapWindow(window.get());
	}

	fs::path getStoragePath() noexcept {
		const char* userFolder = SDL_GetUserFolder(SDL_FOLDER_DOCUMENTS);
		if (!userFolder) {
			return "";
		}
		return fs::path(userFolder) / VI_EXECUTEABLE_NAME;
	}
}