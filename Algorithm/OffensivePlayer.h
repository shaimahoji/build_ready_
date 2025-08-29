#pragma once

#include <functional>  // for std::function
#include <memory>      // for std::unique_ptr
#include <cstddef>     // for size_t

#include <vector>
#include <cmath>
#include <algorithm>
#include <queue>
#include <unordered_set>
#include <string>
#include <unordered_map>
#include <optional>
#include <climits>

#include "../UserCommon/Direction.h"
#include "../UserCommon/Tactics.h"
#include "../UserCommon/OffensiveBattleInfo.h"

// Important: Player.h uses TankAlgorithm and SatelliteView in its method signatures,
// but does NOT include them itself.
// Therefore, they MUST be included before including Player.h.
#include "../common/SatelliteView.h"
#include "../common/TankAlgorithm.h"
#include "../common/Player.h"

namespace Algorithm_322719139_211961057 {

// Type aliases for cleaner code
using Direction = UserCommon_322719139_211961057::Direction;
using MissionPlan = UserCommon_322719139_211961057::MissionPlan;
using PathResult  = UserCommon_322719139_211961057::PathResult;

class Player_322719139_211961057_A : public Player {
private:
    // Player identification
    int player_index_;
    size_t max_steps_;
    size_t num_shells_;
    
    // Board information
    size_t board_width_;
    size_t board_height_;

    std::vector<std::vector<char>> last_board_state_;
    
    // Game step tracking
    size_t current_step_;
    
    // Tank tracking
    struct TankInfo {
        size_t x;
        size_t y;
        Direction direction;
        size_t tank_index;  // Unique index for each tank
        size_t remaining_shells;
        int cooldown_timer;
        bool is_alive;
        size_t last_info_step;  // Last step when this tank got info
        
        TankInfo(size_t x, size_t y, Direction direction, size_t shells)
            : x(x), y(y), direction(direction), remaining_shells(shells),
              cooldown_timer(0), is_alive(true), last_info_step(0) {}
        
        TankInfo(size_t x, size_t y, Direction direction, size_t shells,
           size_t cooldown_timer, bool is_alive, size_t last_info_step)
           : x(x), y(y), direction(direction), remaining_shells(shells),
           cooldown_timer(cooldown_timer), is_alive(is_alive), last_info_step(last_info_step) {}
    };
    
    // My tanks (indexed by tank_index)
    std::vector<TankInfo> my_tanks_;
    
    // Enemy tanks
    std::vector<TankInfo> enemy_tanks_;
    
    // Shell tracking
    struct ShellInfo {
        size_t x;
        size_t y;
        Direction direction;
        size_t first_seen_step;
        bool is_enemy;  // Is this an enemy shell?
        
        ShellInfo(size_t x, size_t y, Direction direction, size_t step, bool enemy)
            : x(x), y(y), direction(direction), first_seen_step(step), is_enemy(enemy) {}
    };
    
    std::vector<ShellInfo> observed_shells_;
    
    // Mine tracking
    std::vector<std::pair<size_t, size_t>> known_mines_;
    
    // Wall tracking
    struct WallInfo {
        size_t x;
        size_t y;
        int estimated_health;  // Estimated health (2 = new, 1 = damaged, 0 = destroyed)
        size_t last_seen_step;  // Last step when this wall was seen
        
        WallInfo(size_t x, size_t y, int health, size_t step)
            : x(x), y(y), estimated_health(health), last_seen_step(step) {}
        
        bool operator==(const WallInfo& other) const {
            return x == other.x && y == other.y;
        }
    };
    
    std::vector<WallInfo> known_walls_;

    // Offensive strategy parameters
    double aggression_level_;  // 0.0-1.0, higher means more aggressive
    
    // Target priority queue (for multiple enemies)
    struct TargetPriority {
        size_t tank_index;  // Index in enemy_tanks_
        double priority;    // Higher means higher priority
        
        bool operator<(const TargetPriority& other) const {
            return priority < other.priority;
        }
    };

    struct Node {
        size_t x, y;
        int cost; // total cost (steps including shooting cooldown)
        // For priority queue:
        bool operator<(const Node& other) const { return cost > other.cost; }
    };
        
    std::vector<TargetPriority> target_priorities_;

