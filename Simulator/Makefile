ID1 = 322719139
ID2 = 211961057

NAME = simulator_$(ID1)_$(ID2)

CXX = g++
CXXFLAGS = -Wall -std=c++17 -I../UserCommon -I../common
LDFLAGS = -ldl -pthread

SRC = $(wildcard *.cpp ../UserCommon/*.cpp)
OBJ = $(SRC:.cpp=.o)

TARGET = $(NAME)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) -o $@ $(OBJ) $(LDFLAGS)

clean:
	rm -f *.o ../UserCommon/*.o $(TARGET)
