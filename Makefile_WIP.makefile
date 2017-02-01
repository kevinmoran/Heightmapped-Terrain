#Kevin's Cross-Platform Makefile
#for Single Translation Unit Builds
#i.e. assumes only source file is main.cpp

BIN = terrain 
BUILD_DIR = build/

CXX = g++
FLAGS = -Wall -pedantic 
DEBUG_FLAGS = -g -DDEBUG
RELEASE_FLAGS = -O3
FLAGS_MAC = -mmacosx-version-min=10.9 -arch x86_64 -fmessage-length=0 -UGLFW_CDECL

INCLUDE_DIRS = -I include 
INCLUDE_DIRS_MAC = -I/sw/include -I/usr/local/include
LIB_PATH = libs/osx_64/
LOC_LIB = $(LIB_PATH)libGLEW.a $(LIB_PATH)libglfw3.a
FRAMEWORKS = -framework Cocoa -framework OpenGL -framework IOKit -framework CoreVideo

SRC = main.cpp

#---------Platform Wrangling---------

#--- WINDOWS ---
ifeq ($(OS),Windows_NT)
    $(info ************ WINDOWS ************)
	BIN_EXT = .exe
else
#--- UNIX ---
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Darwin)
        #--- MAC OS ---
        $(info ************    MAC OS   ************)
        $FLAGS += $FLAGS_MAC
        $INCLUDE_DIRS += $INCLUDE_DIRS_MAC
    else
        #--- LINUX --- TODO?
        $(error ERROR: Unsupported build platform)
    endif
endif

#------------TARGETS------------

all: DEBUG

#Just to make a build directory before all builds!
#All other tasks depend on this so it always runs
#The test thing checks if it already exists first
prebuild:
	test -d $(BUILD_DIR) || mkdir $(BUILD_DIR)

DEBUG: prebuild
	$(info ************ DEBUG BUILD ************)
	${CXX} ${FLAGS} ${DEBUG_FLAGS} ${FRAMEWORKS} -o $(BUILD_DIR)${BIN}${BIN_EXT} ${SRC} ${INCLUDE_DIRS} ${LOC_LIB}

RELEASE: prebuild
	$(info ************ RELEASE BUILD ************)
	${CXX} ${FLAGS} ${RELEASE_FLAGS} ${FRAMEWORKS} -o $(BUILD_DIR)${BIN}${BIN_EXT} ${SRC} ${INCLUDE_DIRS} ${LOC_LIB}

TIMED_DEBUG: prebuild
	$(info ************ DEBUG BUILD ************)
	${CXX} ${FLAGS} ${DEBUG_FLAGS} -ftime-report ${FRAMEWORKS} -o $(BUILD_DIR)${BIN}${BIN_EXT} ${SRC} ${INCLUDE_DIRS} ${LOC_LIB}

TIMED_RELEASE: prebuild
	$(info ************ RELEASE BUILD ************)
	${CXX} ${FLAGS} ${RELEASE_FLAGS} -ftime-report ${FRAMEWORKS} -o $(BUILD_DIR)${BIN}${BIN_EXT} ${SRC} ${INCLUDE_DIRS} ${LOC_LIB}