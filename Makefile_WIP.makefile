#Kevin's Cross-Platform Makefile
#for Single Translation Unit Builds
#i.e. assumes only source file is main.cpp

BIN = terrain
BUILD_DIR = build/

CXX = g++
#General compiler flags
FLAGS = -Wall -pedantic

#Debug/Release build flags
DEBUG_FLAGS = -g -DDEBUG
RELEASE_FLAGS = -O3

#Platform-specific flags
FLAGS_WIN32 = 
FLAGS_MAC = -mmacosx-version-min=10.9 -arch x86_64 -fmessage-length=0 -UGLFW_CDECL

#Additional include directories (common/platform-specific)
INCLUDE_DIRS = -I include
INCLUDE_DIRS_WIN32 = 
INCLUDE_DIRS_MAC = -I/sw/include -I/usr/local/include

#External libs to link to
LIB_DIR_WIN32 = libs/win32/
LIBS_WIN32 = $(LIB_DIR_WIN32)libglew32.dll.a $(LIB_DIR_WIN32)libglfw3dll.a
LIB_DIR_MAC = libs/osx_64/
LIBS_MAC = $(LIB_DIR_MAC)libGLEW.a $(LIB_DIR_MAC)libglfw3.a

#System libs/Frameworks to link
WIN_SYS_LIBS = -lOpenGL32 -L ./ -lglew32 -lglfw3 -lm
FRAMEWORKS = -framework Cocoa -framework OpenGL -framework IOKit -framework CoreVideo

SRC = main.cpp

#---------Platform Wrangling---------

#--- WINDOWS ---
ifeq ($(OS),Windows_NT)
    BIN_EXT = .exe
    $FLAGS += $(FLAGS_WIN32)
    $INCLUDE_DIRS += $(INCLUDE_DIRS_WIN32)
    LIBS = $(LIBS_WIN32)
    SYS_LIBS = $(WIN_SYS_LIBS)
    PREBUILD = @if not exist "$(BUILD_DIR)" mkdir $(BUILD_DIR)
else
#--- UNIX ---
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Darwin)
		#--- MAC OS ---
        $FLAGS += $(FLAGS_MAC)
        $INCLUDE_DIRS += $(INCLUDE_DIRS_MAC)
        LIBS = $(LIBS_MAC)
        SYS_LIBS = $(FRAMEWORKS)
        PREBUILD = @mkdir -p $(BUILD_DIR)
	else
		#--- LINUX --- TODO?
		$(error ERROR: Unsupported build platform)
	endif
endif

#------------TARGETS------------
all: DEBUG

#Prebuild task: Just makes a build directory before compiling!
#All other tasks depend on this so it always runs
#The test thing checks if it already exists first
prebuild:
	$(PREBUILD)

DEBUG: prebuild
	${CXX} ${FLAGS} ${DEBUG_FLAGS} -o $(BUILD_DIR)${BIN}${BIN_EXT} ${SRC} ${INCLUDE_DIRS} ${LIBS} ${SYS_LIBS}

RELEASE: prebuild
	${CXX} ${FLAGS} ${RELEASE_FLAGS} -o $(BUILD_DIR)${BIN}${BIN_EXT} ${SRC} ${INCLUDE_DIRS} ${LIBS} ${SYS_LIBS}

TIMED_DEBUG: prebuild
	${CXX} ${FLAGS} -ftime-report ${RELEASE_FLAGS} -o $(BUILD_DIR)${BIN}${BIN_EXT} ${SRC} ${INCLUDE_DIRS} ${LIBS} ${SYS_LIBS}

TIMED_RELEASE: prebuild
	${CXX} ${FLAGS} ${RELEASE_FLAGS} -ftime-report ${FRAMEWORKS} -o $(BUILD_DIR)${BIN}${BIN_EXT} ${SRC} ${INCLUDE_DIRS} ${LOC_LIB}