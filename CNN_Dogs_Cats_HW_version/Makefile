MAKEFLAGS += " -j2 "
SW_DIR = ../SW
CFLAGS = -O3 -Wall -Isrc -I$(SW_DIR)

all: obj cnnSolver

cnnSolver: obj/cnnSolver.o obj/model.o obj/cnn.o obj/CAccelProxy.o obj/CConv2DProxy.o
	g++ $(CFLAGS) obj/cnnSolver.o obj/model.o obj/cnn.o obj/CAccelProxy.o obj/CConv2DProxy.o -o cnnSolver -lm -lcma -lpthread

obj/cnnSolver.o: src/cnnSolver.cpp src/model.h src/cnn.h
	g++ -c $(CFLAGS) src/cnnSolver.cpp -o obj/cnnSolver.o

obj/model.o: src/model.cpp src/model.h src/cnn.h
	g++ -c $(CFLAGS) src/model.cpp -o obj/model.o

obj/cnn.o: src/cnn.cpp src/cnn.h
	g++ -c $(CFLAGS) src/cnn.cpp -o obj/cnn.o

obj/CAccelProxy.o: $(SW_DIR)/CAccelProxy.cpp $(SW_DIR)/CAccelProxy.hpp
	g++ -c $(CFLAGS) $(SW_DIR)/CAccelProxy.cpp -o obj/CAccelProxy.o

obj/CConv2DProxy.o: $(SW_DIR)/CConv2DProxy.cpp $(SW_DIR)/CConv2DProxy.hpp $(SW_DIR)/CAccelProxy.hpp
	g++ -c $(CFLAGS) $(SW_DIR)/CConv2DProxy.cpp -o obj/CConv2DProxy.o

obj:
	mkdir obj/

clean:
	rm -f cnnSolver
	rm -rf obj/
