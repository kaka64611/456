# RG52MINI Music Player Makefile
# Target: AArch64 (RG52MINI / RK3562 / EmuELEC 4.7)

CC ?= gcc
CFLAGS = -O2 -Wall -Wextra -std=c11 -D_GNU_SOURCE
LDFLAGS = -lm

# SDL2 libraries
SDL_CFLAGS = $(shell pkg-config --cflags sdl2 SDL2_ttf SDL2_image SDL2_mixer 2>/dev/null)
SDL_LIBS = $(shell pkg-config --libs sdl2 SDL2_ttf SDL2_image SDL2_mixer 2>/dev/null)

# Fallback if pkg-config not available
ifeq ($(SDL_CFLAGS),)
SDL_CFLAGS = -I/usr/include/SDL2 -I/usr/local/include/SDL2
endif
ifeq ($(SDL_LIBS),)
SDL_LIBS = -lSDL2 -lSDL2_ttf -lSDL2_image -lSDL2_mixer
endif

CFLAGS += $(SDL_CFLAGS)
LDFLAGS += $(SDL_LIBS)

SRCDIR = src
OBJDIR = obj
BINDIR = bin

SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SOURCES))
TARGET = $(BINDIR)/rg52mini-music

.PHONY: all clean install

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@mkdir -p $(BINDIR)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)
	@echo "Built: $@"

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR) $(BINDIR)

install: $(TARGET)
	@echo "Installing..."
	@cp $(TARGET) /usr/local/bin/
	@echo "Done"
