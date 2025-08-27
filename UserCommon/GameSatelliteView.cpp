#include "GameSatelliteView.h"
#include <cstdlib>
#include <cmath>
#include <iostream>

namespace UserCommon_322719139_211961057 {

// NEW full-board constructor
GameSatelliteView::GameSatelliteView(const std::vector<std::vector<char>>& board)
    : board_(board), requesting_tank_x_(0), requesting_tank_y_(0), requesting_player_index_(0) {}

GameSatelliteView::GameSatelliteView(const std::vector<std::vector<char>>& board,
                                     size_t tank_x, size_t tank_y, int player_index)
    : board_(board), requesting_tank_x_(tank_x), requesting_tank_y_(tank_y),
      requesting_player_index_(player_index) {}

GameSatelliteView::GameSatelliteView(const std::vector<std::vector<char>>& board,
                                     size_t tank_x, size_t tank_y, int player_index,int tank_idx)
    : board_(board), requesting_tank_x_(tank_x), requesting_tank_y_(tank_y),
      requesting_player_index_(player_index),requesting_tank_index_(tank_idx) {}
char GameSatelliteView::getObjectAt(size_t x, size_t y) const {
   // std::cout << "[DEBUG] Entered getObjectAt(x=" << x << ", y=" << y << ")\n";
    //printBoard();
    size_t num_rows = board_.size();
    if (num_rows == 0) {
        std::cout << "[ERROR] board_ is empty (board_.size() == 0)\n";
        return '&';
    }

    if (y >= num_rows) {
        std::cout << "[ERROR] y=" << y << " is out of bounds (rows=" << num_rows << ")\n";
        return '&';
    }

    size_t num_cols = board_[y].size();
    if (x >= num_cols) {
        std::cout << "[ERROR] x=" << x << " is out of bounds for row y=" << y << " (cols=" << num_cols << ")\n";
        return '&';
    }
    
    // If this is the requesting tank's position, return '%' add that player index 
    if (x == requesting_tank_x_ && y == requesting_tank_y_ ) {
        std::cout << "my Tank found\n";
        return '%';  // The tank which requested the info
    }

    char val = board_[y][x];
    //std:: cout << "[DEBUG] Entered getObjectAt " << val << "\n";

    //std::cout << "[DEBUG] board_[" << y << "][" << x << "] = '" << val << "'\n";
    return val;
}

void GameSatelliteView::printBoard() const {
    std::cout << "===== Current Board State =====\n";
    for (size_t y = 0; y < board_.size(); ++y) {
        for (size_t x = 0; x < board_[y].size(); ++x) {
            std::cout << board_[y][x];
        }
        std::cout << "\n";
    }
    std::cout << "===============================\n";
}

}  // namespace
