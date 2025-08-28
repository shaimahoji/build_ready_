ID1 = 322719139
ID2 = 211961057

NAME = simulator_$(ID1)_$(ID2)

CXX = g++
CXXFLAGS = -Wall -std=c++17 -I../UserCommon -I../common
LDFLAGS = -ldl -pthread -rdynamic

# Include all .cpp files in Simulator and UserCommon only once
SRC = \
    $(wildcard *.cpp) \
    $(wildcard ../UserCommon/*.cpp)

# ❌ DO NOT manually add these again — they are already included by wildcard
# SRC += \
#     ../Simulator/AlgorithmRegistrar.cpp \
#     ../Simulator/PlayerRegistration.cpp \
#     ../Simulator/TankAlgorithmRegistration.cpp

OBJ = $(SRC:.cpp=.o)

TARGET = $(NAME)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) -o $(TARGET) $(OBJ) $(LDFLAGS)

clean:
	rm -f *.o ../UserCommon/*.o $(TARGET)
