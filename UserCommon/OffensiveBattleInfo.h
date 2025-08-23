#pragma once
#include "../common/BattleInfo.h"
#include "Direction.h"
#include "Tactics.h"
#include <cstddef>
#include <vector>
#include <utility>
#include <optional>

namespace UserCommon_322719139_211961057 {

    class OffensiveBattleInfo : public BattleInfo {
    private:
        int player_index_; // Player index (1 or 2)
        int tank_index_;   // Tank index

        // Tank's current state
        size_t current_x_;
        size_t current_y_;
        Direction current_direction_;

        size_t remaining_shells_;
        int cooldown_timer_;
        
        // Board information
        size_t board_width_;
        size_t board_height_;
        
        // Game state
        size_t current_step_;
        size_t max_steps_;

        bool defend_;

        std::vector<std::vector<char>> last_board_state_;

        // Mission plan (used for strategy)
        MissionPlan mission_plan_;
        size_t path_index_;

        // Target information
        size_t target_x_;
        size_t target_y_;
        Direction target_direction_;  // Concluded with the help of satellite view
        
        // Threat assessment
        bool under_threat_;
        std::vector<std::pair<size_t, size_t>> incoming_shells_;
        
        // Shooting opportunity assessment
        bool can_hit_target_; // clear direct shot (no obstacles and facing the target)

    public:
        OffensiveBattleInfo();
        ~OffensiveBattleInfo() override = default;
        OffensiveBattleInfo(const OffensiveBattleInfo& other) = default; 
            
        // Getters for tank state
        size_t getCurrentX() const { return current_x_; }
        size_t getCurrentY() const { return current_y_; }
        Direction getCurrentDirection() const { return current_direction_; }
        size_t getRemainingShells() const { return remaining_shells_; }
        int getCooldownTimer() const { return cooldown_timer_; }
        
        // Getters for board information
        size_t getBoardWidth() const { return board_width_; }
        size_t getBoardHeight() const { return board_height_; }
        std::vector<std::vector<char>> getBoardState()const { return last_board_state_;}

        // Getters for game state
        size_t getCurrentStep() const { return current_step_; }
        size_t getMaxSteps() const { return max_steps_; }

        // Getter for mission type
        MissionType getMissionType() const { return mission_plan_.getMissionType(); }
        const std::vector<std::pair<size_t, size_t>> getPath() const { return mission_plan_.getPath(); }
        const std::vector<std::optional<std::pair<size_t, size_t>>> getWallsToDestroy() const { return mission_plan_.getWallsToDestroy(); }
        
        Direction getFiringDirection() const { return mission_plan_.getFiringDirection(); }

        bool AmIDefender(){return defend_;}

        int getTankIndex() const { return tank_index_; }
        int getPlayerIndex() const { return player_index_; }

        // Getter for last board state
        std::vector<std::vector<char>> getLastBoardState() const { return last_board_state_; }

        // Setters
        void setCurrentPosition(size_t x, size_t y);
        void setCurrentDirection(Direction direction);
        void setRemainingShells(size_t shells);
        void setCooldownTimer(int timer);
        void setBoardDimensions(size_t width, size_t height);
        void setGameState(size_t current_step, size_t max_steps);
        void setBoard(std::vector<std::vector<char>> last_board_state_);
        void setDefender();
        void setMissionType(MissionType type) { mission_plan_.setMissionType(type); }
        void setLastBoardState(const std::vector<std::vector<char>>& board_state) {
            last_board_state_ = board_state;
        }
        void setPath(const std::vector<std::pair<size_t, size_t>>& new_path) { mission_plan_.setPath(new_path); }
        void setWallsToDestroy(const std::vector<std::optional<std::pair<size_t, size_t>>>& new_destroy) { mission_plan_.setWallsToDestroy(new_destroy); }
        void setTankIndex(int index) { tank_index_ = index; }
        void setPlayerIndex(int index) { player_index_ = index; }
        void resetPathIndex() { path_index_ = 0; }

        // Target getters
        size_t getTargetX() const { return target_x_; }
        size_t getTargetY() const { return target_y_; }
        Direction getTargetDirection() const { return target_direction_; }
        
        // Threat getters
        bool isUnderThreat() const { return under_threat_; }
        const std::vector<std::pair<size_t, size_t>>& getIncomingShells() const {
            return incoming_shells_;
        }
        
        // Shooting opportunity getters
        bool canHitTarget() const { return can_hit_target_; }
        
        // Target setters
        void setTarget(size_t x, size_t y, Direction direction = Direction::NONE);

        // Threat setters
        void setUnderThreat(bool under_threat);
        void addIncomingShell(size_t x, size_t y);
        void clearIncomingShells();
        
        // Shooting opportunity setters
        void setCanHitTarget(bool can_hit);
        void setShotsNeeded(int shots);
    };
} // namespace
