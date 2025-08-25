#pragma once

#include <memory>
#include <functional>
#include <string>
#include <vector>
#include <fstream>

// -------- Your custom GameSatelliteView --------
#include "../UserCommon/GameSatelliteView.h" // includes SatelliteView.h
#include "../UserCommon/Direction.h" // defines Direction and DirectionUtil

#include "../UserCommon/json.hpp"

// -------- Common headers (in correct dependency order) --------
#include "../common/ActionRequest.h"       // defines ActionRequest for TankAlgorithm
#include "../common/BattleInfo.h"         // defines BattleInfo for TankAlgorithm
#include "../common/TankAlgorithm.h"       
#include "../common/GameResult.h"          // defines GameResult
#include "../common/Player.h"              // defines Player
#include "../common/AbstractGameManager.h" // uses all of the above

// -------- JSON --------
#include "json.hpp"
using json = nlohmann::json;

namespace GameManager_322719139_211961057 {

using Direction = UserCommon_322719139_211961057::Direction;

// GameManager_refactored
class MyGameManager_322719139_211961057 : public AbstractGameManager {
public:
    // Properly using the factory function types
    explicit MyGameManager_322719139_211961057(bool verbose);
    ~MyGameManager_322719139_211961057();

    // Previously called run();
    void executeGameLoop();

    GameResult run(size_t map_width, size_t map_height,
               const SatelliteView& map,
               std::string map_name,
               size_t max_steps, size_t num_shells,
               Player& player1, std::string name1,
               Player& player2, std::string name2,
               TankAlgorithmFactory f1,
               TankAlgorithmFactory f2) override;

    void writeOutput(const std::string& filename);

    std::vector<std::pair<std::string, bool>> ordered_actions;

    //visual
    void setGameLogFile(const std::string &filename);
    void setVisualizationFile(const std::string &filename);
    void writeVisualizationState();
    

private:

    //attempt
    std::array<size_t, 2> tank_counts_;  // index 0 = player 1, index 1 = player 2

    bool output_ok_ = false;
    
    // Factories for creating players and tank algorithms
    //const PlayerFactory& player_factory_;
    //const TankAlgorithmFactory& tank_algorithm_factory_;

    // Game state
    size_t max_steps_;
    size_t num_shells_;
    size_t overall_shells;
    size_t rows_;
    size_t cols_;
    size_t current_step_;

    int no_shell_steps_count_ = 0;  // Counts consecutive steps with zero shells

    // Game board
    std::vector<std::vector<char>> board_;
    
    //visual
    std::ofstream game_log_stream_;
    std::ofstream visualization_stream_;
    std::string game_log_file_;
    std::string visualization_file_;
    std::vector<std::string> round_actions_log_;
    std::vector<json> current_round_actions_; // Use json for easier writeRoundStateJson compatibility


    // HW3: GameManager should NOT own players
    //std::vector<std::unique_ptr<Player>> players_;
    //std::vector<Player*> players_;
    //int dummy_players_[2];  // just for test
    std::vector<void*> players_;

    
    // Tank data (authoritative source)
    struct TankData {
        int player_index;
        int tank_index;
        size_t x;
        size_t y;
        Direction direction;
        size_t remaining_shells;
        bool in_backward_move = false;
        bool in_reverse_mode = false;
        int backward_move_counter;
        bool cooldown_wait;
        int cooldown_timer;  // For tracking shooting cooldown
        bool is_alive;
        
        TankData() = default;  // Default constructor for Tank struct
        TankData(int p_idx, int t_idx, size_t x, size_t y, Direction dir, size_t shells)
            : player_index(p_idx), tank_index(t_idx), x(x), y(y), direction(dir),
            remaining_shells(shells), in_backward_move(false), in_reverse_mode(false),
            backward_move_counter(0), cooldown_wait(false), cooldown_timer(0), is_alive(true) {}

    };
    
    // Unified Tank struct with composition
    struct Tank {
        TankData data;
        std::unique_ptr<TankAlgorithm> algorithm;
        
        // Default constructor
        Tank() = default;
        
        // Constructor with parameters
        Tank(TankData tank_data, std::unique_ptr<TankAlgorithm> tank_algorithm)
            : data(std::move(tank_data)), algorithm(std::move(tank_algorithm)) {}
        
