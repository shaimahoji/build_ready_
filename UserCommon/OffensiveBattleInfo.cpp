#include "OffensiveBattleInfo.h"

namespace UserCommon_322719139_211961057 {
    OffensiveBattleInfo::OffensiveBattleInfo()
        : current_x_(0),
        current_y_(0),
        current_direction_(Direction::NONE),
        remaining_shells_(0),
        cooldown_timer_(0),
        board_width_(100),
        board_height_(100),
        current_step_(0),
        max_steps_(5000),
        defend_(false),
        target_x_(0),
        target_y_(0),
        target_direction_(Direction::NONE),
        under_threat_(false),
        can_hit_target_(false) {
    }

    void OffensiveBattleInfo::setCurrentPosition(size_t x, size_t y) {
        current_x_ = x;
        current_y_ = y;
    }

    void OffensiveBattleInfo::setCurrentDirection(Direction direction) {
        current_direction_ = direction;
    }

    void OffensiveBattleInfo::setRemainingShells(size_t shells) {
        remaining_shells_ = shells;
    }

    void OffensiveBattleInfo::setCooldownTimer(int timer) {
        cooldown_timer_ = timer;
    }

    void OffensiveBattleInfo::setBoardDimensions(size_t width, size_t height) {
        board_width_ = width;
        board_height_ = height;
    }

    void OffensiveBattleInfo::setGameState(size_t current_step, size_t max_steps) {
        current_step_ = current_step;
        max_steps_ = max_steps;
    }

    void OffensiveBattleInfo::setBoard(std::vector<std::vector<char>> board){
        last_board_state_ = board;
    }

    void OffensiveBattleInfo::setDefender(){defend_ = true ;}

    void OffensiveBattleInfo::setTarget(size_t x, size_t y, Direction direction) {
        target_x_ = x;
        target_y_ = y;
        target_direction_ = direction;
    }

    void OffensiveBattleInfo::setUnderThreat(bool under_threat) {
        under_threat_ = under_threat;
    }

    void OffensiveBattleInfo::addIncomingShell(size_t x, size_t y) {
        incoming_shells_.push_back({x, y});
    }

    void OffensiveBattleInfo::clearIncomingShells() {
        incoming_shells_.clear();
    }

    void OffensiveBattleInfo::setCanHitTarget(bool can_hit) {
        can_hit_target_ = can_hit;
    }

    void OffensiveBattleInfo::setShotsNeeded(int shots) {
        // This function was in OffensiveBattleInfo.h but not implemented in .cpp
        // Adding a placeholder implementation or removing if not needed.
        // For now, it's not directly used in the provided code, so leaving it empty.
    }
} // namespace

