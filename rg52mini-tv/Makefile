CC = aarch64-linux-gnu-gcc
CFLAGS = -O2 -Wall -Wextra -std=c11 -D_GNU_SOURCE -D_REENTRANT
INCLUDES = -I/usr/include/SDL2 -Iinclude
LIBDIRS = -Llibs
LIBS = -lmpv -lSDL2 -lSDL2_ttf -lpthread -lm

SRCDIR = src
OBJDIR = obj
BINDIR = .

SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SOURCES))
TARGET = $(BINDIR)/rg52mini-tv

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(LIBDIRS) -o $@ $^ $(LIBS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -rf $(OBJDIR) $(TARGET)
