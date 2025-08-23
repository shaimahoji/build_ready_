#include "Direction.h"
namespace UserCommon_322719139_211961057 {
    std::pair<int, int> DirectionUtil::getMovement(Direction dir) {
        switch (dir) {
            case Direction::UP: return {0, -1};
            case Direction::UP_RIGHT: return {1, -1};
            case Direction::RIGHT: return {1, 0};
            case Direction::DOWN_RIGHT: return {1, 1};
            case Direction::DOWN: return {0, 1};
            case Direction::DOWN_LEFT: return {-1, 1};
            case Direction::LEFT: return {-1, 0};
            case Direction::UP_LEFT: return {-1, -1};
            default: return {0, 0};
        }
    }

    Direction DirectionUtil::rotateLeft(Direction dir) {
        switch (dir) {
            case Direction::UP: return Direction::UP_LEFT;
            case Direction::UP_RIGHT: return Direction::UP;
            case Direction::RIGHT: return Direction::UP_RIGHT;
            case Direction::DOWN_RIGHT: return Direction::RIGHT;
            case Direction::DOWN: return Direction::DOWN_RIGHT;
            case Direction::DOWN_LEFT: return Direction::DOWN;
            case Direction::LEFT: return Direction::DOWN_LEFT;
            case Direction::UP_LEFT: return Direction::LEFT;
            default: return dir;
        }
    }

    Direction DirectionUtil::rotateRight(Direction dir) {
        switch (dir) {
            case Direction::UP: return Direction::UP_RIGHT;
            case Direction::UP_RIGHT: return Direction::RIGHT;
            case Direction::RIGHT: return Direction::DOWN_RIGHT;
            case Direction::DOWN_RIGHT: return Direction::DOWN;
            case Direction::DOWN: return Direction::DOWN_LEFT;
            case Direction::DOWN_LEFT: return Direction::LEFT;
            case Direction::LEFT: return Direction::UP_LEFT;
            case Direction::UP_LEFT: return Direction::UP;
            default: return dir;
        }
    }

    Direction DirectionUtil::getOppositeDirection(Direction dir) {
        switch (dir) {
            case Direction::UP: return Direction::DOWN;
            case Direction::UP_RIGHT: return Direction::DOWN_LEFT;
            case Direction::RIGHT: return Direction::LEFT;
            case Direction::DOWN_RIGHT: return Direction::UP_LEFT;
            case Direction::DOWN: return Direction::UP;
            case Direction::DOWN_LEFT: return Direction::UP_RIGHT;
            case Direction::LEFT: return Direction::RIGHT;
            case Direction::UP_LEFT: return Direction::DOWN_RIGHT;
            default: return dir;
        }}

    Direction DirectionUtil::rotateLeftQuarter(Direction dir) {
        return rotateLeft(rotateLeft(dir));
    }

    Direction DirectionUtil::rotateRightQuarter(Direction dir) {
        return rotateRight(rotateRight(dir));
    }

    std::string DirectionUtil::toString(Direction dir) {
        switch (dir) {
            case Direction::UP: return "U";
            case Direction::UP_RIGHT: return "UR";
            case Direction::RIGHT: return "R";
            case Direction::DOWN_RIGHT: return "DR";
            case Direction::DOWN: return "D";
            case Direction::DOWN_LEFT: return "DL";
            case Direction::LEFT: return "L";
            case Direction::UP_LEFT: return "UL";
            default: return "";
        }
    }

    Direction DirectionUtil::fromString(const std::string& str) {
        if (str == "U") return Direction::UP;
        if (str == "UR") return Direction::UP_RIGHT;
        if (str == "R") return Direction::RIGHT;
        if (str == "DR") return Direction::DOWN_RIGHT;
        if (str == "D") return Direction::DOWN;
        if (str == "DL") return Direction::DOWN_LEFT;
        if (str == "L") return Direction::LEFT;
        if (str == "UL") return Direction::UP_LEFT;
        return Direction::UP; // default
    }

    Direction DirectionUtil::getDirectionFromDelta(int dx, int dy) {
        if (dx == 0 && dy < 0) return Direction::UP;
        if (dx > 0 && dy < 0) return Direction::UP_RIGHT;
        if (dx > 0 && dy == 0) return Direction::RIGHT;
        if (dx > 0 && dy > 0) return Direction::DOWN_RIGHT;
        if (dx == 0 && dy > 0) return Direction::DOWN;
        if (dx < 0 && dy > 0) return Direction::DOWN_LEFT;
        if (dx < 0 && dy == 0) return Direction::LEFT;
        if (dx < 0 && dy < 0) return Direction::UP_LEFT;
        return Direction::UP; // Default/fallback
    }

    std::string DirectionUtil::getActionToMove(Direction current_dir, int dx, int dy) {
        Direction target_dir = getDirectionFromDelta(dx, dy);

        if (current_dir == target_dir) {
            return "MOVE_FORWARD";
        }

        // Determine shortest rotation
        if (rotateLeft(current_dir) == target_dir) {
            return "ROTATE_LEFT";
        } else if (rotateRight(current_dir) == target_dir) {
            return "ROTATE_RIGHT";
        } else {
            // Not just one rotation away, so rotate left as default
            return "ROTATE_LEFT";
        }
    }

    std::vector<Direction> DirectionUtil::allDirections() {
        return {
            Direction::UP,
            Direction::LEFT,
            Direction::RIGHT,
            Direction::DOWN_LEFT,
            Direction::UP_LEFT,
            Direction::UP_RIGHT,
            Direction::DOWN_RIGHT,
            Direction::DOWN
        };
    }
} // namespace