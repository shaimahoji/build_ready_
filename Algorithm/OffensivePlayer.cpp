#include <limits>
#include <iostream>
#include <sstream>
#include <set>


// NOTE: OffensiveTankAlgorithm is used ONLY in this .cpp (for dynamic_cast), so:
// Do NOT move this include to OffensivePlayer.h — keep header clean and decoupled.
#include "OffensiveTankAlgorithm.h"

#include "OffensivePlayer.h"

//in .cpp — since it directly uses REGISTER_PLAYER(...).
#include "../common/PlayerRegistration.h"

using namespace UserCommon_322719139_211961057;

namespace Algorithm_322719139_211961057 {

OffensivePlayer::OffensivePlayer(int player_index, size_t board_width, size_t board_height, size_t max_steps, size_t num_shells)
    : player_index_(player_index),
      max_steps_(max_steps),
      num_shells_(num_shells),
      board_width_(board_width),
      board_height_(board_height),
      last_board_state_(board_height, std::vector<char>(board_width, ' ')),
      current_step_(0),
      my_tanks_(),
      enemy_tanks_(),
      aggression_level_(0.8) {

    size_t start_x = board_width_ / 2;
    size_t start_y = board_height_ / 2;
    
    my_tanks_.emplace_back(start_x, start_y,
        (player_index == 1) ? Direction::LEFT : Direction::RIGHT,
        num_shells_);
}

OffensivePlayer::OffensivePlayer(const OffensivePlayer& other)
    : Player(other),
      player_index_(other.player_index_),
      max_steps_(other.max_steps_),
      num_shells_(other.num_shells_),
      board_width_(other.board_width_),
      board_height_(other.board_height_),
      last_board_state_(other.last_board_state_),
      current_step_(other.current_step_),
      my_tanks_(other.my_tanks_),
      enemy_tanks_(other.enemy_tanks_),
      observed_shells_(other.observed_shells_),
      known_mines_(other.known_mines_),
      known_walls_(other.known_walls_),
      aggression_level_(other.aggression_level_),
      target_priorities_(other.target_priorities_),
      mission_plans_per_tanks(other.mission_plans_per_tanks),
      paths_per_missions(other.paths_per_missions) {
}

void OffensivePlayer::updateTankWithBattleInfo(TankAlgorithm& tank, SatelliteView& satellite_view) {
    std::cout << "---------- updateTankWithBattleInfo ---------- \n";

    auto* offensive_tank = dynamic_cast<OffensiveTankAlgorithm*>(&tank);
    if (!offensive_tank) {
        std::cerr << "[Error] Invalid tank type\n";
        return;
    }

    int tank_index = getTankIndexFromSatellite(satellite_view);
    if (tank_index < 0) {
        std::cerr << "[Error] Failed to find tank index from satellite view\n";
        return;
    }

    std::cout << "[DEBUG] Tank index: " << tank_index << "\n";

    if (tank_index < 0 || tank_index >= static_cast<int>(my_tanks_.size())) {
        std::cerr << "[Error] Tank index out of range\n";
        return;
    }

    updateBoardInfo(satellite_view, tank_index);
    updateTargetPriorities();
    createMissionsForTanks();

    OffensiveBattleInfo battle_info;

    TankInfo& tank_info = my_tanks_[tank_index];

    battle_info.setCurrentPosition(tank_info.x, tank_info.y);
    battle_info.setCurrentDirection(tank_info.direction);
    battle_info.setRemainingShells(tank_info.remaining_shells);
    battle_info.setCooldownTimer(tank_info.cooldown_timer);
    battle_info.setBoardDimensions(board_width_, board_height_);
    battle_info.setGameState(current_step_, max_steps_);

    bool in_danger = isTankUnderThreat(tank_info);
    battle_info.setUnderThreat(in_danger);

    std::vector<std::pair<size_t, size_t>> danger_sources;
    for (const auto& shell : observed_shells_) {
        size_t dx = toroidalDistance(tank_info.x, shell.x, board_width_);
        size_t dy = toroidalDistance(tank_info.y, shell.y, board_height_);

        if (dx <= 2 && dy <= 2 && !hasObstacleBetween(tank_info.x, tank_info.y, shell.x, shell.y, false)) {
            danger_sources.emplace_back(shell.x, shell.y);
        }
    }

    for (const auto& source : danger_sources) {
        battle_info.addIncomingShell(source.first, source.second);
    }

    if (tank_index < static_cast<int>(mission_plans_per_tanks.size())) {
        const MissionPlan& plan = mission_plans_per_tanks[tank_index];

        battle_info.setMissionType(plan.mission_type);
        battle_info.setPath(plan.getPath());
        battle_info.setWallsToDestroy(plan.getWallsToDestroy());

        std::cout << "Tank " << tank_index << " mission: " 
                  << static_cast<int>(plan.mission_type) << "\n";
        std::cout << "Path (" << plan.getPath().size() << " steps): ";
        for (const auto& pos : plan.getPath()) {
            std::cout << "(" << pos.first << "," << pos.second << ") ";
        }
        std::cout << "\nDestroy targets: " << plan.getWallsToDestroy().size() << "\n";

        for (const TankInfo& enemy : enemy_tanks_) {
            if (enemy.x == plan.getPositionX() && enemy.y == plan.getPositionY()) {
                std::cout << "my tank position: (" 
                          << tank_info.x << "," << tank_info.y << ")\n";
                std::cout << "Targeting enemy at (" << enemy.x << "," << enemy.y << ")\n";
                battle_info.setTarget(enemy.x, enemy.y, enemy.direction);
                battle_info.setCanHitTarget(plan.isDirectFirePossible());
                break;
            }
        }
    }

    offensive_tank->updateBattleInfo(battle_info);
}

void OffensivePlayer::updateBoardInfo(SatelliteView& satellite_view, int tank_index) {
    observed_shells_.clear();
    
    size_t tank_x = 0, tank_y = 0;
    bool found_tank = false;

    for (size_t y = 0; y < board_height_; ++y) {
        for (size_t x = 0; x < board_width_; ++x) {
            char object = satellite_view.getObjectAt(x, y);
            
            last_board_state_[y][x] = object;

            if (object == '%') {
                tank_x = x;
                tank_y = y;
                found_tank = true;
                
                if (tank_index >= 0 && tank_index < static_cast<int>(my_tanks_.size())) {
                    my_tanks_[tank_index].x = x;
                    my_tanks_[tank_index].y = y;
                }
            }
            else if (object == std::to_string(player_index_)[0]) {
                updateMyTankPosition(x, y, tank_index);
            }
            else if (object == std::to_string(3 - player_index_)[0]) {
                updateEnemyTankPosition(x, y);
            }
            else if (object == '*') {
                observed_shells_.push_back(ShellInfo(x, y, Direction::NONE, current_step_, false));
            }
            else if (object == '#') {
                updateWallPosition(x, y,false);
            }
            else if (object == '$') {
                updateWallPosition(x, y,true);
            }
            else if (object == '@') {
                updateMinePosition(x, y);
            }
        }
    }

    if (found_tank && tank_index >= 0 && tank_index < static_cast<int>(my_tanks_.size())) {
        my_tanks_[tank_index].x = tank_x;
        my_tanks_[tank_index].y = tank_y;
        my_tanks_[tank_index].last_info_step = current_step_;
    }

    current_step_++;
}

int OffensivePlayer::getTankIndexFromSatellite(const SatelliteView& satellite_view) {
    int tank_index = 0;

    for (size_t y = 0; y < board_height_; ++y) {
        for (size_t x = 0; x < board_width_; ++x) {

            char object = satellite_view.getObjectAt(x, y);

            if(object =='1' || object == '2') {
                tank_index += 1;
            }else if(object == '%') {
                return (tank_index);
            }
        }
    }

    return -1;
}

std::vector<std::pair<size_t, size_t>> OffensivePlayer::findPathOLD(
    size_t start_x, size_t start_y, size_t goal_x, size_t goal_y) const {
    struct Node {
        size_t x, y;
        double g_cost;
        double h_cost;
        double f_cost;
        std::pair<size_t, size_t> parent;
        
        Node(size_t x, size_t y, double g, double h, std::pair<size_t, size_t> parent)
            : x(x), y(y), g_cost(g), h_cost(h), f_cost(g + h), parent(parent) {}
        
        bool operator>(const Node& other) const {
            return f_cost > other.f_cost;
        }
    };
    
    std::priority_queue<Node, std::vector<Node>, std::greater<Node>> open_set;
    
    std::unordered_set<size_t> closed_set;
    
    auto hash_position = [this](size_t x, size_t y) {
        return y * board_width_ + x;
    };
    
    auto heuristic = [](size_t x, size_t y, size_t goal_x, size_t goal_y) {
        return std::abs(static_cast<int>(goal_x) - static_cast<int>(x)) + 
               std::abs(static_cast<int>(goal_y) - static_cast<int>(y));
    };
    
    open_set.push(Node(start_x, start_y, 0, heuristic(start_x, start_y, goal_x, goal_y), 
                      {start_x, start_y}));
    
    const std::vector<std::pair<int, int>> directions = {
        {0, -1}, {1, 0}, {0, 1}, {-1, 0},
        {1, -1}, {1, 1}, {-1, 1}, {-1, -1}
    };
    
    std::unordered_map<size_t, std::pair<size_t, size_t>> came_from;
    
    while (!open_set.empty()) {
        Node current = open_set.top();
        open_set.pop();
        
        if (current.x == goal_x && current.y == goal_y) {
            std::vector<std::pair<size_t, size_t>> path;
            std::pair<size_t, size_t> current_pos = {current.x, current.y};
            
            while (current_pos.first != start_x || current_pos.second != start_y) {
                path.push_back(current_pos);
                current_pos = came_from[hash_position(current_pos.first, current_pos.second)];
            }
            
            std::reverse(path.begin(), path.end());
            return path;
        }
        
        closed_set.insert(hash_position(current.x, current.y));
        
        for (const auto& dir : directions) {
            size_t new_x = current.x + dir.first;
            size_t new_y = current.y + dir.second;
            
            if (isObstacle(new_x, new_y)) {
                continue;
            }
            
            if (closed_set.find(hash_position(new_x, new_y)) != closed_set.end()) {
                continue;
            }
            
            double new_g_cost = current.g_cost + 1;
            
            Node neighbor(new_x, new_y, new_g_cost, 
                         heuristic(new_x, new_y, goal_x, goal_y), 
                         {current.x, current.y});
            
            open_set.push(neighbor);
            
            came_from[hash_position(new_x, new_y)] = {current.x, current.y};
        }
    }
    
    return {};
}

Direction OffensivePlayer::estimateDirection(size_t prev_x, size_t prev_y, 
                                           size_t new_x, size_t new_y,Direction prev_dir) 
{

    int dx = static_cast<int>(new_x) - static_cast<int>(prev_x);
    int dy = static_cast<int>(new_y) - static_cast<int>(prev_y);

    dx = std::clamp(dx, -1, 1);
    dy = std::clamp(dy, -1, 1);

    if (dx == 0 && dy == 0) {
        return DirectionUtil::rotateRight(prev_dir);
    }

    if (dx == 0 && dy == -1) return Direction::UP;
    if (dx == -1 && dy == 0) return Direction::LEFT;
    if (dx == 1 && dy == 0)  return Direction::RIGHT;
    if (dx == -1 && dy == 1) return Direction::DOWN_LEFT;
    if (dx == -1 && dy == -1)return Direction::UP_LEFT;
    if (dx == 1 && dy == -1) return Direction::UP_RIGHT;
    if (dx == 1 && dy == 1) return Direction::DOWN_RIGHT;
    if (dx == 0 && dy == 1) return Direction::DOWN;

    return prev_dir;
}

bool OffensivePlayer::isValidPosition(size_t x, size_t y) const {
    return x < board_width_ && y < board_height_;
}

bool OffensivePlayer::isObstacle(size_t x, size_t y) const {
    if (!isValidPosition(x, y)) {
        return true;
    }
    
    char object = last_board_state_[y][x];
    return object == '#'|| object == '$' || object == '@';
}

std::vector<std::pair<size_t, size_t>> OffensivePlayer::findPath(
    size_t start_x, size_t start_y, size_t goal_x, size_t goal_y) const {
    struct Node {
        size_t x, y;
        double g_cost;
        double h_cost;
        double f_cost;
        std::pair<size_t, size_t> parent;
        
        Node(size_t x, size_t y, double g, double h, std::pair<size_t, size_t> parent)
            : x(x), y(y), g_cost(g), h_cost(h), f_cost(g + h), parent(parent) {}
        
        bool operator>(const Node& other) const {
            return f_cost > other.f_cost;
        }
    };
    
    std::priority_queue<Node, std::vector<Node>, std::greater<Node>> open_set;
    
    std::unordered_set<size_t> closed_set;
    
    auto hash_position = [this](size_t x, size_t y) {
        return y * board_width_ + x;
    };
    
    auto heuristic = [](size_t x, size_t y, size_t goal_x, size_t goal_y) {
        return std::abs(static_cast<int>(goal_x) - static_cast<int>(x)) + 
               std::abs(static_cast<int>(goal_y) - static_cast<int>(y));
    };
    
    open_set.push(Node(start_x, start_y, 0, heuristic(start_x, start_y, goal_x, goal_y), 
                      {start_x, start_y}));
    
    const std::vector<std::pair<int, int>> directions = {
        {0, -1}, {1, 0}, {0, 1}, {-1, 0},
        {1, -1}, {1, 1}, {-1, 1}, {-1, -1}
    };
    
    std::unordered_map<size_t, std::pair<size_t, size_t>> came_from;
    
    while (!open_set.empty()) {
        Node current = open_set.top();
        open_set.pop();
        
        if (current.x == goal_x && current.y == goal_y) {
            std::vector<std::pair<size_t, size_t>> path;
            std::pair<size_t, size_t> current_pos = {current.x, current.y};
            
            while (current_pos.first != start_x || current_pos.second != start_y) {
                path.push_back(current_pos);
                current_pos = came_from[hash_position(current_pos.first, current_pos.second)];
            }
            
            std::reverse(path.begin(), path.end());
            return path;
        }
        
        closed_set.insert(hash_position(current.x, current.y));
        
        for (const auto& dir : directions) {
            size_t new_x = current.x + dir.first;
            size_t new_y = current.y + dir.second;
            
            if (isObstacle(new_x, new_y)) {
                continue;
            }
            
            if (closed_set.find(hash_position(new_x, new_y)) != closed_set.end()) {
                continue;
            }
            
            double new_g_cost = current.g_cost + 1;
            
            Node neighbor(new_x, new_y, new_g_cost, 
                         heuristic(new_x, new_y, goal_x, goal_y), 
                         {current.x, current.y});
            
            open_set.push(neighbor);
            
            came_from[hash_position(new_x, new_y)] = {current.x, current.y};
        }
    }
    
    return {};
}

Direction OffensivePlayer::calculateDirection(size_t from_x, size_t from_y, size_t to_x, size_t to_y) const {
    int dx = static_cast<int>(to_x) - static_cast<int>(from_x);
    int dy = static_cast<int>(to_y) - static_cast<int>(from_y);

    return DirectionUtil::getDirectionFromDelta(dx, dy);
}

std::vector<Direction> OffensivePlayer::getRotationSequence(Direction current_dir, Direction target_dir) const {
    std::vector<Direction> rotations;
    Direction temp_dir = current_dir;
    
    static const std::unordered_map<Direction, int> dir_to_angle = {
        {Direction::UP, 0},
        {Direction::UP_RIGHT, 1},
        {Direction::RIGHT, 2},
        {Direction::DOWN_RIGHT, 3},
        {Direction::DOWN, 4},
        {Direction::DOWN_LEFT, 5},
        {Direction::LEFT, 6},
        {Direction::UP_LEFT, 7}
    };

    int current_angle = dir_to_angle.at(current_dir);
    int target_angle = dir_to_angle.at(target_dir);

    int diff = (target_angle - current_angle + 8) % 8;
    int clockwise_steps = diff;
    int counter_clockwise_steps = (8 - diff) % 8;

    if (clockwise_steps <= counter_clockwise_steps) {
        while (temp_dir != target_dir) {
            temp_dir = DirectionUtil::rotateRight(temp_dir);
            rotations.push_back(temp_dir);
        }
    } else {
        while (temp_dir != target_dir) {
            temp_dir = DirectionUtil::rotateLeft(temp_dir);
            rotations.push_back(temp_dir);
        }
    }

    return rotations;
}

std::vector<std::pair<size_t, size_t>> OffensivePlayer::predictShellPath(
    size_t x, size_t y, Direction direction, int steps) const {
    std::vector<std::pair<size_t, size_t>> path;
    
    auto movement = DirectionUtil::getMovement(direction);
    int dx = movement.first * 2;
    int dy = movement.second * 2;
    
    for (int i = 1; i <= steps; ++i) {
        x = (x + dx + board_width_) % board_width_;
        y = (y + dy + board_height_) % board_height_;
        
        path.push_back({x, y});
        
        if (isObstacle(x, y)) {
            break;
        }
    }
    
    return path;
}

std::pair<size_t, size_t> OffensivePlayer::findClosestEnemyTank(size_t from_x, size_t from_y) const {
    double min_distance = std::numeric_limits<double>::max();
    std::pair<size_t, size_t> closest = {0, 0};
    bool found = false;
    
    for (const auto& enemy : enemy_tanks_) {
        double distance = calculateDistance(from_x, from_y, enemy.x, enemy.y);
        
        if (distance < min_distance) {
            min_distance = distance;
            closest = {enemy.x, enemy.y};
            found = true;
        }
    }
    
    return found ? closest : std::make_pair(from_x, from_y);
}

bool OffensivePlayer::hasLineOfSight(size_t from_x, size_t from_y, size_t to_x, size_t to_y) const {
    int dx = std::abs(static_cast<int>(to_x) - static_cast<int>(from_x));
    int dy = std::abs(static_cast<int>(to_y) - static_cast<int>(from_y));
    int sx = from_x < to_x ? 1 : -1;
    int sy = from_y < to_y ? 1 : -1;
    int err = dx - dy;
    
    while (from_x != to_x || from_y != to_y) {
        if (isObstacle(from_x, from_y)) {
            return false;
        }
        
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            from_x += sx;
        }
        if (e2 < dx) {
            err += dx;
            from_y += sy;
        }
    }
    
