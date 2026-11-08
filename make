CC ?= gcc

CPPFLAGS := -Iinclude
WARNINGS := -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wvla -Werror=vla
CSTD := -std=c23

SRC := $(wildcard src/*.c)
OBJ := $(SRC:.c=.o)
TARGET := csvstat

.PHONY: all debug sanitize release clean

all: debug

debug: CFLAGS := $(CSTD) $(WARNINGS) -O0 -g3
debug: $(TARGET)

sanitize: CFLAGS := $(CSTD) $(WARNINGS) -O1 -g3 \
	-fsanitize=address,undefined -fno-omit-frame-pointer
sanitize: LDFLAGS := -fsanitize=address,undefined
sanitize: clean $(TARGET)

release: CFLAGS := $(CSTD) $(WARNINGS) -O2 -DNDEBUG
release: clean $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) -o $@

src/%.o: src/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)