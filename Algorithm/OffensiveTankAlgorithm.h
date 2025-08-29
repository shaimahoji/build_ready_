#pragma once

#include <memory>
#include <vector>
#include <queue>
#include <map>
#include <set>
#include <algorithm>
#include <functional>  // must come before TankAlgorithm.h

#include "../UserCommon/Direction.h"
#include "../UserCommon/OffensiveBattleInfo.h"

#include "../common/ActionRequest.h" // must include before TankAlgorithm.h
//#include "../common/BattleInfo.h"
//#include "../common/SatelliteView.h"
#include "../common/TankAlgorithm.h"

namespace Algorithm_322719139_211961057 {

// Type aliases for cleaner code
using Direction = UserCommon_322719139_211961057::Direction;
using OffensiveBattleInfo = UserCommon_322719139_211961057::OffensiveBattleInfo;

class TankAlgorithm_322719139_211961057_A : public TankAlgorithm {
private:
    int player_index_;
    int tank_index_;
    
    int init_shells_num_;

    // Last received battle info data (copied, not referenced)
    size_t current_x_;
    size_t current_y_;
    Direction current_direction_;
    size_t remaining_shells_;
    int cooldown_timer_;
    
    // Board information
    size_t board_width_;
    size_t board_height_;

    std::vector<std::vector<char>> board_;
    
    // Game state
    size_t current_step_;
    size_t max_steps_;
    
    int steps_until_next_info;

    // Flag to track if we need to request battle info
    bool need_battle_info_;
    size_t last_info_step_;
    size_t info_request_interval_;

    // Target coordinates (copied from battle info)
    size_t target_x_;
    size_t target_y_;
    
    // Path to target (using partial BFS pathfinding)
    std::vector<std::pair<size_t, size_t>> path_to_target_;
    size_t path_index_; // Index of current step in the path

    // Threat assessment
    bool under_threat_;
    std::vector<std::pair<size_t, size_t>> incoming_shells_;
    
    // Shooting opportunity assessment
    bool can_hit_target_;
    int shots_needed_;
    
    // Action state tracking
    int action_counter_;

    OffensiveBattleInfo last_offensive_info_;
    
public:
    TankAlgorithm_322719139_211961057_A(int player_index, int tank_index);
    ~TankAlgorithm_322719139_211961057_A() override = default;

    TankAlgorithm_322719139_211961057_A(const TankAlgorithm_322719139_211961057_A& other);

    ActionRequest getAction() override;

    int getTankIndex() const { return tank_index_; }

    void updateBattleInfo(BattleInfo& info) override;
    
private:
    void updateSpecificBattleInfo(BattleInfo& info);
    Direction directionAfterRotationAction(ActionRequest action);
    
    //bool shouldRequestBattleInfo() const;
    bool shouldRequestBattleInfo();
    ActionRequest rotateToDirection(Direction target_direction) const;
    ActionRequest moveTowardsPosition(size_t target_x, size_t target_y) const;
    bool isPositionInFront(size_t x, size_t y) const;
    Direction getDirectionToPosition(size_t x, size_t y) const;
    bool canShoot() const;
    bool isObstacle(int x, int y) const;
    std::vector<std::pair<int, int>> findPath(int target_r_x, int target_r_y) const;
    ActionRequest takeAction();

    ActionRequest determineOffensiveAction();
    ActionRequest attackTarget();
    ActionRequest followPathToTarget();
    ActionRequest handleWallInPath(size_t wall_x, size_t wall_y);
    bool isRotationAction(ActionRequest action);
    void resetPathIndex();
};

} // namespace

