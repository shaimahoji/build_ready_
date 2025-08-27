#include <cmath>
#include <unordered_map>
#include <iostream>

#include "OffensiveTankAlgorithm.h"

#include "../common/TankAlgorithmRegistration.h"

using namespace UserCommon_322719139_211961057;

namespace Algorithm_322719139_211961057 {

TankAlgorithm_322719139_211961057_A::TankAlgorithm_322719139_211961057_A(int player_index, int tank_index)
    : player_index_(player_index),
      tank_index_(tank_index),
      init_shells_num_(0),
      current_x_(0),
      current_y_(0),
      current_direction_((player_index == 1) ? Direction::LEFT : Direction::RIGHT),
      remaining_shells_(0),
      cooldown_timer_(0),
      board_width_(100),
      board_height_(100),
      board_(),
      current_step_(0),
      max_steps_(5000),
      steps_until_next_info(0),
      need_battle_info_(true),
      last_info_step_(0),
      info_request_interval_(player_index == 1 ? 5 : 3),
      target_x_(0),
      target_y_(0),
      path_index_(0),
      under_threat_(false),
      can_hit_target_(false),
      shots_needed_(0),
      action_counter_(0) {
}

TankAlgorithm_322719139_211961057_A::TankAlgorithm_322719139_211961057_A(const TankAlgorithm_322719139_211961057_A& other)
    : TankAlgorithm(other),
      player_index_(other.player_index_),
      tank_index_(other.tank_index_),
      init_shells_num_(other.init_shells_num_),
      current_x_(other.current_x_),
      current_y_(other.current_y_),
      current_direction_(other.current_direction_),
      remaining_shells_(other.remaining_shells_),
      cooldown_timer_(other.cooldown_timer_),
      board_width_(other.board_width_),
      board_height_(other.board_height_),
      board_(other.board_),
      current_step_(other.current_step_),
      max_steps_(other.max_steps_),
      steps_until_next_info(other.steps_until_next_info),
      need_battle_info_(other.need_battle_info_),
      last_info_step_(other.last_info_step_),
      info_request_interval_(other.info_request_interval_),
      target_x_(other.target_x_),
      target_y_(other.target_y_),
      path_to_target_(other.path_to_target_),
      path_index_(other.path_index_),
      under_threat_(other.under_threat_),
      incoming_shells_(other.incoming_shells_),
      can_hit_target_(other.can_hit_target_),
      shots_needed_(other.shots_needed_),
      action_counter_(other.action_counter_),
      last_offensive_info_(other.last_offensive_info_) {
}

void TankAlgorithm_322719139_211961057_A::updateBattleInfo(BattleInfo& info) {
    need_battle_info_ = false;
    last_info_step_ = current_step_;
    
    //MyBattleInfo& my_info = static_cast<MyBattleInfo&>(info);
    OffensiveBattleInfo& my_info = static_cast<OffensiveBattleInfo&>(info);

    current_x_ = my_info.getCurrentX();
    current_y_ = my_info.getCurrentY();
    current_direction_ = my_info.getCurrentDirection();

    remaining_shells_ = my_info.getRemainingShells();
    cooldown_timer_ = my_info.getCooldownTimer();
    
    board_width_ = my_info.getBoardWidth();
    board_height_ = my_info.getBoardHeight();

    board_ = my_info.getBoardState();
    
    current_step_ = my_info.getCurrentStep();
    max_steps_ = my_info.getMaxSteps();
    
    updateSpecificBattleInfo(info);
}

ActionRequest TankAlgorithm_322719139_211961057_A::getAction() {
    current_step_++;
    steps_until_next_info++;

    std::cout << "[DEBUG] Step: " << current_step_
              << ", Steps until next info: " << steps_until_next_info
              << ", Shells: " << remaining_shells_
              << ", Cooldown: " << cooldown_timer_ << "\n";

    if (shouldRequestBattleInfo()) {
        std::cout << "[DEBUG] Requesting battle info (need_battle_info_=" 
                  << (need_battle_info_ ? "true" : "false") << ")\n";
        need_battle_info_ = false;
        return ActionRequest::GetBattleInfo;
    }

    action_counter_++;
    std::cout << "[DEBUG] Action counter: " << action_counter_ << "\n";

    ActionRequest action = determineOffensiveAction();
    std::cout << "[DEBUG] Determined action: " << static_cast<int>(action) << "\n";
    return action;
}

void TankAlgorithm_322719139_211961057_A::updateSpecificBattleInfo(BattleInfo& info) {
    std::cout << "\n---------- updateSpecificBattleInfo(BattleInfo& info) called ----------\n";
    
    std::cout << "[DEBUG] typeid(info).name() = " << typeid(info).name() << "\n";

    try {
        last_offensive_info_ = dynamic_cast<OffensiveBattleInfo&>(info);
    } catch (const std::bad_cast& e) {
        std::cerr << "[ERROR] Failed dynamic_cast to OffensiveBattleInfo: " << e.what() << "\n";
        return;
    }

    OffensiveBattleInfo& offensive_info = last_offensive_info_;

    std::cout << "[DEBUG] MissionType = " << static_cast<int>(offensive_info.getMissionType()) << "\n";
    std::cout << "[DEBUG] Target: (" << offensive_info.getTargetX() << ", " << offensive_info.getTargetY() << ")\n";
    std::cout << "[DEBUG] Target Direction = " << static_cast<int>(offensive_info.getTargetDirection()) << "\n";
    std::cout << "[DEBUG] Under threat? " << offensive_info.isUnderThreat() << "\n";
    std::cout << "[DEBUG] Incoming shells count = " << offensive_info.getIncomingShells().size() << "\n";
    std::cout << "[DEBUG] Can hit target? " << offensive_info.canHitTarget() << "\n";
    std::cout << "[DEBUG] Path size = " << offensive_info.getPath().size() << "\n";
    std::cout << "[DEBUG] WallsToDestroy size = " << offensive_info.getWallsToDestroy().size() << "\n";

    under_threat_ = offensive_info.isUnderThreat();
    incoming_shells_ = offensive_info.getIncomingShells();
    target_x_ = offensive_info.getTargetX();
    target_y_ = offensive_info.getTargetY();
    can_hit_target_ = offensive_info.canHitTarget();
    path_to_target_ = offensive_info.getPath();
    path_index_ = 0;

    std::cout << "[DEBUG] Updating internal fields...\n";
    std::cout << "[DEBUG] can_hit_target_ = " << can_hit_target_ << "\n";
    std::cout << "[DEBUG] path_to_target_ size = " << path_to_target_.size() << "\n";

    switch (offensive_info.getMissionType()) {
        case MissionType::ATTACK:
            action_counter_ = can_hit_target_ ? 1 : path_to_target_.size();
            std::cout << "[DEBUG] Mission = ATTACK. action_counter_ = " << action_counter_ << "\n";
            break;

        case MissionType::RETREAT:
            action_counter_ = (!path_to_target_.empty() && 
                               offensive_info.getWallsToDestroy().empty())
                                ? path_to_target_.size()
                                : 0;
            std::cout << "[DEBUG] Mission = RETREAT. action_counter_ = " << action_counter_ << "\n";
            break;

        case MissionType::HOLD:
        default:
            action_counter_ = 0;
            target_x_ = current_x_;
            target_y_ = current_y_;
            std::cout << "[DEBUG] Mission = HOLD/UNKNOWN. action_counter_ = 0\n";
            break;
    }

    std::cout << "[DEBUG] Finished updateSpecificBattleInfo.\n\n";
}

Direction TankAlgorithm_322719139_211961057_A::directionAfterRotationAction(ActionRequest action) {

    switch (action) {
        case ActionRequest::RotateLeft45:
            current_direction_ = DirectionUtil::rotateLeft(current_direction_);
            break;
        case ActionRequest::RotateRight45:
            current_direction_ = DirectionUtil::rotateRight(current_direction_);
            break;
        case ActionRequest::RotateLeft90:
            current_direction_ = DirectionUtil::rotateLeftQuarter(current_direction_);
            break;
        case ActionRequest::RotateRight90:
            current_direction_ = DirectionUtil::rotateRightQuarter(current_direction_);
            break;
        default:
            break;
    }

    return current_direction_;
}

bool TankAlgorithm_322719139_211961057_A::shouldRequestBattleInfo() const {
    if (need_battle_info_) {
        return true;
    }

    if (current_step_ - last_info_step_ >= info_request_interval_) {
        return true;
    }

    if (max_steps_ > 0 && current_step_ >= max_steps_ * 0.9) {
        bool should_request = (current_step_ - last_info_step_ >= 2);
        return should_request;
    }

    return false;
}

ActionRequest TankAlgorithm_322719139_211961057_A::rotateToDirection(Direction target_direction) const {
    if (target_direction == current_direction_) {
        std::cout << "[DEBUG] No rotation needed\n";
        return ActionRequest::DoNothing;
    }

    int current_idx = static_cast<int>(current_direction_);
    int target_idx = static_cast<int>(target_direction);

    int diff = (target_idx - current_idx + 8) % 8;

    switch (diff) {
        case 1: return ActionRequest::RotateRight45;
        case 2: return ActionRequest::RotateRight90;
        case 3: return ActionRequest::RotateRight90;
        case 4: return ActionRequest::RotateRight90;
        case 5: return ActionRequest::RotateLeft90;
        case 6: return ActionRequest::RotateLeft90;
        case 7: return ActionRequest::RotateLeft45;
        default: return ActionRequest::DoNothing;
    }
}

ActionRequest TankAlgorithm_322719139_211961057_A::moveTowardsPosition(size_t target_x, size_t target_y) const {
    std::cout<<target_x<<"," <<target_y<<std::endl;
    Direction target_direction = getDirectionToPosition(target_x, target_y);

    if (target_direction != current_direction_) {
        return rotateToDirection(target_direction);
    }

    return ActionRequest::MoveForward;
}

bool TankAlgorithm_322719139_211961057_A::isPositionInFront(size_t x, size_t y) const {
    int dx = static_cast<int>(x) - static_cast<int>(current_x_);
    int dy = static_cast<int>(y) - static_cast<int>(current_y_);

    if (dx > static_cast<int>(board_width_) / 2) dx -= board_width_;
    if (dx < -static_cast<int>(board_width_) / 2) dx += board_width_;
    if (dy > static_cast<int>(board_height_) / 2) dy -= board_height_;
    if (dy < -static_cast<int>(board_height_) / 2) dy += board_height_;

    std::pair<int, int> movement = DirectionUtil::getMovement(current_direction_);
    return (dx == movement.first && dy == movement.second) || 
           (movement.first != 0 && dx * movement.first > 0 && (movement.second == 0 || dy * movement.second > 0)) ||
           (movement.second != 0 && dy * movement.second > 0 && (movement.first == 0 || dx * movement.first > 0));
}

Direction TankAlgorithm_322719139_211961057_A::getDirectionToPosition(size_t x, size_t y) const {
    int dx = static_cast<int>(x) - static_cast<int>(current_x_);
    int dy = static_cast<int>(y) - static_cast<int>(current_y_);

    if (dx > static_cast<int>(board_width_) / 2) dx -= board_width_;
    if (dx < -static_cast<int>(board_width_) / 2) dx += board_width_;
    if (dy > static_cast<int>(board_height_) / 2) dy -= board_height_;
    if (dy < -static_cast<int>(board_height_) / 2) dy += board_height_;

    return DirectionUtil::getDirectionFromDelta(dx, dy);
}

bool TankAlgorithm_322719139_211961057_A::canShoot() const {
    std::cout <<"CAN SHOOT" << std::endl;
    std::cout << "remaining_shells_" << remaining_shells_ << " cooldown_timer_" <<  cooldown_timer_ << std::endl;
    return remaining_shells_ > 0 && cooldown_timer_ == 0;
}

bool TankAlgorithm_322719139_211961057_A::isObstacle(int x, int y) const {
    if (board_.empty() || board_[0].empty()) {
        return true;
    }

    int board_width = static_cast<int>(board_[0].size());
    int board_height = static_cast<int>(board_.size());

    x = (x % board_width + board_width) % board_width;
    y = (y % board_height + board_height) % board_height;

    char object = board_[y][x];

    return (object == '#' || object == '$' || object == '@' ||
            object == std::to_string(3 - player_index_)[0]);
}

std::vector<std::pair<int, int>> TankAlgorithm_322719139_211961057_A::findPath(int target_x, int target_y) const {
    if (board_.empty() || board_[0].empty()) {
        return {};
    }

    int height = board_.size();
    int width = board_[0].size();

    int dx[] = {-1, 0, 1, -1, 1, -1, 0, 1};
    int dy[] = {-1, -1, -1, 0, 0, 1, 1, 1};

    std::queue<std::pair<int, int>> q;
    std::map<std::pair<int, int>, std::pair<int, int>> parent;
    std::set<std::pair<int, int>> visited;

    std::pair<int, int> start{-1, -1};
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (board_[y][x] == '%') {
                start = {x, y};
                break;
            }
        }
        if (start.first != -1) break;
    }

    if (start.first == -1) return {};

    q.push(start);
    visited.insert(start);

    std::pair<int, int> current;
    
    while (!q.empty()) {
        current = q.front();
        q.pop();

        if (current.first == target_x && current.second == target_y) {
            std::vector<std::pair<int, int>> path;
            while (current != start) {
                path.push_back(current);
                current = parent[current];
            }
            std::reverse(path.begin(), path.end());
            return path;
        }

        for (int i = 0; i < 8; ++i) {
            int nx = current.first + dx[i];
            int ny = current.second + dy[i];

            if (nx >= 0 && nx < width && ny >= 0 && ny < height &&
                visited.find({nx, ny}) == visited.end() &&
                !isObstacle(nx, ny)) {

                visited.insert({nx, ny});
                parent[{nx, ny}] = current;
                q.push({nx, ny});
            }
        }
    }

    return {};
}

