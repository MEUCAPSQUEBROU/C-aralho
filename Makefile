CXX      = g++
CXXFLAGS = -std=c++17 -O2 -Wall
LIBS     = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

plane: main.cpp
	$(CXX) $(CXXFLAGS) main.cpp -o plane $(LIBS)

run: plane
	./plane

clean:
	rm -f plane
