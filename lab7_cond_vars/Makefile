# https://stackoverflow.com/a/18258352/13830772
rwildcard = $(filter-out \ ,$(foreach pattern,$(2),$(wildcard $(1)/$(pattern)))$(foreach child,$(wildcard $(1)/*),$(call rwildcard,$(child),$(2))))
# https://stackoverflow.com/a/7324640/13830772
eq = $(and $(findstring $(1),$(2)),$(findstring $(2),$(1)))

TARGET   ?= main
BUILD_DIR = build
DATA_FILE = $(BUILD_DIR)/data.bin

DEBUG_SUFFIX = $(if $(call eq,$(MODE),debug),-debug)
CFLAGS = -W -Wall -Wextra -Werror -pedantic -Wno-strict-prototypes -std=c11 -Isrc/main/ $(if $(call eq,$(MODE),debug),-ggdb) -D_DEFAULT_SOURCE -D_GNU_SOURCE -lm
CC = gcc $(CFLAGS)
VALGRIND = valgrind --leak-check=full --show-leak-kinds=all
GDB = gdb
GDB_COMMANDS = $(BUILD_DIR)/gdb-commands.txt

SOURCES = $(call rwildcard,src/main,*.c)
OBJECTS = $(foreach source,$(SOURCES),$(call object,$(source)))
MAINS = $(wildcard src/cmd/*.c)
EXECUTABLES = $(foreach main,$(MAINS),$(call executable,$(main)))
COMPILE_COMMANDS = $(BUILD_DIR)/compile_commands.json

target = $(BUILD_DIR)/$(1)$(DEBUG_SUFFIX)
executable = $(BUILD_DIR)/$(patsubst %.c,%,$(notdir $(1)))$(DEBUG_SUFFIX)

.PHONY: all
all: app

.PHONE: run
run: $(call target,$(TARGET))
	$(call target,$(TARGET)) $(TARGET_ARGS)

.PHONE: vrun
vrun: $(EXECUTABLES)
	$(VALGRIND) $(call target,$(TARGET))

.PHONE: gdb
gdb: $(EXECUTABLES) $(GDB_COMMANDS)
	$(GDB) -x $(GDB_COMMANDS) -q $(call target,$(TARGET))

.PHONE: bear
bear:
	make clean
	mkdir -p $(dir $(COMPILE_COMMANDS))
	bear --output $(COMPILE_COMMANDS) -- make app

.PHONY: app
app: $(EXECUTABLES)

$(EXECUTABLES): $(call executable,%): src/cmd/%.c $(SOURCES)
	mkdir -p $(dir $@)
	$(CC) $< $(SOURCES) -o $@

$(GDB_COMMANDS):
	mkdir -p $(dir $@)
	echo run finish > $@
	echo bt 10 >> $@

.PHONY: all
clean:
	rm -rf $(BUILD_DIR)
