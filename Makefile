CC = g++
CFLAGS = -Wall -g -Iinclude

OBJDIR = bin/obj
BINDIR = bin

# ---------------- Main TUI Explorer ----------------
TARGET = $(BINDIR)/main

SRC = src/dir_functions.cpp \
      src/navigate.cpp \
      src/commands.cpp \
      src/main.cpp \
      src/display_file.cpp \
      src/search.cpp \
      src/file_details.cpp \
      src/utils.cpp \
      src/invertedIndex.cpp

OBJ = $(SRC:src/%.cpp=$(OBJDIR)/%.o)

# ---------------- Rules ----------------
all: $(TARGET)

$(TARGET): $(OBJ)
	@mkdir -p $(BINDIR)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

$(OBJDIR)/%.o: src/%.cpp $(DEPS)
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf bin
