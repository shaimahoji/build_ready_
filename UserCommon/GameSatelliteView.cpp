#include "GameSatelliteView.h"

namespace UserCommon_322719139_211961057 {
    GameSatelliteView::GameSatelliteView(const std::vector<std::vector<char>>& board, 
                                    size_t tank_x, size_t tank_y, int player_index)
        : board_(board),
        requesting_tank_x_(tank_x),
        requesting_tank_y_(tank_y),
        requesting_player_index_(player_index) {
    }

    char GameSatelliteView::getObjectAt(size_t x, size_t y) const {
        // Check if position is outside the board
        if (y >= board_.size() || x >= board_[0].size()) {
            return '&';  // Outside battlefield
        }
        
        // If this is the requesting tank's position, return '%'
        if (x == requesting_tank_x_ && y == requesting_tank_y_) {
            return '%';  // The tank which requested the info
        }
        
        // Otherwise, return the object at the position
        return board_[y][x];
    }
} // namespace