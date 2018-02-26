# TwitchXP GCC makefile
CC=gcc
LD=ld
EXEC_FILE=TwitchXP.exe
LIB_PATH?=.
CFLAGS= -fwritable-strings -mno-stack-arg-probe -Os -Wall -pedantic -mwindows -I.
LDFLAGS= --strip-all --subsystem windows -L $(LIB_PATH) -l kernel32 -l user32 -l ole32 -l comctl32 -l comdlg32 -l shell32 -l gdi32 -l advapi32 -l wininet -l shlwapi -nostdlib --exclude-libs msvcrt.a -e_WinMain@16

OBJ_EXT=.o
OBJS=SysToolX${OBJ_EXT} M3U8JSON${OBJ_EXT} IniFiles${OBJ_EXT} TwitchXP${OBJ_EXT}

all: $(EXEC_FILE)

$(EXEC_FILE): $(OBJS)
	${LD} ${OBJS} resource/TwitchXP.res -o $@ ${LDFLAGS}


o=${OBJ_EXT}

wipe:
	@rm -rf *${OBJ_EXT}

clean:	wipe all