    // game-step plans for each tank
    std::vector<MissionPlan> mission_plans_per_tanks;
    std::unordered_map<size_t, std::vector<std::pair<size_t, size_t>>> paths_per_missions;

public:
    Player_322719139_211961057_A(int player_index, size_t board_width, size_t board_height, size_t max_steps, size_t num_shells);
    ~Player_322719139_211961057_A() override = default;

    Player_322719139_211961057_A(const Player_322719139_211961057_A& other);
    
    void updateTankWithBattleInfo(TankAlgorithm& tank, SatelliteView& satellite_view) override;

private:
    // Helper method to update tank and enemy information from satellite view
    void updateBoardInfo(SatelliteView& satellite_view, int tank_index);
    
    
    // Helper method to update direction
    Direction estimateDirection(size_t prev_x, size_t prev_y, 
                                           size_t new_x, size_t new_y,Direction prev_dir) ;
    // Helper method to find a tank's index
    //int findTankIndex(TankAlgorithm& tank) const;
    
    // Path finding utilities
    bool isValidPosition(size_t x, size_t y) const;
    bool isObstacle(size_t x, size_t y) const;
    std::vector<std::pair<size_t, size_t>> findPath(size_t start_x, size_t start_y, 
                                                   size_t goal_x, size_t goal_y) const;
    
    // Distance calculation
    double calculateDistance(size_t x1, size_t y1, size_t x2, size_t y2) const;
    
    // Direction utilities
    Direction calculateDirection(size_t from_x, size_t from_y, size_t to_x, size_t to_y) const;
    std::vector<Direction> getRotationSequence(Direction current_dir, Direction target_dir) const;
    
    // Shell prediction
    std::vector<std::pair<size_t, size_t>> predictShellPath(size_t x, size_t y, 
                                                          Direction direction, int steps) const;
    
    // Danger assessment
    //bool isTankInDanger(const TankInfo& tank, int& danger_level, 
    //                   std::vector<std::pair<size_t, size_t>>& danger_sources) const;    
    
    bool hasLineOfSightLimited(size_t from_x, size_t from_y, size_t to_x, size_t to_y) const;
    int manhattanDistance(int x1, int y1, int x2, int y2) const;
    bool canReachWithinTwoSteps(int from_x, int from_y, int to_x, int to_y) const;
    
    // Target acquisition
    std::pair<size_t, size_t> findClosestEnemyTank(size_t from_x, size_t from_y) const;
    
    // Line of sight calculation
    bool hasLineOfSight(size_t from_x, size_t from_y, size_t to_x, size_t to_y) const;
    
    // Update tank position
    void updateMyTankPosition(size_t x, size_t y, int excluding_tank_index);
    void updateEnemyTankPosition(size_t x, size_t y);
    
    // Update object positions
    void updateWallPosition(size_t x, size_t y,bool damaged);
    void updateMinePosition(size_t x, size_t y);
    int getTankIndexFromSatellite(const SatelliteView& satellite_view);

    void updateTargetPriorities();
    
    MissionPlan createMissionPlan(size_t my_tank_index, size_t enemy_tank_index);
    void createMissionsForTanks();
    
    bool canHitTarget(size_t from_x, size_t from_y, Direction direction, 
                     size_t target_x, size_t target_y) const;
    
    // New methods for improved targeting
    double scoreTargetForTank(const TankInfo& my_tank, const TankInfo& enemy_tank, const TargetPriority* priority_ptr);
    int computeTurnsToAlign(Direction current_dir, size_t from_x, size_t from_y, size_t to_x, size_t to_y) const;

    bool isTankUnderThreat(const TankInfo& tank) const;
    int  toroidalDistance(int a, int b, int size) const;
    std::optional<std::pair<int, int>> findSafeRetreatPosition(const TankInfo& tank) const;

    //board related helper methods
    bool isTileUnderThreat(int x, int y) const;
    bool hasObstacleBetween(int x1, int y1, int x2, int y2, bool consider_mines) const;

    /* Pathfinding related methods */
    int getShootingCost(char cell);
    PathResult findPath(size_t start_x, size_t start_y, size_t goal_x, size_t goal_y, 
        const std::vector<std::vector<char>>& board_state, size_t path_max_length = 5);
    PathResult findPathOLD2(
    size_t start_x, size_t start_y,
    size_t goal_x, size_t goal_y,
    const std::vector<std::vector<char>>& board_state,
    size_t path_max_length);

    bool shouldAttemptHit(size_t from_x, size_t from_y, Direction direction, size_t target_x, size_t target_y) const;
};

} // namespace