    return true;
}

bool OffensivePlayer::hasLineOfSightLimited(size_t from_x, size_t from_y, size_t to_x, size_t to_y) const {
    int dist = manhattanDistance(from_x, from_y, to_x, to_y);
    if (dist > 4) return false;

    if (!canReachWithinTwoSteps(from_x, from_y, to_x, to_y)) return false;

    return hasLineOfSight(from_x, from_y, to_x, to_y);
}

int OffensivePlayer::manhattanDistance(int x1, int y1, int x2, int y2) const {
    return std::abs(x1 - x2) + std::abs(y1 - y2);
}

bool OffensivePlayer::canReachWithinTwoSteps(int from_x, int from_y, int to_x, int to_y) const {
    const std::vector<std::pair<int, int>> directions = {
        {1, 0}, {-1, 0}, {0, 1}, {0, -1},
        {1, 1}, {1, -1}, {-1, 1}, {-1, -1}
    };

    for (auto [dx1, dy1] : directions) {
        int x1 = (from_x + dx1 + board_width_) % board_width_;
        int y1 = (from_y + dy1 + board_height_) % board_height_;

        if (isObstacle(x1, y1)) continue;
        if (x1 == to_x && y1 == to_y) return true;

        for (auto [dx2, dy2] : directions) {
            int x2 = (x1 + dx2 + board_width_) % board_width_;
            int y2 = (y1 + dy2 + board_height_) % board_height_;

            if (isObstacle(x2, y2)) continue;
            if (x2 == to_x && y2 == to_y) return true;
        }
    }

    return false;
}

