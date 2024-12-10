# vim: set tabstop=4 shiftwidth=4 expandtab noexpandtab:

CXXFLAGS = -ggdb3
CXXFLAGS += -std=c++20
CXXFLAGS += -ferror-limit=1
CXXFLAGS += -D__USE_SDL
#CXXFLAGS += -D__USE_GLFW
CXXFLAGS += -I./external/glm
CXXFLAGS += -I./external/SDL/include
CXXFLAGS += -I ./external/stb/
LDLIBS += -lassimp -lz 

ifeq ($(NATIVE),1)
	CXX = clang++
	# TODO use architecture specific install location
 	CXXFLAGS += -I$(HOME)/.local/vortex_deps/assimp-build/installed/native/include
	CXXFLAGS += -I/opt/homebrew/include
 	LDFLAGS += -L$(HOME)/.local/vortex_deps/assimp-build/installed/native/lib
	LDFLAGS += -L /opt/homebrew/lib
	LDLIBS += -framework OpenGL
	LDLIBS += -lGLEW
	LDLIBS += -lglfw
	LDLIBS += -lsdl2
	OUTPUT ?= ./build/program
else
	CXX = em++
	CXXFLAGS += -I$(HOME)/.emscripten_cache/sysroot/include
	LDFLAGS += -L$(HOME)/.emscripten_cache/sysroot/lib        # Nix shell
	LDLIBS += -s USE_ZLIB=1 -sNO_DISABLE_EXCEPTION_CATCHING=1 
	LDLIBS += -s USE_SDL=2
	LDLIBS += -s USE_GLFW=3
	LDLIBS += -s FULL_ES2=1 -s USE_WEBGL2=1 -O0
	LDLIBS += -s ALLOW_MEMORY_GROWTH=1 -s GL_UNSAFE_OPTS=0
	LDLIBS += -s ASSERTIONS=1 -s SAFE_HEAP=1
ifneq ($(wildcard $(APP_ROOT)/shaders),)
	LDLIBS += $(shell ls $(APP_ROOT)/shaders | sed -e \
		's|.*|--preload-file $(APP_ROOT)/shaders/\0@shaders/\0|')
endif
ifneq ($(wildcard $(APP_ROOT)/models),)
	LDLIBS += $(shell ls $(APP_ROOT)/models | sed -e \
		's|.*|--preload-file $(APP_ROOT)/models/\0@models/\0|')
endif
ifneq ($(wildcard $(APP_ROOT)/assets),)
	LDLIBS += $(shell ls $(APP_ROOT)/assets | sed -e \
		's|.*|--preload-file $(APP_ROOT)/assets/\0@assets/\0|')
endif
	OUTPUT ?= ./build/index.html
endif

# All IMGUI stuff
CXXFLAGS += -I./external/imgui
CXXFLAGS += -I./external/imgui/backends
CXXFLAGS += ${CXXFLAGS_EXTRA}
IMGUI_DIR=./external/imgui
IMGUI_SOURCES += $(IMGUI_DIR)/imgui.cpp
IMGUI_SOURCES += $(IMGUI_DIR)/imgui_demo.cpp
IMGUI_SOURCES += $(IMGUI_DIR)/imgui_draw.cpp
IMGUI_SOURCES += $(IMGUI_DIR)/imgui_tables.cpp
IMGUI_SOURCES += $(IMGUI_DIR)/imgui_widgets.cpp
IMGUI_SOURCES += $(IMGUI_DIR)/backends/imgui_impl_sdl2.cpp
IMGUI_SOURCES += $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp

build:
	mkdir -p ./build
	$(CXX) $(CXXFLAGS) $(APP_ROOT)/main.cpp $(IMGUI_SOURCES) $(PLATFORM_SOURCES) \
		$(LDFLAGS) $(LDLIBS) -o $(OUTPUT)
clean:
	rm -rf ./build

test: clean build
ifeq ($(NATIVE),1)
	./build/program
else
	bash -c '(cd build && python3 -mhttp.server)'
endif