ActionRequest TankAlgorithm_322719139_211961057_A::takeAction() {
    if (shouldRequestBattleInfo()) {
        return ActionRequest::GetBattleInfo;
    }
    return ActionRequest::DoNothing;
}

ActionRequest TankAlgorithm_322719139_211961057_A::determineOffensiveAction() {
    std::cout << "---------- determineOffensiveAction called ----------\n";
    MissionType mission = last_offensive_info_.getMissionType();
    const auto& path = last_offensive_info_.getPath();
    const auto& walls_to_destroy = last_offensive_info_.getWallsToDestroy();

    std::cout << "[DEBUG] OffensiveTankAlgorithm: MissionType = " << static_cast<int>(mission) << "\n";
    std::cout << "[DEBUG] OffensiveTankAlgorithm: Path size = " << path.size() 
              << ", WallsToDestroy size = " << walls_to_destroy.size() << "\n";
    std::cout << "[DEBUG] OffensiveTankAlgorithm: Path index = " << path_index_ 
              << ", Can hit target = " << can_hit_target_ << "\n";
    std::cout << "firing direction: " 
              << DirectionUtil::toString(last_offensive_info_.getFiringDirection()) << "\n";

    if (mission == MissionType::ATTACK && can_hit_target_) {
        std::cout << "[DEBUG] ATTACK mission and target in range -> Shooting\n";
        return ActionRequest::Shoot;
    }

    if (mission == MissionType::ATTACK && path_index_ < path.size()) {
        const auto& [next_x, next_y] = path[path_index_];
        std::cout << "[DEBUG] ATTACK mission: Next move to (" << next_x << ", " << next_y << ")\n";

        if (path_index_ < walls_to_destroy.size() && walls_to_destroy[path_index_].has_value()) {
            const auto& [obstacle_x, obstacle_y] = *walls_to_destroy[path_index_];
            std::cout << "[DEBUG] ATTACK mission: Wall in path at (" << obstacle_x << ", " << obstacle_y << ") -> Deciding action to destroy\n";
            return handleWallInPath(obstacle_x, obstacle_y);
        }

        std::cout << "[DEBUG] ATTACK mission: Executing moveTowardsPosition logic\n";

        ActionRequest possible_rotation = moveTowardsPosition(next_x, next_y);
        if(isRotationAction(possible_rotation))
        {
            current_direction_ = directionAfterRotationAction(possible_rotation);
            std::cout << "current_direction_ after rotation: " 
                << DirectionUtil::toString(current_direction_) << "\n";
            if(current_direction_ == getDirectionToPosition(next_x, next_y))
            {
                can_hit_target_ = true;
                std::cout << can_hit_target_ << " can hit target after rotation\n";
            }
        }
        return possible_rotation;
    }

    if (mission == MissionType::RETREAT && path_index_ < path.size()) {
        const auto& [next_x, next_y] = path[path_index_];
        std::cout << "[DEBUG] RETREAT mission: Moving towards (" << next_x << ", " << next_y << ")\n";
        ActionRequest possible_rotation = moveTowardsPosition(next_x, next_y);
        if(isRotationAction(possible_rotation))
        {
            current_direction_ = directionAfterRotationAction(possible_rotation);
            std::cout << "current_direction_ after rotation: " 
                << DirectionUtil::toString(current_direction_) << "\n";
            if(current_direction_ == getDirectionToPosition(next_x, next_y))
            {
                can_hit_target_ = true;
                std::cout << can_hit_target_ << " can hit target after rotation\n";
            }

        }
        return possible_rotation;
    }

    if (mission == MissionType::HOLD) {
        std::cout << "[DEBUG] HOLD mission: Doing nothing\n";
        return ActionRequest::DoNothing;
    }

    std::cout << "[DEBUG] Unknown mission or empty path: Defaulting to DoNothing\n";
    return ActionRequest::DoNothing;
}