void OffensivePlayer::updateMyTankPosition(size_t x, size_t y, int excluding_tank_index) {
    for (size_t i = 0; i < my_tanks_.size(); ++i) {
        if (static_cast<int>(i) != excluding_tank_index) {
            if (my_tanks_[i].x == x && my_tanks_[i].y == y) {
                my_tanks_[i].last_info_step = current_step_;
                return;
            }
        }
    }
    
    Direction default_direction = (player_index_ == 1) ? Direction::LEFT : Direction::RIGHT;
    my_tanks_.push_back(TankInfo(x, y, default_direction, num_shells_));
}

void OffensivePlayer::updateEnemyTankPosition(size_t x, size_t y) {
    for (auto& enemy : enemy_tanks_) {
        if (enemy.x == x && enemy.y == y) {
            enemy.last_info_step = current_step_;
            return;
        }
    }
    
    Direction default_direction = (player_index_ == 1) ? Direction::RIGHT : Direction::LEFT;
    enemy_tanks_.push_back(TankInfo(x, y, default_direction, num_shells_));
}

void OffensivePlayer::updateWallPosition(size_t x, size_t y,bool damaged) {
    auto it = std::find_if(known_walls_.begin(), known_walls_.end(),
        [x, y](const WallInfo& wall) {
            return wall.x == x && wall.y == y;
        });

    if (it == known_walls_.end()) {
        if(!damaged){
        known_walls_.push_back(WallInfo(x, y,2, current_step_));
        }
        else{
        known_walls_.push_back(WallInfo(x, y, 1, current_step_));
        }
    } else {
        it->last_seen_step = current_step_;
    }
}

