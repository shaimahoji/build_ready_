#pragma once

#include <vector>
#include <cstddef> // for size_t
#include <mutex>

#include "../common/SatelliteView.h"

namespace UserCommon_322719139_211961057 {
    class GameSatelliteView : public SatelliteView {
    private:
    
        std::vector<std::vector<char>> board_;

        size_t requesting_tank_x_;
        size_t requesting_tank_y_;
        int requesting_player_index_;

        size_t board_height_;
        size_t board_width_;
        
    public:
        explicit GameSatelliteView(const std::vector<std::vector<char>>& board);
        GameSatelliteView(const std::vector<std::vector<char>>& board, 
                        size_t tank_x, size_t tank_y, int player_index);
        ~GameSatelliteView() override = default;
        
        char getObjectAt(size_t x, size_t y) const override;
        char getObjectAt2(size_t x, size_t y) const;
        size_t getBoardWidth() const { return board_width_; };
        size_t getBoardHeight() const { return board_height_; };
    };
} // namespace