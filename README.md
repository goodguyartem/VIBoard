# VIBoard
A lightweight free and open-source soundboard.
## Key features
* Consumes minimal resources, especially in background.
* A dedicated window for each soundboard you add, with the ability to tab between them and place them anywhere on your display.
* Ability to assign a hotkey to each sound (can be triggered while in almost any application or game).
* Support for outputing to multiple output devices for mixing soundboards with your microphone input (requires [VB Cable](https://vb-audio.com/Cable/)).
* Ability to trigger a game's push-to-talk when playing a sound.
* .mp3 and .wav support.
* Themes. Who doesn't like themes

![Screenshot](https://github.com/goodguyartem/ViBoard/blob/main/screenshots/image2.png?raw=true)
![Screenshot](https://github.com/goodguyartem/ViBoard/blob/main/screenshots/image3.png?raw=true)

## Installing
Simply download the latest build matching your architecture (x64 or x86) from the [Releases](https://github.com/goodguyartem/VIBoard/releases) section and extract the downloaded .zip file into a directory of your choosing (such as C:\Program Files). Currently, only Windows builds are available.

If you would like to mix your microphone input with your soundboard, download [VB Cable](https://vb-audio.com/Cable/) and use it as your secondary output device. See the Getting Started section of the program's Welcome page for setup instructions.

If you get a "Windows Protected Your PC" prompt, it is normal as the program isn't signed. Simply click "More info" then "Run anyway." (Source code is available.)

## Bug Reports / Feature Requests
As the program is currently in beta, there are bound to be issues. I'll do my best to address all submitted issues and feature requests.

## Building
### Windows
Make sure [Premake 5](https://premake.github.io/) is installed and is added to your PATH variable.

Clone the repository with:
```
git clone --recursive https://github.com/goodguyartem/VIBoard.git
```
If you're not using Visual Studio you'll also need to obtain [SDL3](https://github.com/libsdl-org/SDL) and [SDL3_Image](https://github.com/libsdl-org/SDL_image) builds matching your compiler.

Navigate to the repository's directory and run the `premake5.lua` build script to generate project files for your IDE of choice. For example, to generate Visual Studio 2022:
```
premake5 vs2022
```
You can run `premake5 --help` if this is your first time using Premake.

### Other Operating Systems
Currently, the project is only available for Windows, but adding support for other operating systems is trivial as all Windows-specific code is abstracted away. Namely, you'll need to implement system-wide hotkey support by defining the functions:
* `HotkeyId registerHotkey(const Hotkey& hotkey)`, `void unregisterHotkey(HotkeyId id)`, and
* `void processHotkeyPresses(const Application& app) noexcept`
  
the ability to launch the program on system startup via the function:
* `bool setLaunchOnStartup(bool launch, SDL_Window* window)`, and if needed
* `void initPlatform()` and `void quitPlatform() noexcept`.

See `src/platform/`.

## To-do
* .ogg support