void OffensivePlayer::updateMinePosition(size_t x, size_t y) {
    auto it = std::find(known_mines_.begin(), known_mines_.end(), std::make_pair(x, y));
    if (it == known_mines_.end()) {
        known_mines_.push_back({x, y});
    }
}

double OffensivePlayer::calculateDistance(size_t x1, size_t y1, size_t x2, size_t y2) const {
    int dx = std::abs(static_cast<int>(x2) - static_cast<int>(x1));
    int dy = std::abs(static_cast<int>(y2) - static_cast<int>(y1));
    
    dx = std::min(dx, static_cast<int>(board_width_) - dx);
    dy = std::min(dy, static_cast<int>(board_height_) - dy);
    
    return std::sqrt(dx * dx + dy * dy);
}

void OffensivePlayer::updateTargetPriorities() {
    target_priorities_.clear();
    
    for (size_t i = 0; i < enemy_tanks_.size(); ++i) {
        const TankInfo& enemy = enemy_tanks_[i];
        
        if (!enemy.is_alive) {
            continue;
        }
        
        double min_distance = std::numeric_limits<double>::max();
        for (const auto& my_tank : my_tanks_) {
            double distance = calculateDistance(my_tank.x, my_tank.y, enemy.x, enemy.y);
            min_distance = std::min(min_distance, distance);
        }
        
        double priority = 100.0 / (min_distance + 1.0);
        
        for (const auto& my_tank : my_tanks_) {
            if (hasLineOfSight(my_tank.x, my_tank.y, enemy.x, enemy.y)) {
                priority *= 1.5;
                break;
            }
        }
        
        target_priorities_.push_back({i, priority});
    }
    
    std::sort(target_priorities_.begin(), target_priorities_.end(),
             [](const TargetPriority& a, const TargetPriority& b) {
                 return a.priority > b.priority;
             });
}

