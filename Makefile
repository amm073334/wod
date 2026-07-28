HEADERS := $(wildcard *.h)
SOURCES := $(wildcard *.c)
MAINS := main.c test.c

TESTS := $(wildcard test/*.wod)

WARNINGS = /Wall /wd5045 /wd4820 /wd4061 /wd4668 /wd4201

release: wodc.exe

debug: CFLAGS = /Zi
debug: wodc.exe test.exe

wodc.exe: $(HEADERS) $(SOURCES)
	@ if not exist build mkdir build
	@ cl $(CFLAGS) $(WARNINGS) /Fobuild\ main.c $(filter-out $(MAINS),$(SOURCES)) /link /out:$@

test.exe: wodc.exe test.c sv.c memory.c
	@ if not exist build mkdir build
	@ cl $(CFLAGS) $(WARNINGS) /Fobuild\ test.c sv.c memory.c /link /out:$@

test: test.exe
	@ -$(foreach test,$(TESTS),.\test.exe $(test)&)

clean:
	@ if exist build rmdir /s /q build
	@ if exist *.obj del *.obj
	@ if exist *.exe del *.exe
	@ if exist *.pdb del *.pdb
	@ if exist *.ilk del *.ilk

.PHONY: clean debug release test