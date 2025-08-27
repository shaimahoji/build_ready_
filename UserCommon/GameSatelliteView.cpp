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
    // Optional: if running with multiple threads, guard std::cout
    static std::mutex cout_mutex;
    std::lock_guard<std::mutex> lock(cout_mutex);

    //std::cout << "[DEBUG] Entered getObjectAt(x=" << x << ", y=" << y << ")\n";

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
    if (num_cols == 0) {
        std::cout << "[ERROR] board_[" << y << "] is empty (cols = 0)\n";
        return '&';
    }

    if (x >= num_cols) {
        std::cout << "[ERROR] x=" << x << " is out of bounds for row y=" << y << " (cols=" << num_cols << ")\n";
        return '&';
    }

    char val = board_[y][x];
    //std::cout << "[DEBUG] board_[" << y << "][" << x << "] = '" << val << "'\n";
    std::cout << "[DEBUG] Returning from getObjectAt()\n";
    return val;
}



}  // namespace