void OffensivePlayer::createMissionsForTanks() {
    std::cout << "---------- createMissionsForTanks ---------- \n";
    mission_plans_per_tanks.clear();

    if (enemy_tanks_.empty() || target_priorities_.empty()) {
        std::cout << "[DEBUG] No enemies or priorities, assigning HOLD mission to all tanks.\n";
        for (size_t i = 0; i < my_tanks_.size(); ++i) {
            MissionPlan default_plan;
            default_plan.mission_type = MissionType::HOLD;
            default_plan.position_x_ = my_tanks_[i].x;
            default_plan.position_y_ = my_tanks_[i].y;
            default_plan.firing_direction = Direction::NONE;
            default_plan.direct_fire_possible = false;
            default_plan.estimated_steps = 0;
            mission_plans_per_tanks.push_back(default_plan);
        }
        return;
    }

    std::unordered_map<size_t, TargetPriority*> enemy_priority_map;
    for (auto& prio : target_priorities_) {
        enemy_priority_map[prio.tank_index] = &prio;
    }

    std::vector<std::vector<std::pair<size_t, double>>> tank_target_scores;

    for (size_t i = 0; i < my_tanks_.size(); ++i) {
        const TankInfo& my_tank = my_tanks_[i];
        std::vector<std::pair<size_t, double>> target_scores;

        for (size_t j = 0; j < enemy_tanks_.size(); ++j) {
            const TankInfo& enemy_tank = enemy_tanks_[j];
            TargetPriority* prio_ptr = enemy_priority_map.count(j) ? enemy_priority_map[j] : nullptr;
            double score = scoreTargetForTank(my_tank, enemy_tank, prio_ptr);
            if (score > -1e8) {
                target_scores.emplace_back(j, score);
            }
        }

        std::sort(target_scores.begin(), target_scores.end(),
            [](const auto& a, const auto& b) {
                return a.second > b.second;
            });

        tank_target_scores.push_back(target_scores);
    }

    std::vector<bool> enemy_targeted(enemy_tanks_.size(), false);
    std::vector<size_t> tank_assignments(my_tanks_.size(), SIZE_MAX);

    for (size_t i = 0; i < my_tanks_.size(); ++i) {
        for (const auto& [target_idx, score] : tank_target_scores[i]) {
            if (!enemy_targeted[target_idx]) {
                tank_assignments[i] = target_idx;
                enemy_targeted[target_idx] = true;
                break;
            }
        }
    }

    for (size_t i = 0; i < my_tanks_.size(); ++i) {
        if (tank_assignments[i] == SIZE_MAX && !tank_target_scores[i].empty()) {
            tank_assignments[i] = tank_target_scores[i][0].first;
        }
    }

    for (size_t i = 0; i < my_tanks_.size(); ++i) {
        if (tank_assignments[i] != SIZE_MAX) {
            mission_plans_per_tanks.push_back(createMissionPlan(i, tank_assignments[i]));
        } else {
            MissionPlan fallback;
            fallback.mission_type = MissionType::HOLD;
            fallback.position_x_ = my_tanks_[i].x;
            fallback.position_y_ = my_tanks_[i].y;
            fallback.firing_direction = Direction::NONE;
            fallback.direct_fire_possible = false;
            fallback.estimated_steps = 0;
            mission_plans_per_tanks.push_back(fallback);
        }
    }

    for (size_t i = 0; i < my_tanks_.size(); ++i) {
        std::cout << "[Tank " << i << "] target scores:\n";
        for (const auto& [target_idx, score] : tank_target_scores[i]) {
            std::cout << "  -> Enemy " << target_idx << ": " << score << "\n";
        }
        if (tank_assignments[i] != SIZE_MAX) {
            std::cout << "Assigned to enemy " << tank_assignments[i] << "\n";
        } else {
            std::cout << "No valid assignment\n";
        }
    }
}

