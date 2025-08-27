#include "GameSatelliteView.h"
#include <cstdlib>
#include <cmath>
#include <iostream>

namespace UserCommon_322719139_211961057 {

// NEW full-board constructor
GameSatelliteView::GameSatelliteView(const std::vector<std::vector<char>>& board)
    : board_(board), requesting_tank_x_(0), requesting_tank_y_(0), requesting_player_index_(0), board_height_(board.size()), board_width_((board.empty()) ? 0 : board[0].size()) {
        std::cout << "---------- GameSatelliteView Constructed by board ----------\n";
        std::cout << "[DEBUG] GameSatelliteView constructed from board. Dimensions: "
          << board_width_ << "x" << board_height_ << std::endl;


    }

    
GameSatelliteView::GameSatelliteView(const std::vector<std::vector<char>>& board,
                                     size_t tank_x, size_t tank_y, int player_index)
    : board_(board),
      requesting_tank_x_(tank_x),
      requesting_tank_y_(tank_y),
      requesting_player_index_(player_index),
      board_height_(board.size()),
      board_width_(board.empty() ? 0 : board[0].size()) {
        std::cout << "---------- GameSatelliteView Constructed by values ----------\n";
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


}  // namespace
