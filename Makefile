HEADERS := $(wildcard *.h)
SOURCES := $(wildcard *.c)
MAINS := main.c test.c

TESTS := $(wildcard test/*.wod)

release: wodc.exe

debug: CFLAGS = /Zi /Wall /wd5045 /wd4820 /wd4061 /wd4668 /wd4201
debug: wodc.exe

wodc.exe: $(HEADERS) $(SOURCES)
	@ if not exist build mkdir build
	@ cl $(CFLAGS) /Fobuild\ main.c $(filter-out $(MAINS),$(SOURCES)) /link /out:$@

test: wodc.exe test.c sv.c memory.c
	@ cl test.c sv.c memory.c > nul
	@ -$(foreach test,$(TESTS),.\test.exe $(test)&)

clean:
	@ if exist build rmdir /s /q build
	@ if exist *.obj del *.obj
	@ if exist *.exe del *.exe
	@ if exist *.pdb del *.pdb
	@ if exist *.ilk del *.ilk

.PHONY: clean debug release test