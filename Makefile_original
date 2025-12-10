CC = g++
CFLAGS = -Wall -g -Iinclude

TARGET = bin/main
OBJDIR = bin/obj

SRC = src/dir_functions.cpp \
      src/navigate.cpp \
      src/commands.cpp \
      src/main.cpp \
      src/display_file.cpp \
      src/search.cpp \
      src/file_details.cpp \
      src/utils.cpp

OBJ = $(SRC:src/%.cpp=$(OBJDIR)/%.o)

DEPS = include/myheader.h

all: $(TARGET)

$(TARGET): $(OBJ)
	@mkdir -p bin
	$(CC) $(CFLAGS) -o $@ $(OBJ)

$(OBJDIR)/%.o: src/%.cpp $(DEPS)
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf bin
