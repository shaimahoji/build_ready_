ID1 = 322719139
ID2 = 211961057

NAME = Algorithm_$(ID1)_$(ID2)

CXX = g++
CXXFLAGS = -Wall -std=c++17 -fPIC -I../UserCommon -I../common

SRC = $(wildcard *.cpp) $(wildcard ../UserCommon/*.cpp)
OBJ = $(SRC:.cpp=.o)

TARGET = lib$(NAME).so

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) -shared -o $@ $(OBJ)

clean:
	rm -f *.o ../UserCommon/*.o $(TARGET)
