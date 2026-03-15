MAKEFLAGS += " -j2 "
CFLAGS=-O3 -Wall

all: obj cnnSolver

cnnSolver: obj/cnnSolver.o obj/model.o obj/cnn.o
	g++ $(CFLAGS) obj/cnnSolver.o obj/model.o obj/cnn.o -o cnnSolver -lm

obj/cnnSolver.o: src/cnnSolver.cpp src/model.h src/cnn.h
	g++ -c $(CFLAGS) src/cnnSolver.cpp -o obj/cnnSolver.o
obj/model.o: src/model.cpp src/model.h src/cnn.h
	g++ -c $(CFLAGS) src/model.cpp -o obj/model.o
obj/cnn.o: src/cnn.cpp src/cnn.h
	g++ -c $(CFLAGS) src/cnn.cpp -o obj/cnn.o

obj:
	mkdir obj/

clean:
	rm -f cnnSolver
	rm -rf obj/