ActionRequest TankAlgorithm_322719139_211961057_A::handleWallInPath(size_t wall_x, size_t wall_y) {
    Direction wall_direction = getDirectionToPosition(wall_x, wall_y);
    if (current_direction_ == wall_direction && canShoot()) {
        return ActionRequest::Shoot;
    }

    if (current_direction_ != wall_direction) {
        ActionRequest rotation_needed = rotateToDirection(wall_direction);
        current_direction_ = directionAfterRotationAction(rotation_needed);
        std::cout << "current_direction_ after rotation: " 
                  << DirectionUtil::toString(current_direction_) << "\n";
        return rotation_needed;
    }

    return ActionRequest::DoNothing; 
}

ActionRequest TankAlgorithm_322719139_211961057_A::attackTarget() {
    Direction target_direction = getDirectionToPosition(target_x_, target_y_);
    if (target_direction != current_direction_) {
        ActionRequest rotation_needed = rotateToDirection(target_direction);
        current_direction_ = directionAfterRotationAction(rotation_needed);
        std::cout << "current_direction_ after rotation: " 
            << DirectionUtil::toString(current_direction_) << "\n";
        return rotation_needed;
    }
    
    if (canShoot()) {
        return ActionRequest::Shoot;
    }
    
    return followPathToTarget();
}

