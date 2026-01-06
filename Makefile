HEADERS := $(wildcard *.h)
SOURCES := $(wildcard *.c)

release: $(HEADERS) $(SOURCES) wodc

debug: CFLAGS = /Zi /Wall /wd5045 /wd4820 /wd4061 /wd4668
debug: wodc

wodc:
	mkdir build
	cl ${CFLAGS} /Fobuild\ main.c \
	memory.c sv.c environment.c error.c \
	gamedata.c commonevent.c db.c \
	lexer.c parser.c typechecker.c wl.c \
	/link /out:$@

test: test.c sv.c memory.c
	cl $^

clean:
	del wodc
	rmdir /s /q build

.PHONY: clean debug release