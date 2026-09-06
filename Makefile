SOURCE_DIR := src
TEST_DIR := test
BUILD_DIR := build

HEADERS := $(wildcard $(SOURCE_DIR)/*.h)
SOURCES := $(wildcard $(SOURCE_DIR)/*.c)
MAINS := $(SOURCE_DIR)/main.c $(SOURCE_DIR)/test.c $(SOURCE_DIR)/dbextract.c
INCLUDES := $(filter-out $(MAINS),$(SOURCES))
OBJECTS := $(addprefix $(BUILD_DIR)/,$(notdir $(INCLUDES:.c=.obj)))

TESTS := $(wildcard $(TEST_DIR)/*.wod)

WARNINGS = /Wall /wd5045 /wd4820 /wd4061 /wd4668 /wd4201

release: wodc.exe

debug: CFLAGS = /Zi
debug: wodc.exe test.exe

wodc.exe: $(HEADERS) $(SOURCES)
	@ if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)
	@ cl $(CFLAGS) $(WARNINGS) /Fo$(BUILD_DIR)\ \
		$(SOURCE_DIR)/main.c $(filter-out $(MAINS),$(SOURCES)) /link /out:$@

test.exe: wodc.exe
	@ if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)
	@ cl $(CFLAGS) $(WARNINGS) /Fo$(BUILD_DIR)\ $(SOURCE_DIR)/test.c $(OBJECTS) /link /out:$@

test: test.exe
	@ .\test.exe -f test

dbextract.exe: $(SOURCE_DIR)/dbextract.c
	@ if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)
	@ cl $(CFLAGS) /Zi $(WARNINGS) /Fo$(BUILD_DIR)\ $(SOURCE_DIR)/dbextract.c $(SOURCE_DIR)/source.c $(SOURCE_DIR)/memory.c $(SOURCE_DIR)/sv.c /link /out:$@

clean:
	@ if exist build rmdir /s /q build
	@ if exist *.obj del *.obj
	@ if exist *.exe del *.exe
	@ if exist *.pdb del *.pdb
	@ if exist *.ilk del *.ilk

.PHONY: clean debug release test