ActionRequest TankAlgorithm_322719139_211961057_A::followPathToTarget() {
    if (path_to_target_.empty()) {
        return ActionRequest::DoNothing;
    }
    
    const auto& next_point = path_to_target_[0];
    
    ActionRequest possible_rotation = moveTowardsPosition(next_point.first, next_point.second);
    if(isRotationAction(possible_rotation))
    {
        current_direction_ = directionAfterRotationAction(possible_rotation);
        std::cout << "current_direction_ after rotation: " 
            << DirectionUtil::toString(current_direction_) << "\n";
        if(current_direction_ == getDirectionToPosition(next_point.first, next_point.second))
        {
            can_hit_target_ = true;
            std::cout << can_hit_target_ << " can hit target after rotation\n";
        }

    }
    return possible_rotation;
}

void TankAlgorithm_322719139_211961057_A::resetPathIndex() {
    path_index_ = 0;
}

bool TankAlgorithm_322719139_211961057_A::isRotationAction(ActionRequest action) {
    return action == ActionRequest::RotateLeft45 ||
           action == ActionRequest::RotateRight45 ||
           action == ActionRequest::RotateLeft90 ||
           action == ActionRequest::RotateRight90;
}


//REGISTER_TANK_ALGORITHM(TankAlgorithm_322719139_211961057_A); //it looks like its wrong, according to pdf, check and if so, correct one below:
//REGISTER_TANK_ALGORITHM(OffensiveTankAlgorithm_322719139_211961057);

} // namespace Algorithm_322719139_211961057

// Bring the class name into global scope for the macro
using Algorithm_322719139_211961057::TankAlgorithm_322719139_211961057_A;
REGISTER_TANK_ALGORITHM(TankAlgorithm_322719139_211961057_A);
