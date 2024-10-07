# vim: set tabstop=4 shiftwidth=4 expandtab noexpandtab:

CXXFLAGS = -ggdb3
CXXFLAGS += -std=c++20
CXXFLAGS += -ferror-limit=1

CXXFLAGS += -D__USE_SDL
#CXXFLAGS += -D__USE_GLFW

ifeq ($(NATIVE),1)
	CXX = clang++
	CXXFLAGS += -I/opt/homebrew/include
	LDFLAGS += -L /opt/homebrew/lib
	LDLIBS += -framework OpenGL
	LDLIBS += -lGLEW
	LDLIBS += -lglfw
	LDLIBS += -lsdl2
	OUTPUT = ./build/program
else
	CXX = em++
	LDLIBS += -s USE_ZLIB=1 -sNO_DISABLE_EXCEPTION_CATCHING=1 
	LDLIBS += -s USE_SDL=2
	LDLIBS += -s USE_GLFW=3
	LDLIBS += -s FULL_ES2=1 -s USE_WEBGL2=1 -O0
	LDLIBS += -s ALLOW_MEMORY_GROWTH=1 -s GL_UNSAFE_OPTS=0
	LDLIBS += -s ASSERTIONS=1 -s SAFE_HEAP=1
	LDLIBS += $(shell ls ./shaders | sed -e \
		's|.*|--preload-file ./shaders/\0@shaders/\0|')
	LDLIBS += $(shell ls ./models | sed -e \
		's|.*|--preload-file ./models/\0@models/\0|')
	OUTPUT = ./build/index.html
endif

build:
	mkdir -p ./build
	$(CXX) $(CXXFLAGS) ./src/example-001/main.cpp $(IMGUI_SOURCES) $(PLATFORM_SOURCES) \
		$(LDFLAGS) $(LDLIBS) -o $(OUTPUT)
clean:
	rm -rf ./build

test: clean build
ifeq ($(NATIVE),1)
	./build/program
else
	bash -c '(cd build && python3 -mhttp.server)'
endif