        // Move constructor
        Tank(Tank&& other) noexcept
            : data(std::move(other.data)), algorithm(std::move(other.algorithm)) {}
        
        // Move assignment operator
        Tank& operator=(Tank&& other) noexcept {
            if (this != &other) {
                data = std::move(other.data);
                algorithm = std::move(other.algorithm);
            }
            return *this;
        }
        
        // Delete copy constructor and copy assignment to ensure unique ownership
        Tank(const Tank&) = delete;
        Tank& operator=(const Tank&) = delete;
    };
    
    // Unified tank storage - replaces parallel containers
    std::vector<Tank> tanks_;
    
    // Shell data
    struct ShellData {
        size_t x;
        size_t y;
        Direction direction;
        ShellData(size_t x, size_t y, Direction dir)
            : x(x), y(y), direction(dir){}
    };
    
    // Wall tracking
    struct WallInfo {
        size_t x;
        size_t y;
        int health;  //  health (2 = new, 1 = damaged, 0 = destroyed)        
        WallInfo(size_t x, size_t y,int health)
            : x(x), y(y),health(health) {}
        
        bool operator==(const WallInfo& other) const {
            return x == other.x && y == other.y;
        }
    };

    // Mines tracking
    struct MineInfo {
        size_t x;
        size_t y;       
        MineInfo(size_t x, size_t y)
            : x(x), y(y){}
        bool operator==(const MineInfo& other) const {
            return x == other.x && y == other.y;
        }
    };

    std::vector<WallInfo> walls_;
    std::vector<MineInfo> mines_;
    std::vector<ShellData> shells_;
    
    /* Output file and stream */
    std::string output_file_;
    std::ofstream output_stream_;
 
    /* Helper methods */
    void initializePlayers();
    void initializeTanks();
    void processGameStep();
    bool isGameOver();
    bool isValidCellChar(char c) const;
    bool CanMoveBackward(MyGameManager_322719139_211961057::TankData *tank_data);
    void updateTankCounters(MyGameManager_322719139_211961057::TankData *tank_data);
    void addShell(size_t x,size_t y,Direction dir);
    void moveShells();
    bool checkShellCollision(size_t shell_idx, std::vector<size_t>& shells_to_remove);
    void handleBattleInfoRequest(int player_idx, int tank_idx);
    void initializeBoard();

    /* Helper methods for reading board */
    void parseHeaderLine(const std::string& line, const std::string& key, size_t& out);
    void processBoardChar(char& c, size_t col, size_t row,
                    std::map<int, std::vector<std::pair<size_t, size_t>>>& player_tank_positions,
                    std::string& warnings, bool& has_warnings);
    void writeWarningsToFile(const std::string& warnings);
    //void initializePlayersAndTanks(const std::map<int, std::vector<std::pair<size_t, size_t>>>& player_tank_positions);

    void checkCollisions();
    void updateBoard();
    void processAction(int player_idx, int tank_idx, ActionRequest action);
    void logAction(int player_idx, int tank_idx, ActionRequest action, bool is_valid);
    void logAction2(int tank_idx, ActionRequest action, bool is_valid);
    void logGameResult(const std::string &result, const std::string &reason);
    void logFinalResult();

    void displayBoard() const;
    void logToFile();
    void writeRoundStateJson(std::ofstream &out, int round_number);

    /* methods for processing actions */
    bool processBackwardMove(TankData* tank_data, int player_idx, int tank_idx,int index, ActionRequest action);
    bool processMoveForward(TankData* tank_data);
    bool processMoveBackward(TankData* tank_data, int player_idx, int tank_idx,int index);
    void processRotation(TankData* tank_data, ActionRequest action);
    bool processShoot(TankData* tank_data);
    void updateShellsStepCount();

    void readBoardFromSatellite(
    size_t map_width,
    size_t map_height,
    const SatelliteView& map,
    TankAlgorithmFactory player1_tank_algo_factory,
    TankAlgorithmFactory player2_tank_algo_factory);
    
    void initializeTanks(
    const std::map<int, std::vector<std::pair<size_t, size_t>>>& player_tank_positions,
    TankAlgorithmFactory player1_tank_algo_factory,
    TankAlgorithmFactory player2_tank_algo_factory);
};

std::string toString2(Direction dir);

} // namespace