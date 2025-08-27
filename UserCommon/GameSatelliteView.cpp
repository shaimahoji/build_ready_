#include "GameSatelliteView.h"
#include <cstdlib>
#include <cmath>
#include <iostream>

namespace UserCommon_322719139_211961057 {

// NEW full-board constructor
GameSatelliteView::GameSatelliteView(const std::vector<std::vector<char>>& board)
    : board_(board), requesting_tank_x_(0), requesting_tank_y_(0), requesting_player_index_(0) {
        std::cout << "---------- GameSatelliteView Constructed by board ----------\n";
    }

    
GameSatelliteView::GameSatelliteView(const std::vector<std::vector<char>>& board,
                                     size_t tank_x, size_t tank_y, int player_index)
    : board_(board), requesting_tank_x_(tank_x), requesting_tank_y_(tank_y),
      requesting_player_index_(player_index) {
        std::cout << "---------- GameSatelliteView Constructed by values ----------\n";
      }


char GameSatelliteView::getObjectAt(size_t x, size_t y) const {
    std::cout << "[getObjectAt] called for values x,y: (" << x << "," << y << ")\n";
    if (y >= board_.size() || x >= board_[y].size()) {
        return '&';
    }

    if (x == requesting_tank_x_ && y == requesting_tank_y_) {
        std::cout << "[%] Injecting '%' for player " << requesting_player_index_
                  << " at (" << x << "," << y << ")\n";
        return '%';
    }

    return board_[y][x];
}

}  // namespace