std::optional<std::pair<int, int>> OffensivePlayer::findSafeRetreatPosition(const TankInfo& tank) const {
    auto [dx, dy] = DirectionUtil::getMovement(tank.direction);
    int fx = (tank.x + dx + board_width_) % board_width_;
    int fy = (tank.y + dy + board_height_) % board_height_;

    char forward_tile = last_board_state_[fy][fx];
    if (forward_tile != '#' && forward_tile != '$' && forward_tile != '@' && !isTileUnderThreat(fx, fy)) {
        return std::make_pair(fx, fy);
    }

    std::vector<std::pair<int, int>> candidates;
    for (int dir = 0; dir < 8; ++dir) {
        auto [dx, dy] = DirectionUtil::getMovement(static_cast<Direction>(dir));
        int nx = (tank.x + dx + board_width_) % board_width_;
        int ny = (tank.y + dy + board_height_) % board_height_;

        char tile = last_board_state_[ny][nx];
        if (tile == '#' || tile == '$' || tile == '@') continue;
        if (isTileUnderThreat(nx, ny)) continue;

        candidates.emplace_back(nx, ny);
    }

    if (candidates.empty()) return std::nullopt;

    auto distanceToNearestThreat = [&](int x, int y) {
        int min_dist = std::numeric_limits<int>::max();
        for (const auto& enemy : enemy_tanks_) {
            if (!enemy.is_alive) continue;
            int dx = toroidalDistance(x, enemy.x, board_width_);
            int dy = toroidalDistance(y, enemy.y, board_height_);
            min_dist = std::min(min_dist, dx + dy);
        }
        for (const auto& shell : observed_shells_) {
            int dx = toroidalDistance(x, shell.x, board_width_);
            int dy = toroidalDistance(y, shell.y, board_height_);
            min_dist = std::min(min_dist, dx + dy);
        }
        return min_dist;
    };

    auto best_it = std::max_element(candidates.begin(), candidates.end(),
        [&](const auto& a, const auto& b) {
            return distanceToNearestThreat(a.first, a.second) <
                   distanceToNearestThreat(b.first, b.second);
        });

    return *best_it;
}

bool OffensivePlayer::isTileUnderThreat(int x, int y) const {
    TankInfo dummy_tank(x, y, Direction::NONE, true);
    return isTankUnderThreat(dummy_tank);
}

