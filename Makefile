CC = g++
CFLAGS = -Wall -g
DEPS = myheader.h 
OBJ = dir_functions.o navigate.o commands.o main.o display_file.o search.o file_details.o
%.o: %.cpp $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

main: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^
	rm -rf *o

clean:
	rm -rf *o main