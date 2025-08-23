//-------------------------------
// common/TankAlgorithm.h
//-------------------------------
#pragma once

class TankAlgorithm {
public:
	virtual ~TankAlgorithm() {}
    virtual ActionRequest getAction() = 0;
    virtual void updateBattleInfo(BattleInfo& info) = 0;
};

using TankAlgorithmFactory =
std::function<std::unique_ptr<TankAlgorithm>(int player_index, int tank_index)>;
