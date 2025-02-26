#ifdef VI_PLATFORM_WINDOWS
#include "Platform.h"
#include "../Exceptions.h"

#include <string>
#include <filesystem>
#include <format>

#include <Windows.h>
#include <Shlobj_core.h>
#include <winnls.h>
#include <shobjidl.h>
#include <objbase.h>
#include <objidl.h>
#include <shlguid.h>

using namespace std::string_literals;
namespace fs = std::filesystem;

namespace vi {
	namespace {
		fs::path getStartupFolderPath() {
			PWSTR pszPath;
			const HRESULT hr = SHGetKnownFolderPath(FOLDERID_Startup, 0, NULL, &pszPath);
			if (SUCCEEDED(hr)) {
				const fs::path path(pszPath);
				CoTaskMemFree(static_cast<LPVOID>(pszPath));
				return path;
			}
			throw ExternalError("SHGetKnownFolderPath failed.");
		}

		inline fs::path getStartupShortcut() {
			return getStartupFolderPath() / (VI_EXECUTEABLE_NAME + ".lnk"s);
		}

		HRESULT createLink(LPCWSTR targetPath, LPCSTR linkPath, LPCWSTR workingDir, LPCWSTR description) noexcept {
			// Get a pointer to the IShellLink interface. It is assumed that CoInitialize
			// has already been called.
			IShellLink* psl;
			HRESULT result = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLink, reinterpret_cast<LPVOID*>(&psl));
			if (SUCCEEDED(result)) {
				// Set the path to the shortcut target and add the description. 
				psl->SetPath(targetPath);
				psl->SetDescription(description);
				psl->SetWorkingDirectory(workingDir);

				IPersistFile* ppf;
				// Query IShellLink for the IPersistFile interface, used for saving the 
				// shortcut in persistent storage. 
				result = psl->QueryInterface(IID_IPersistFile, reinterpret_cast<LPVOID*>(&ppf));

				if (SUCCEEDED(result)) {
					std::wstring wsz(MAX_PATH, L'\0');

					// Ensure that the string is Unicode. 
					MultiByteToWideChar(CP_ACP, 0, linkPath, -1, wsz.data(), MAX_PATH);

					// Save the link by calling IPersistFile::Save. 
					result = ppf->Save(wsz.c_str(), true);
					ppf->Release();
				}
				psl->Release();
			}
			return result;
		}
	}

	void initPlatform() {
		const HRESULT result = CoInitialize(nullptr);
		if (result != S_OK && result != S_FALSE) {
			throw ExternalError("CoInitialize failed.");
		}
	}

	void quitPlatform() noexcept {
		CoUninitialize();
	}

	bool isLaunchingOnStartup() {
		return fs::exists(getStartupShortcut());
	}

	bool setLaunchOnStartup(bool launch, SDL_Window* window) {
		if (!launch) {
			const auto shortcut = getStartupShortcut();
			if (fs::exists(shortcut)) {
				fs::remove(shortcut);
			}
			return false;
		} else {
			const auto exePath = fs::current_path() / std::format("{}.exe", VI_EXECUTEABLE_NAME);
			if (FAILED(createLink(exePath.c_str(), getStartupShortcut().string().c_str(), fs::current_path().c_str(), L"Launch ViBoard"))) {
				SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
					"Unable to open on startup",
					"An error occured while trying to add program to startup programs.\n"
					"Try running the program with administrative privileges.", 
					window);
				return false;
			}
			return true;
		}
	}

	void sendKeyPress(uint16_t scancode, bool pressed) noexcept {
		INPUT input = {};
		input.type = INPUT_KEYBOARD;
		input.ki.wVk = MapVirtualKey(scancode, MAPVK_VSC_TO_VK);
		input.ki.wScan = scancode;
		if (!pressed) {
			input.ki.dwFlags = KEYEVENTF_KEYUP;
		}
		SendInput(1, &input, sizeof(INPUT));
	}
}
#endif