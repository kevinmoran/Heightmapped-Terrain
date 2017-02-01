BIN = terrain.exe
CC = g++
FLAGS = -Wall -pedantic -mmacosx-version-min=10.9 -arch x86_64 -fmessage-length=0 -UGLFW_CDECL
DFLAGS = -g -DDEBUG
RFLAGS = -O3
INC = -I include -I/sw/include -I/usr/local/include
LIB_PATH = libs/osx_64/
LOC_LIB = $(LIB_PATH)libGLEW.a $(LIB_PATH)libglfw3.a
FRAMEWORKS = -framework Cocoa -framework OpenGL -framework IOKit -framework CoreVideo
SRC = main.cpp

all: DEBUG

DEBUG: 
	${CC} ${FLAGS} ${DFLAGS} ${FRAMEWORKS} -o ${BIN} ${SRC} ${INC} ${LOC_LIB}

RELEASE: 
	${CC} ${FLAGS} ${RFLAGS} ${FRAMEWORKS} -o ${BIN} ${SRC} ${INC} ${LOC_LIB}

TIMED_DEBUG:
	${CC} ${FLAGS} ${DFLAGS} -ftime-report ${FRAMEWORKS} -o ${BIN} ${SRC} ${INC} ${LOC_LIB}

TIMED_RELEASE:
	${CC} ${FLAGS} ${RFLAGS} -ftime-report ${FRAMEWORKS} -o ${BIN} ${SRC} ${INC} ${LOC_LIB}