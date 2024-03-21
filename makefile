name = breakout-clone
version = -std=c++23
warnings = -Wall -Wextra -Wconversion -Wsign-conversion
sdl_path = C:/cpplibraries/SDL2


debug:
	g++ $(version) $(warnings) src/*.cpp -o$(name).exe -Isrc -I$(sdl_path)/include -L$(sdl_path)/lib -lmingw32 -lSDL2main -lSDL2 -lSDL2_mixer -lSDL2_image

release:
	g++ $(version) -O3 -mwindows src/*.cpp -o$(name).exe -Isrc -I$(sdl_path)/include -L$(sdl_path)/lib -lmingw32 -lSDL2main -lSDL2 -lSDL2_mixer -lSDL2_image