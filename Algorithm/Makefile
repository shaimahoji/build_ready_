ID1 = 322719139
ID2 = 211961057

NAME = Algorithm_$(ID1)_$(ID2)

CXX = g++
CXXFLAGS = -Wall -std=c++17 -fPIC -I../UserCommon -I../common

# Base source files
SRC = \
    $(wildcard *.cpp) \
    $(wildcard ../UserCommon/*.cpp)

# ✅ EXCLUDE the following from the Algorithm build:
# ❌ ../Simulator/PlayerRegistration.cpp
# ❌ ../Simulator/TankAlgorithmRegistration.cpp
# ❌ ../Simulator/AlgorithmRegistrar.cpp

# You still need to include the registration *headers* in your .cpp files,
# like: #include "../common/PlayerRegistration.h"

OBJ = $(SRC:.cpp=.o)

TARGET = $(NAME).so

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) -shared -o $@ $(OBJ)

clean:
	rm -f *.o ../UserCommon/*.o $(TARGET)