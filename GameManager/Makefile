ID1 = 322719139
ID2 = 211961057

NAME = GameManager_$(ID1)_$(ID2)

CXX = g++
CXXFLAGS = -Wall -std=c++17 -fPIC -I../UserCommon -I../common -I.. #must delete last one and figure out what to do with the visualizer

SRC = $(wildcard *.cpp ../UserCommon/*.cpp)
OBJ = $(SRC:.cpp=.o)

TARGET = lib$(NAME).so

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) -shared -o $@ $(OBJ)

clean:
	rm -f *.o ../UserCommon/*.o $(TARGET)