MissionPlan OffensivePlayer::createMissionPlan(size_t my_tank_index, size_t enemy_tank_index) {
    std::cout << "---------- createMissionPlan for tank ---------- " 
              << my_tank_index << " targeting enemy " << enemy_tank_index << " --- \n";
    MissionPlan plan;

    if (my_tank_index >= my_tanks_.size() || enemy_tank_index >= enemy_tanks_.size()) {
        std::cout << "[HOLD] default plan for tank " << my_tank_index 
                  << " targeting enemy " << enemy_tank_index << "\n";
        return plan;
    }

    const TankInfo& my_tank = my_tanks_[my_tank_index];
    const TankInfo& enemy_tank = enemy_tanks_[enemy_tank_index];

    PathResult path_result = findPath(my_tank.x, my_tank.y, enemy_tank.x, enemy_tank.y, last_board_state_);
    
    plan.setPath(path_result.path);
    plan.setWallsToDestroy(path_result.destroy);
    plan.setPositionX(enemy_tank.x);
    plan.setPositionY(enemy_tank.y);
    plan.setEstimatedSteps(static_cast<int>(path_result.path.size()));

    if ((my_tank.cooldown_timer > 0 || my_tank.remaining_shells == 0) && 
        isTankUnderThreat(my_tank)) {
        plan.setMissionType(MissionType::RETREAT);

        auto safe_pos = findSafeRetreatPosition(my_tank);
        if (safe_pos) {
            plan.setPositionX(safe_pos->first);
            plan.setPositionY(safe_pos->second);

            PathResult retreat_path = findPath(
                my_tank.x, my_tank.y,
                safe_pos->first, safe_pos->second,
                last_board_state_
            );

            plan.setPath(retreat_path.path);
            plan.setWallsToDestroy(retreat_path.destroy);
            plan.setEstimatedSteps(static_cast<int>(retreat_path.path.size()));
        } else {
            plan.setMissionType(MissionType::HOLD);
            plan.setPath({});
            plan.setWallsToDestroy({});
            plan.setEstimatedSteps(0);
        }

        return plan;
    }

    Direction fire_dir = calculateDirection(
        my_tank.x, my_tank.y,
        enemy_tank.x, enemy_tank.y
    );
    bool can_fire = canHitTarget(
        my_tank.x, my_tank.y,
        my_tank.direction,
        enemy_tank.x, enemy_tank.y
    );

    plan.setMissionType(MissionType::ATTACK);
    plan.setFiringDirection(fire_dir);
    plan.setDirectFirePossible(can_fire);

    return plan;
}

bool OffensivePlayer::isTankUnderThreat(const TankInfo& tank) const {
    const size_t threat_range = 2;

    for (const auto& enemy : enemy_tanks_) {
        if (!enemy.is_alive) continue;

        size_t dx = toroidalDistance(tank.x, enemy.x, board_width_);
        size_t dy = toroidalDistance(tank.y, enemy.y, board_height_);

        if (dx <= threat_range && dy <= threat_range) {

            if (!hasObstacleBetween(tank.x, tank.y, enemy.x, enemy.y, true))
            {
                return true;
            }
        }
    }

    for (const auto& shell : observed_shells_) {
        size_t dx = toroidalDistance(tank.x, shell.x, board_width_);
        size_t dy = toroidalDistance(tank.y, shell.y, board_height_);

        if (dx <= threat_range && dy <= threat_range) {

            if (!hasObstacleBetween(tank.x, tank.y, shell.x, shell.y, false))
            {
                return true;
            }
        }
    }

    return false;
}

bool OffensivePlayer::hasObstacleBetween(int x1, int y1, int x2, int y2, bool consider_mines) const {
    int dx = x2 - x1;
    int dy = y2 - y1;

    if (dx >  static_cast<int>(board_width_ / 2)) dx -= board_width_;
    if (dx < -static_cast<int>(board_width_ / 2)) dx += board_width_;
    if (dy >  static_cast<int>(board_height_ / 2)) dy -= board_height_;
    if (dy < -static_cast<int>(board_height_ / 2)) dy += board_height_;


    int steps = std::max(abs(dx), abs(dy));
    if (steps == 0) return false;

    double stepX = dx / static_cast<double>(steps);
    double stepY = dy / static_cast<double>(steps);

    for (int i = 1; i <= steps; ++i) {
        int cx = static_cast<int>(std::round(x1 + i * stepX));
        int cy = static_cast<int>(std::round(y1 + i * stepY));

        cx = (cx + board_width_) % board_width_;
        cy = (cy + board_height_) % board_height_;

        char tile = last_board_state_[cy][cx];

        if (tile == '#' || tile == '$' || (consider_mines && tile == '@')) {
            return true;
        }
    }

    return false;
}

int OffensivePlayer::toroidalDistance(int a, int b, int size) const {
    int delta = abs(a - b);
    return std::min(delta, size - delta);
}

bool OffensivePlayer::canHitTarget(size_t from_x, size_t from_y, Direction direction,
                                   size_t target_x, size_t target_y) const {
    Direction required_direction = calculateDirection(from_x, from_y, target_x, target_y);
    if (required_direction != direction)
        return false;

    return !hasObstacleBetween(from_x, from_y, target_x, target_y, true);
}

bool OffensivePlayer::shouldAttemptHit(size_t from_x, size_t from_y, Direction direction,
                                       size_t target_x, size_t target_y) const {
    int dx = toroidalDistance(from_x, target_x, board_width_);
    int dy = toroidalDistance(from_y, target_y, board_height_);
    double current_distance = std::sqrt(dx * dx + dy * dy);

    Direction opposite_dir = DirectionUtil::getOppositeDirection(direction);

    bool better_hit_possible = false;
    double reverse_distance = current_distance;

    if (canHitTarget(from_x, from_y, opposite_dir, target_x, target_y)) {
        Direction reverse_target_dir = calculateDirection(from_x, from_y, target_x, target_y);
        if (reverse_target_dir == opposite_dir) {
            int dx_rev = toroidalDistance(from_x, target_x, board_width_);
            int dy_rev = toroidalDistance(from_y, target_y, board_height_);
            reverse_distance = std::sqrt(dx_rev * dx_rev + dy_rev * dy_rev);
            better_hit_possible = reverse_distance + 2 < current_distance;
        }
    }

    double board_diag = std::sqrt(board_width_ * board_width_ + board_height_ * board_height_);
    bool within_effective_range = current_distance <= 0.5 * board_diag;

    return within_effective_range || !better_hit_possible;
}

