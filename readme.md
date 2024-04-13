A clone of the 1976 arcade version of [Breakout](https://en.wikipedia.org/wiki/Breakout_(video_game)?&useskin=vector).

![](https://files.catbox.moe/crmcxq.png)

## Controls
**A controller is required.**  
`start` start the game  
`left stick` move the paddle  

## Keybindings
`F5` reset  
`F11` toggle between windowed mode and the chosen fullscreen mode. Will default to borderless if no fullscreen mode is set.

## Configuration
A graphics configuration file can be found in `data`.  

# Build
## Windows
### Requirements
- [MSYS2](https://code.visualstudio.com/docs/cpp/config-mingw#_installing-the-mingww64-toolchain): follow the 'Installing the MinGW-w64 toolchain' instructions.
- [Make](https://gnuwin32.sourceforge.net/packages/make.htm): after installation, add `\bin` to your path.
- [SDL2](https://github.com/libsdl-org/SDL/releases/latest): get SDL2-devel-x.x.x-mingw.
- [SDL_Mixer](https://github.com/libsdl-org/SDL_mixer/releases/latest): get SDL2_mixer-devel-x.x.x-mingw.
- [SDL_Image](https://github.com/libsdl-org/SDL_image/releases/latest): get SDL2_image-devel-x.x.x-mingw.

For SDL2, SDL_Mixer and SDL_image, extract the contents of `\x86_64-w64-mingw32\` into a directory of your choosing. Then copy the `.dll` files from `\bin` into the root of the cloned repository folder.

Open `makefile` and change the `sdl_path` variable to your SDL2 install directory.

Run `make release`.
