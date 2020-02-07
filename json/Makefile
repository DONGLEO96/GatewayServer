CC=g++
FLAGS=-c -Wall -O2 -g 
SRC=$(wildcard ./*.cpp)

all:
	$(CC) $(FLAGS) json_reader.cpp -o ../test/json_reader.o
	$(CC) $(FLAGS) json_writer.cpp -o ../test/json_writer.o
	$(CC) $(FLAGS) json_value.cpp -o ../test/json_value.o
	
echo:
	echo $(ALL)

#clean:
#	rm -rf *.o
