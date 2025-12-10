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
      src/invertedIndex.cpp \
	  src/utils2.cpp

OBJ = $(SRC:src/%.cpp=$(OBJDIR)/%.o)

# ---------------- Offline Indexer ----------------
TARGET2 = $(BINDIR)/main2

SRC2 = src/main2.cpp \
       src/invertedIndex.cpp \
	   src/utils2.cpp

OBJ2 = $(SRC2:src/%.cpp=$(OBJDIR)/%.o)

DEPS = include/myheader.h

# ---------------- Rules ----------------
all: $(TARGET) $(TARGET2)

$(TARGET): $(OBJ)
	@mkdir -p $(BINDIR)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

$(TARGET2): $(OBJ2)
	@mkdir -p $(BINDIR)
	$(CC) $(CFLAGS) -o $@ $(OBJ2)

$(OBJDIR)/%.o: src/%.cpp $(DEPS)
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf bin
