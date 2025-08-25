ID1 = 322719139
ID2 = 211961057

NAME = GameManager_$(ID1)_$(ID2)

CXX = g++
CXXFLAGS = -Wall -std=c++17 -fPIC -I../UserCommon -I../common

# ✅ Only include GameManager and your own shared code
SRC = \
  GameManager.cpp \
  ../UserCommon/GameSatelliteView.cpp \
  ../UserCommon/Direction.cpp \
  ../UserCommon/OffensiveBattleInfo.cpp

# ❌ DO NOT include:
# ../Simulator/GameManagerRegistration.cpp
# ../Simulator/GameManagerRegistrar.cpp

OBJ = $(SRC:.cpp=.o)
TARGET = $(NAME).so

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) -shared -o $@ $(OBJ)

clean:
	rm -f *.o ../UserCommon/*.o $(TARGET)
