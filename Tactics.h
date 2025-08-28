#pragma once

#include "Direction.h"
#include <optional>

namespace UserCommon_322719139_211961057 {
    enum class MissionType { ATTACK, RETREAT, HOLD };

    /* Pathfinding related structures */
    struct PathResult {
        std::vector<std::pair<size_t, size_t>> path;      // Movement steps, 5 max
        std::vector<std::optional<std::pair<size_t, size_t>>> destroy;   // Walls to shoot on the path
    };

    struct MissionPlan {
        MissionType mission_type;
        size_t position_x_;
        size_t position_y_;
        
        bool direct_fire_possible;
        Direction firing_direction;
        int estimated_steps;

        PathResult path_and_walls;

        MissionPlan()
            : mission_type(MissionType::ATTACK),
            position_x_(-1),
            position_y_(-1),
            direct_fire_possible(false),
            firing_direction(Direction::NONE),
            estimated_steps(0) {}

        // Mission type
        MissionType getMissionType() const { return mission_type; }
        void setMissionType(MissionType type) { mission_type = type; }

        // Position
        size_t getPositionX() const { return position_x_; }
        void setPositionX(size_t x) { position_x_ = x; }
        size_t getPositionY() const { return position_y_; }
        void setPositionY(size_t y) { position_y_ = y; }

        // Firing logic
        bool isDirectFirePossible() const { return direct_fire_possible; }
        void setDirectFirePossible(bool possible) { direct_fire_possible = possible; }

        Direction getFiringDirection() const { return firing_direction; }
        
        void setFiringDirection(Direction dir) { firing_direction = dir; }

        // Path info
        const std::vector<std::pair<size_t, size_t>>& getPath() const { return path_and_walls.path; }
        void setPath(const std::vector<std::pair<size_t, size_t>>& new_path) { 
            path_and_walls.path = new_path; 
        }

        const std::vector<std::optional<std::pair<size_t, size_t>>>& getWallsToDestroy() const { return path_and_walls.destroy; }
        void setWallsToDestroy(const std::vector<std::optional<std::pair<size_t, size_t>>>& new_destroy) { 
            path_and_walls.destroy = new_destroy; 
        }

        int getEstimatedSteps() const { return estimated_steps; }
        void setEstimatedSteps(int steps) { estimated_steps = steps; }
    };
} // namespace