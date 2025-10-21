# GCC mingw32-make makefile
CC=@gcc
RC=@windres
RM=@rm -rf

CFLAGS= -fno-exceptions -fno-rtti -Os -Wall -pedantic -I.
LDFLAGS= -s -nostdlib -mwindows -e_WinMain@16
LDLIBS= -l kernel32 -l user32 -l ole32 -l comctl32 -l comdlg32 -l shell32 -l gdi32 -l wininet -l shlwapi -l ws2_32

EXE=TwitchXP
RES=${EXE}.res
EXT=.o
OBJ=SysToolX${EXT} M3U8JSON${EXT} IniFiles${EXT} TwitchXP${EXT}

$(EXE):	$(OBJ) $(RES)

$(RES):	resource/${EXE}.rc resource/${EXE}.h
	${RC} --include-dir=resource -i resource/${EXE}.rc -O coff -o $@

wipe:
	${RM} *${EXT}
	${RM} ${RES}

all:	$(EXE)

clean:	wipe all
