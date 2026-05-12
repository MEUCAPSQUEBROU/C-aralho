# Linux build (uses raylib built locally with OpenGL 2.1)
CXX        = g++
RAYLIB_DIR = vendor/raylib/src
CXXFLAGS   = -std=c++17 -O2 -Wall -I$(RAYLIB_DIR)
LIBS       = $(RAYLIB_DIR)/libraylib.a -lGL -lm -lpthread -ldl -lrt -lX11

# Windows cross-build (uses raylib built with mingw-w64)
WIN_CXX        = x86_64-w64-mingw32-g++
WIN_RAYLIB_DIR = vendor/raylib-win
WIN_CXXFLAGS   = -std=c++17 -O2 -Wall -I$(WIN_RAYLIB_DIR) -DUNICODE -D_UNICODE
WIN_LIBS       = $(WIN_RAYLIB_DIR)/libraylib.a -lopengl32 -lgdi32 -lwinmm -static -lpthread

plane: main.cpp
	$(CXX) $(CXXFLAGS) main.cpp -o plane $(LIBS)

plane.exe: main.cpp
	$(WIN_CXX) $(WIN_CXXFLAGS) main.cpp -o plane.exe $(WIN_LIBS)

run: plane
	./plane

win: plane.exe

# Package the Windows build with assets/ ready to ship
dist-win: plane.exe
	rm -rf dist/plane-win && mkdir -p dist/plane-win
	cp plane.exe dist/plane-win/
	cp -r assets dist/plane-win/
	tar -czf dist/plane-win.tar.gz -C dist plane-win
	@echo "Created dist/plane-win/ and dist/plane-win.tar.gz"

clean:
	rm -f plane plane.exe
	rm -rf dist
