HEADERS := $(wildcard *.h)
SOURCES := $(wildcard *.c)
MAINS := main.c test.c

release: wodc.exe

debug: CFLAGS = /Zi /Wall /wd5045 /wd4820 /wd4061 /wd4668
debug: wodc.exe

wodc.exe: $(HEADERS) $(SOURCES)
	@ if not exist build mkdir build
	@ cl $(CFLAGS) /Fobuild\ main.c $(filter-out $(MAINS),$(SOURCES)) /link /out:$@

test: test.c sv.c memory.c
	@ cl $^

clean:
	@ if exist build rmdir /s /q build
	@ if exist *.obj del *.obj
	@ if exist *.exe del *.exe

.PHONY: clean debug release