double OffensivePlayer::scoreTargetForTank(const TankInfo& my_tank, const TankInfo& enemy_tank, const TargetPriority* priority_ptr) {
    if (!enemy_tank.is_alive) return -1e9;

    double score = 0;

    int dx = static_cast<int>(abs((int)my_tank.x - (int)enemy_tank.x));
    int dy = static_cast<int>(abs((int)my_tank.y - (int)enemy_tank.y));
    double dist = sqrt(dx * dx + dy * dy);
    score -= dist * 1.0;

    if (canHitTarget(my_tank.x, my_tank.y, my_tank.direction, enemy_tank.x, enemy_tank.y)) {
        score += 30;
    } else if (hasLineOfSight(my_tank.x, my_tank.y, enemy_tank.x, enemy_tank.y)) {
        score += 10;
    }

    score *= (1.0 + aggression_level_ * 0.1);

    if (my_tank.remaining_shells == 0 || my_tank.cooldown_timer > 0) {
        score -= 20;
    }

    if (priority_ptr != nullptr) {
        score *= (1.0 + priority_ptr->priority);
    }

    int turns_to_align = computeTurnsToAlign(my_tank.direction, my_tank.x, my_tank.y, enemy_tank.x, enemy_tank.y);
    if (turns_to_align > 2) {
        score -= turns_to_align * 5;
    }

    if (aggression_level_ < 0.3 && dist > 5) {
        score -= 10;
    } else if (aggression_level_ >= 0.7 && dist < 3) {
        score += 5;
    }

    return score;
}

int OffensivePlayer::computeTurnsToAlign(Direction current_dir, size_t from_x, size_t from_y, size_t to_x, size_t to_y) const {
    Direction target_dir = calculateDirection(from_x, from_y, to_x, to_y);
    
    if (current_dir == target_dir) {
        return 0;
    }
    
    std::vector<Direction> rotations = getRotationSequence(current_dir, target_dir);
    
    return rotations.size();
}

int OffensivePlayer::getShootingCost(char cell) {
    if (cell == '#') return 2 * (1 + 4) + 1;
    if (cell == '$') return 1 * (1 + 4) + 1;
    if (cell == '@') return INT_MAX;
    return 1;
}

PathResult OffensivePlayer::findPath(
    size_t start_x, size_t start_y,
    size_t goal_x, size_t goal_y,
    const std::vector<std::vector<char>>& board_state,
    size_t path_max_length) {

    std::priority_queue<Node> pq;
    std::unordered_map<size_t, int> dist;
    std::unordered_map<size_t, std::pair<size_t, size_t>> parent;
    std::unordered_map<size_t, bool> needs_destroy;
    auto hash = [&](size_t x, size_t y) { return y * board_state[0].size() + x; };

    pq.push({start_x, start_y, 0});
    dist[hash(start_x, start_y)] = 0;

    PathResult result;
    bool found = false;
    size_t end_x = start_x, end_y = start_y;

    while (!pq.empty()) {
        Node current = pq.top();
        pq.pop();

        if ((current.x == goal_x && current.y == goal_y) ||
            (path_max_length > 0 && current.cost >= static_cast<int>(path_max_length))) {
            end_x = current.x;
            end_y = current.y;
            found = true;
            break;
        }

        if (dist[hash(current.x, current.y)] < current.cost) continue;

        auto all_directions = DirectionUtil::allDirections();
        for (Direction dir : all_directions) {
            auto [dx, dy] = DirectionUtil::getMovement(dir);
            size_t nx = (current.x + dx + board_state[0].size()) % board_state[0].size();
            size_t ny = (current.y + dy + board_state.size()) % board_state.size();
            char cell = board_state[ny][nx];

            int move_cost = getShootingCost(cell);
            if (move_cost == INT_MAX) continue;

            int new_cost = current.cost + move_cost;
            size_t pos_hash = hash(nx, ny);

            if (!dist.count(pos_hash) || new_cost < dist[pos_hash]) {
                dist[pos_hash] = new_cost;
                parent[pos_hash] = {current.x, current.y};
                needs_destroy[pos_hash] = (cell == '#' || cell == '$');
                pq.push({nx, ny, new_cost});
            }
        }
    }

    if (found) {
        std::vector<std::pair<size_t, size_t>> raw_path;
        auto current = std::make_pair(end_x, end_y);
        while (current.first != start_x || current.second != start_y) {
            raw_path.push_back(current);
            current = parent[hash(current.first, current.second)];
        }
        std::reverse(raw_path.begin(), raw_path.end());
        result.path = raw_path;

        for (const auto& pos : result.path) {
            size_t pos_hash = hash(pos.first, pos.second);
            if (needs_destroy[pos_hash]) {
                result.destroy.push_back(pos);
            } else {
                result.destroy.push_back(std::nullopt);
            }
        }
    }

    return result;
}


REGISTER_PLAYER(OffensivePlayer); //it looks like its wrong, according to pdf, check and if so, correct one below:
//REGISTER_PLAYER(Player_322719139_211961057_A);

} // namespace Algorithm_322719139_211961057
