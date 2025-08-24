#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <map>
#include <cmath>

#include "GameManager.h"

#include "../common/AbstractGameManager.h"  // defines GameManagerFactory
#include "../common/GameManagerRegistration.h"

using namespace UserCommon_322719139_211961057;

// Static initializer object that prints during global/static init
struct _StartupDebugPrinter {
    _StartupDebugPrinter() {
        std::cout << "[DEBUG] very top of GameManager.cpp\n";
    }
};

// This runs before main, even before your GameManager constructor
static _StartupDebugPrinter _startup_debug_printer;
// --------------------

namespace GameManager_322719139_211961057 {

GameManager_322719139_211961057_A::GameManager_322719139_211961057_A(bool verbose)
    : max_steps_(5000), // The following are initial values, they're updated in readBoard
      num_shells_(16),
      overall_shells(0),
      rows_(0),
      cols_(0),
      current_step_(0) {
    
    std::cout << "[DEBUG] GameManager constructor start\n";

    this->players_.resize(2); // if this crashes, we'll see it
    std::cout << "[DEBUG] players_ resized\n";

    if (verbose) {
        std::cout << "[GameManager] Verbose mode enabled.\n";
    }

    std::cout << "[DEBUG] GameManager constructor end\n";
}

GameManager_322719139_211961057_A::~GameManager_322719139_211961057_A() {
    if (output_stream_.is_open()) {
        output_stream_.close();
    }
    if (game_log_stream_.is_open()) {
        game_log_stream_.close();
    }
    if (visualization_stream_.is_open()) {
        visualization_stream_.close();
    }
}

/**************************/
/* initializing the Game  */
/**************************/

void GameManager_322719139_211961057_A::readBoardFromSatellite(
    size_t map_width,
    size_t map_height,
    const SatelliteView& map,
    TankAlgorithmFactory player1_tank_algo_factory,
    TankAlgorithmFactory player2_tank_algo_factory
) {
    rows_ = map_height;
    cols_ = map_width;
    board_.resize(rows_, std::vector<char>(cols_, ' '));

    std::map<int, std::vector<std::pair<size_t, size_t>>> player_tank_positions;

    std::string warnings;
    bool has_warnings = false;

    for (size_t y = 0; y < rows_; ++y) {
        for (size_t x = 0; x < cols_; ++x) {
            char c = map.getObjectAt(x, y);
            processBoardChar(c, x, y, player_tank_positions, warnings, has_warnings);
        }
    }

    if (has_warnings && !warnings.empty()) {
        writeWarningsToFile(warnings);
    }

    tank_counts_[0] = player_tank_positions[1].size();  // Player 1 is key 1
    tank_counts_[1] = player_tank_positions[2].size();  // Player 2 is key 2
    
    initializeTanks(player_tank_positions, player1_tank_algo_factory, player2_tank_algo_factory);
}

//HW2 implementation so far : the one i want to keep
// Assuming that the players and factories have been initialized in run(...),
// this function initializes tanks based on the provided tank positions.
void GameManager_322719139_211961057_A::initializeTanks(
    const std::map<int, std::vector<std::pair<size_t, size_t>>>& player_tank_positions,
    TankAlgorithmFactory player1_tank_algo_factory,
    TankAlgorithmFactory player2_tank_algo_factory
) {

    std::size_t total_tanks = 0;
    
    // Create tanks in spawn order (row by row, left to right)
    // First collect all tank positions with their spawn order
    std::vector<std::tuple<size_t, size_t, int, int>> all_tank_positions; // x, y, player_idx, tank_idx
    
    for (const auto& [player_idx, positions] : player_tank_positions) {
        if (!positions.empty()) {
            
            for (size_t i = 0; i < positions.size(); ++i) {
                const auto& [tank_x, tank_y] = positions[i];
                all_tank_positions.emplace_back(tank_x, tank_y, player_idx, static_cast<int>(i));
                total_tanks++;
            }
        }
    }
    
    // Sort by spawn order (row by row, left to right)
    std::sort(all_tank_positions.begin(), all_tank_positions.end(),
        [](const auto& a, const auto& b) {
            size_t y_a = std::get<1>(a), x_a = std::get<0>(a);
            size_t y_b = std::get<1>(b), x_b = std::get<0>(b);
            if (y_a != y_b) return y_a < y_b;  // Row by row
            return x_a < x_b;  // Left to right within row
        });
    
    // Create tanks in the sorted order using std::make_unique
    tanks_.reserve(total_tanks);
    for (const auto& [tank_x, tank_y, player_idx, tank_idx] : all_tank_positions) {
        // Create tank data
        Direction direction = (player_idx == 1) ? Direction::LEFT : Direction::RIGHT;
        std::cout << "----------- Creating tank -----------" << std::endl;
        std::cout << "player << " << player_idx 
                  << ", tank_idx: " << tank_idx 
                  << ", tank_x: " << tank_x 
                  << ", tank_y: " << tank_y 
                  << ", direction: " << DirectionUtil::toString(direction) 
                  << ", num_shells_: " << num_shells_ << std::endl; //which presents remaining shells
        TankData tank_data(player_idx, tank_idx, tank_x, tank_y, direction, num_shells_);
        
        // Create tank algorithm using factory (which should use std::make_unique internally)
        std::unique_ptr<TankAlgorithm> tank_algorithm;

        if (player_idx == 1) {
            tank_algorithm = player1_tank_algo_factory(player_idx, tank_idx);
        } else { // player_idx == 2
            tank_algorithm = player2_tank_algo_factory(player_idx, tank_idx);
        }

        // Create and add the tank
        tanks_.emplace_back(std::move(tank_data), std::move(tank_algorithm));
    }
    
    overall_shells = total_tanks * num_shells_;
    ordered_actions.resize(tanks_.size(), {"UNKNOWN", false}); // previously my_vector
}

/********* Helper functions to make readBoard more concise and readable ************/
void GameManager_322719139_211961057_A::parseHeaderLine(const std::string& line, const std::string& key, size_t& out) {
    if (line.find(key) != std::string::npos) {
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string value = line.substr(pos + 1);
            try { out = std::stoul(value); }
            catch (...) {
                std::cerr << "Error: Bad value for " << key << " in header\n";
                out = 0;
            }
            //std::cout << "Parsed " << key << ": " << out << std::endl;
        }
    }
}

void GameManager_322719139_211961057_A::processBoardChar(char& c, size_t col, size_t row,
                                   std::map<int, std::vector<std::pair<size_t, size_t>>>& player_tank_positions,
                                   std::string& warnings, bool& has_warnings) {
    if (!isValidCellChar(c)) {
        warnings += "Warning: Invalid character '" + std::string(1, c) +
                    "' at (" + std::to_string(col) + "," + std::to_string(row) +
                    "), treated as space\n";
        c = ' ';
        has_warnings = true;
    }

    switch (c) {
        case '1':
        case '2': {
            int player_idx = c - '0';
            player_tank_positions[player_idx].emplace_back(col, row);
            break;
        }
        case '#': {
            board_[row][col] = c;
            walls_.emplace_back(col, row, 2);
            break;
        }
        case '@': {
            board_[row][col] = c;
            mines_.emplace_back(col, row);
            break;
        }
        default: {
            board_[row][col] = c;
            break;
        }
    }
}

void GameManager_322719139_211961057_A::writeWarningsToFile(const std::string& warnings) {
    std::ofstream error_file("input_errors.txt");
    if (error_file.is_open()) {
        error_file << warnings;
    }
    else {
    std::cerr << "Warning: Cannot write to input_errors.txt, printing warnings to screen\n";
    std::cerr << warnings;
    }
}

/*********************/
/* Running the Game  */
/*********************/
/* ---------- overridden from AbstractGameManager ---------- */
GameResult GameManager_322719139_211961057_A::run(
    size_t map_width, size_t map_height,
    const SatelliteView& map, std::string map_name,
    size_t max_steps, size_t num_shells,
    Player& player1, std::string name1,
    Player& player2, std::string name2,
    TankAlgorithmFactory player1_tank_algo_factory,
    TankAlgorithmFactory player2_tank_algo_factory)
{
    // Step 1: Store metadata
    max_steps_ = max_steps;
    num_shells_ = num_shells;

    // Step 2: Initialize players
    //floating attempt
    //players_.resize(2);
    //players_[0] = &player1;
    //players_[1] = &player2;

    this->players_.resize(2);
    this->players_[0] = static_cast<void*>(&player1);
    this->players_[1] = static_cast<void*>(&player2);


    // Step 3: Read board from SatelliteView + Initialize tanks by calling initializePlayersAndTanks
    readBoardFromSatellite(map_width, map_height, map, player1_tank_algo_factory, player2_tank_algo_factory);    

    // Step 4: Update overlays and run game loop
    updateBoard(); //??
    executeGameLoop();

    // Step 5: Final result computation
    GameResult result;
    result.rounds = current_step_;

    int player1_alive = 0, player2_alive = 0;
    for (const auto& tank : tanks_) {
        if (tank.data.is_alive) {
            if (tank.data.player_index == 1)
                player1_alive++;
            else
                player2_alive++;
        }
    }

    if (player1_alive > 0 && player2_alive == 0)
        result.winner = 1;
    else if (player2_alive > 0 && player1_alive == 0)
        result.winner = 2;
    else
        result.winner = 0;

    if (player1_alive == 0 && player2_alive == 0)
        result.reason = GameResult::Reason::ALL_TANKS_DEAD;
    else if (current_step_ >= max_steps_)
        result.reason = GameResult::Reason::MAX_STEPS;
    else
        result.reason = GameResult::Reason::ZERO_SHELLS;
    
        // Count alive tanks for player 1
    size_t alive_p1 = std::count_if(
        tanks_.begin(),
        tanks_.end(),
        [](const Tank& tank) {
            return tank.data.is_alive && tank.data.player_index == 1;
        }
    );

    // Count alive tanks for player 2
    size_t alive_p2 = std::count_if(
        tanks_.begin(),
        tanks_.end(),
        [](const Tank& tank) {
            return tank.data.is_alive && tank.data.player_index == 2;
        }
    );

    result.remaining_tanks = {alive_p1, alive_p2};

    result.gameState = std::make_unique<GameSatelliteView>(board_, 0, 0, 0);

    return result;
}


void GameManager_322719139_211961057_A::executeGameLoop() {
    // Open output files
    output_stream_.open(output_file_);
    game_log_stream_.open(game_log_file_);
    visualization_stream_.open(visualization_file_);

    output_ok_ = output_stream_.is_open();
    if (!output_ok_ ) {
        std::cerr << "Error: Could not open output file " << output_file_ << " — results will be printed to stdout.\n";
    }

    // Redirect to std::cout when file is unavailable.
    //if (output_ok_) output_stream_ << result;
    //else std::cout << result;


    initializeBoard();
    updateBoard();
    
    // Main game loop
    while (!isGameOver() && current_step_ < max_steps_) {
        std::cout << "\n Main game loop entered.\n";
        processGameStep();
        updateShellsStepCount();
        logToFile();
        current_step_++;
        std::cout << "\nStep " << current_step_ << ":\n";
    }
    
    // Log final result
    logFinalResult();
    
    // Close output file
    output_stream_.close();
}

void GameManager_322719139_211961057_A::updateShellsStepCount() {
    if (overall_shells == 0) {
        no_shell_steps_count_++;
    } else {
        no_shell_steps_count_ = 0;
    }
}

void GameManager_322719139_211961057_A::initializeBoard() {
    board_.resize(rows_);
    for (auto& row : board_) {
        row.resize(cols_, ' ');
    }
}

void GameManager_322719139_211961057_A::updateBoard() {
    // Clear the board
    for (auto& row : board_) {
        std::fill(row.begin(), row.end(), ' ');
    }
    
    // Place walls 
    for (size_t i = 0; i < walls_.size(); ) {
        if (walls_[i].health == 2) {
            board_[walls_[i].y][walls_[i].x] = '#';
            ++i;
        }
        else if (walls_[i].health == 1) {
            board_[walls_[i].y][walls_[i].x] = '$';
            ++i;
        }
        else {
            // Remove walls with health <= 0 safely
            walls_[i] = walls_.back();
            walls_.pop_back();
        }
    }
    
    //Place mines
    for (const auto& mine : mines_) {
        board_[mine.y][mine.x] = '@';
    }
    
    // Place tanks using unified tank storage
    for (const auto& tank : tanks_) {
        if (tank.data.is_alive) {
            board_[tank.data.y][tank.data.x] = '0' + tank.data.player_index;
        }
    }
    
    // Place shells
    for (const auto& shell : shells_) {
        board_[shell.y][shell.x] = '*';
    }
}

/*************/
/* Printing  */
/*************/
void GameManager_322719139_211961057_A::writeOutput(const std::string& filename) {
    output_file_ = filename;
}

void GameManager_322719139_211961057_A::logAction(int player_idx, int tank_idx, ActionRequest action, bool is_valid) {
    // Convert action to string
    std::string action_str;
    switch (action) {
        case ActionRequest::MoveForward: action_str = "Move Forward"; break;
        case ActionRequest::MoveBackward: action_str = "Move Backward"; break;
        case ActionRequest::RotateLeft45: action_str = "Rotate Left 45"; break;
        case ActionRequest::RotateRight45: action_str = "Rotate Right 45"; break;
        case ActionRequest::RotateLeft90: action_str = "Rotate Left 90"; break;
        case ActionRequest::RotateRight90: action_str = "Rotate Right 90"; break;
        case ActionRequest::Shoot: action_str = "Shoot"; break;
        case ActionRequest::GetBattleInfo: action_str = "Get Battle Info"; break;
        case ActionRequest::DoNothing: action_str = "Do Nothing"; break;
        default: action_str = "Unknown Action"; break;
    }
    
    // Log to visualization
    if (visualization_stream_.is_open()) {
        json action_entry;
        action_entry["step"] = current_step_;
        action_entry["player"] = player_idx;
        action_entry["tank"] = tank_idx;
        action_entry["action"] = action_str;
        action_entry["valid"] = is_valid;
        current_round_actions_.push_back(action_entry);
    }
}


void GameManager_322719139_211961057_A::logAction2(int tank_idx, ActionRequest action, bool is_valid) {
    // Convert action to string
    std::string action_str;
    switch (action) {
        case ActionRequest::MoveForward: action_str = "Move Forward"; break;
        case ActionRequest::MoveBackward: action_str = "Move Backward"; break;
        case ActionRequest::RotateLeft45: action_str = "Rotate Left 45"; break;
        case ActionRequest::RotateRight45: action_str = "Rotate Right 45"; break;
        case ActionRequest::RotateLeft90: action_str = "Rotate Left 90"; break;
        case ActionRequest::RotateRight90: action_str = "Rotate Right 90"; break;
        case ActionRequest::Shoot: action_str = "Shoot"; break;
        case ActionRequest::GetBattleInfo: action_str = "Get Battle Info"; break;
        case ActionRequest::DoNothing: action_str = "Do Nothing"; break;
        default: action_str = "Unknown Action"; break;
    }
    
    // Log the action
    ordered_actions[tank_idx] = std::make_pair(action_str, is_valid);
}

void GameManager_322719139_211961057_A::logGameResult(const std::string& result, const std::string& reason) {
    if (!output_stream_.is_open()) {
        return;
    }
    
    output_stream_ << "\nGame Result: " << result << "\n";
    output_stream_ << "Reason: " << reason << "\n";
    output_stream_ << "Total Steps: " << current_step_ << "\n";
}

void GameManager_322719139_211961057_A::logFinalResult() {
    // For graceful degradation.
    std::ostream& out = (output_ok_ && output_stream_.is_open()) ? output_stream_ : std::cout;
 
    // Count alive tanks for each player
    int player1_alive = 0;
    int player2_alive = 0;
    
    for (const auto& tank : tanks_) {
        if (tank.data.is_alive) {
            if (tank.data.player_index == 1) {
                player1_alive++;
            } else if (tank.data.player_index == 2) {
                player2_alive++;
            }
        }
    }
    const int no_shell_steps_threshold = 40; //??? magic number, should be a class member
    std::string result;

    // Determine game result according to specified format
    if (no_shell_steps_count_ >= no_shell_steps_threshold) {
        result = "Tie, both players have zero shells for " + std::to_string(no_shell_steps_threshold) + " steps";
    } else if (player1_alive > 0 && player2_alive == 0) {
        result = "Player 1 won with " + std::to_string(player1_alive) + " tanks still alive";
    } else if (player2_alive > 0 && player1_alive == 0) {
        result = "Player 2 won with " + std::to_string(player2_alive) + " tanks still alive";
    } else if (player1_alive == 0 && player2_alive == 0) {
        result = "Tie, both players have zero tanks";
    } else if (current_step_ >= max_steps_) {
        result = "Tie, reached max steps = " + std::to_string(max_steps_) + 
                 ", player 1 has " + std::to_string(player1_alive) + 
                 " tanks, player 2 has " + std::to_string(player2_alive) + " tanks";
    } else {
        // Check for shell exhaustion tie
        bool all_out_of_shells = true;
        for (const auto& tank : tanks_) {
            if (tank.data.is_alive && tank.data.remaining_shells > 0) {
                all_out_of_shells = false;
                break;
            }
        }
        
        if (all_out_of_shells) {
            // Use a configurable variable instead of hardcoding 40
            const int no_shell_steps_threshold = 40; // This should be a class member
            result = "Tie, both players have zero shells for " + 
                    std::to_string(no_shell_steps_threshold) + " steps";
        } else {
            result = "Game ended - unknown condition";
            std::cerr << "Warning: Game ended in unexpected state — no clear winner or reason.\n";
        }
    }
    
    out << result << std::endl;

    // FINAL FALLBACK (added at the end)
    if (result == "Game ended - unknown condition" &&
        current_step_ < max_steps_ &&
        (player1_alive > 0 || player2_alive > 0)) {
        
        std::string fallback = "Game ended unexpectedly.";
        std::cerr << "Fallback: " << fallback << std::endl;
        out << fallback << std::endl;
    }
}

void GameManager_322719139_211961057_A::displayBoard() const {
    for (const auto& row : board_) {
        for (char cell : row) {
            std::cout << cell;
        }
        std::cout << '\n';
    }
}

void GameManager_322719139_211961057_A::logToFile() {
    std::ostream& out = (output_ok_ && output_stream_.is_open()) ? output_stream_ : std::cout;

    for (size_t i = 0; i < ordered_actions.size(); ++i) {
        const auto& [action_str, is_valid] = ordered_actions[i];
        const auto& tank = tanks_[i];

        if (tank.data.is_alive) {
            out << action_str;
            if (!is_valid) out << " (ignored)";
        } else {
            out << action_str << " (killed)";
        }

        if (i + 1 < ordered_actions.size()) out << ", "; //??? < or !=
    }

    out << '\n';
}

/*************************/
/* processing Game step */
/*************************/
void GameManager_322719139_211961057_A::processGameStep() {
    std::cout << "Processing game step " << current_step_ << "\n";
    
    // Process each tank in the unified tanks_ vector (maintains spawn order)
    for (size_t tank_index = 0; tank_index < tanks_.size(); ++tank_index) {
        Tank& tank = tanks_[tank_index];
        
        // Skip logic for tanks already marked 'killed'
        if (!tank.data.is_alive) {
            continue;
        }
        
        std::cout << "Processing tank " << tank.data.tank_index << " for player " << tank.data.player_index << "\n";
        
        if (!tank.algorithm) {
            std::cerr << "Error: Tank algorithm is null for player " << tank.data.player_index
                      << ", tank index " << tank.data.tank_index << "\n";
            continue; // skip this tank to avoid crashing
        }

        // 1. Call the algorithm->decideAction() method
        ActionRequest action = tank.algorithm->getAction();
        std::cout << "Processing action: " << static_cast<int>(action) << "\n";
        
        // Handle battle info requests separately
        if (action == ActionRequest::GetBattleInfo) {
            std::cout << "Handling getBattleInfo action\n";
            handleBattleInfoRequest(tank.data.player_index, tank.data.tank_index);
            continue;
        }
        
        // 2. Validate and apply the action using the data reference
        // 3. Record the result (including (ignored) or (killed) if applicable)
        processAction(tank.data.player_index, tank.data.tank_index, action);
    }
    
    // Move shells and check collisions
    moveShells();
    checkCollisions();
    
    // Update the board representation
    updateBoard();
      
    // Write visualization state
    writeVisualizationState();
}

void GameManager_322719139_211961057_A::handleBattleInfoRequest(int player_idx, int tank_idx) {
    // Find the tank in the unified storage
    Tank* tank = nullptr;
    int index = 0;
    for (auto& t : tanks_) {
        if (t.data.player_index == player_idx && t.data.tank_index == tank_idx) {
            tank = &t;
            break;
        }
        index++;
    }
    
    if (!tank || !tank->data.is_alive) {
        logAction2(index, ActionRequest::GetBattleInfo, false);
        logAction(player_idx, tank_idx, ActionRequest::GetBattleInfo, false);
        return;
    }
    
    // debugging to find error
    //floating attempt
    //if (!players_[player_idx - 1]) {
    if (static_cast<Player*>(this->players_[player_idx - 1])) {
    //if (!this->players_[player_idx - 1]) {
        std::cerr << "Error: players_[" << (player_idx - 1) << "] is nullptr!\n";
        return;
    }
    if (!tank->algorithm) {
        std::cerr << "Error: tank algorithm is nullptr!\n";
        return;
    }

    if (tank->data.y >= board_.size() || tank->data.x >= board_[tank->data.y].size()) {
        std::cerr << "Tank position out of bounds: x=" << tank->data.x << ", y=" << tank->data.y << "\n";
        return;
    }
    
    // Create satellite view
    GameSatelliteView satellite_view(board_, tank->data.x, tank->data.y, player_idx);

    std::cout << "Calling updateTankWithBattleInfo for player " << player_idx 
          << ", tank " << tank_idx << "\n";
          
    // Update the tank with battle info
    //floating attempt
    //players_[player_idx-1]->updateTankWithBattleInfo(
    //this->players_[player_idx-1]->updateTankWithBattleInfo(
    static_cast<Player*>(this->players_[player_idx - 1])->updateTankWithBattleInfo(
        *tank->algorithm, 
        satellite_view
    );
    
    // Log the action
    logAction2(index, ActionRequest::GetBattleInfo, true);
    logAction(player_idx, tank_idx, ActionRequest::GetBattleInfo, true);
    std::cout << "finished handleBattleInfoRequest\n";
}

void GameManager_322719139_211961057_A::processAction(int player_idx, int tank_idx, ActionRequest action) {
    // Find the tank data in the unified storage
    TankData* tank_data = nullptr;
    int index = 0;
    for (auto& tank : tanks_) {
        if (tank.data.player_index == player_idx && tank.data.tank_index == tank_idx) {
            tank_data = &tank.data;
            break;
        }
        index++;
    }

    if (!tank_data || !tank_data->is_alive) {
        logAction2(index, action, false);
        logAction(player_idx, tank_idx, action, false);
        return;
    }

    bool is_valid = true;

    // Handle reverse mode exit condition
    if (tank_data->in_reverse_mode && action != ActionRequest::MoveBackward) {
        tank_data->in_reverse_mode = false;
    }

    // Handle backward move sequence
    if (tank_data->in_backward_move) {
        is_valid = processBackwardMove(tank_data, player_idx, tank_idx,index, action);
        if (is_valid) return; // Action was handled in the backward move logic
    }

    // Process normal actions
    switch (action) {
        case ActionRequest::MoveForward:
            is_valid = processMoveForward(tank_data);
            break;
            
        case ActionRequest::MoveBackward:
            is_valid = processMoveBackward(tank_data, player_idx, tank_idx,index);
            if (is_valid) return; // Action was fully handled
            break;
            
        case ActionRequest::RotateLeft45:
        case ActionRequest::RotateRight45:
        case ActionRequest::RotateLeft90:
        case ActionRequest::RotateRight90:
            if (!tank_data->in_backward_move) {
                processRotation(tank_data, action);
                is_valid =true; //rotation alwayes valid
            }
            break;
            
        case ActionRequest::Shoot:
            std::cout << "Processing shoot action for player " << player_idx 
                      << ", tank " << tank_idx << "\n";
            is_valid = processShoot(tank_data);
            break;
            
        case ActionRequest::DoNothing:
            is_valid = true;
            break;
            
        default:
            is_valid = false;
            break;
    }

    // Update tank counters
    updateTankCounters(tank_data);
    
    // Log the action with validation result
    logAction2(index,action,is_valid);
    logAction(player_idx, tank_idx, action, is_valid);
}

//if we should ignore actions or cancel the current backward action
bool GameManager_322719139_211961057_A::processBackwardMove(TankData* tank_data, int player_idx, int tank_idx,int index, ActionRequest action) {
    // Handle forward move (cancel countdown)
    if (action == ActionRequest::MoveForward) {
        tank_data->in_backward_move = false;
        tank_data->backward_move_counter = 0;
        logAction2(index,action,true);
        logAction(player_idx, tank_idx, action, true);
        return true;
    }

    // Countdown not reached
    if (tank_data->backward_move_counter < 2) {
        tank_data->backward_move_counter++;
        logAction2(index,action,true);
        logAction(player_idx, tank_idx, action, true);
        return true;
    }

    // Third step: perform move
    if (action == ActionRequest::MoveBackward) {
        auto [dx, dy] = DirectionUtil::getMovement(tank_data->direction);
        size_t new_x = (tank_data->x - dx + cols_) % cols_;
        size_t new_y = (tank_data->y - dy + rows_) % rows_;

        bool can_move = (board_[new_y][new_x] != '#' && board_[new_y][new_x] != '$');
        if (can_move) {
            tank_data->x = new_x;
            tank_data->y = new_y;
            tank_data->in_reverse_mode = true;
        }

        // Reset countdown regardless of success
        tank_data->in_backward_move = false;
        tank_data->backward_move_counter = 0;

        updateTankCounters(tank_data);
        logAction2(index,action,can_move);
        logAction(player_idx, tank_idx, action, can_move);
        return true;
    }

    // Ignore all other actions during countdown
    logAction2(index, action, true);
    logAction(player_idx, tank_idx, action, true);
    return true;
}

bool GameManager_322719139_211961057_A::processMoveForward(TankData* tank_data) {
    auto [dx, dy] = DirectionUtil::getMovement(tank_data->direction);
    size_t new_x = (tank_data->x + dx + cols_) % cols_;
    size_t new_y = (tank_data->y + dy + rows_) % rows_;

    // Fixed logic error: changed || to && (original condition was always true)
    if (board_[new_y][new_x] != '#' && board_[new_y][new_x] != '$') {
        tank_data->x = new_x;
        tank_data->y = new_y;
        return true;
    }
    return false;
}

//if we are in reverse mode -or want to start counting for backward move
bool GameManager_322719139_211961057_A::processMoveBackward(TankData* tank_data, int player_idx, int tank_idx,int index) {
    if (tank_data->in_reverse_mode) {
        auto [dx, dy] = DirectionUtil::getMovement(tank_data->direction);
        size_t new_x = (tank_data->x - dx + cols_) % cols_;
        size_t new_y = (tank_data->y - dy + rows_) % rows_;

        if (board_[new_y][new_x] != '#' && board_[new_y][new_x] != '$') {
            tank_data->x = new_x;
            tank_data->y = new_y;
            return true;
        }
        return false;
    } else {
        // Start backward countdown
        tank_data->in_backward_move = true;
        tank_data->backward_move_counter = 0;
        logAction2(index, ActionRequest::MoveBackward, true);
        logAction(player_idx, tank_idx, ActionRequest::MoveBackward, true);
        return true;
    }
}

void GameManager_322719139_211961057_A::processRotation(TankData* tank_data, ActionRequest action) {
    switch (action) {
        case ActionRequest::RotateLeft45:
            tank_data->direction = DirectionUtil::rotateLeft(tank_data->direction);
            break;
        case ActionRequest::RotateRight45:
            tank_data->direction = DirectionUtil::rotateRight(tank_data->direction);
            break;
        case ActionRequest::RotateLeft90:
            tank_data->direction = DirectionUtil::rotateLeftQuarter(tank_data->direction);
            break;
        case ActionRequest::RotateRight90:
            tank_data->direction = DirectionUtil::rotateRightQuarter(tank_data->direction);
            break;
        default:
            break;
    }
}

bool GameManager_322719139_211961057_A::processShoot(TankData* tank_data) {
    std::cout << "---------- processShoot ----------\n";
    if (tank_data->cooldown_wait || tank_data->remaining_shells <= 0) {
        std::cout << "cooldown_wait: " << tank_data->cooldown_wait 
                  << ", remaining_shells: " << tank_data->remaining_shells << "\n";
        std::cout << "Tank " << tank_data->tank_index << " for player " << tank_data->player_index << " cannot shoot: cooldown or no shells left.\n";
        return false;
    }
    tank_data->remaining_shells--;
    overall_shells--; // Decrement overall shells counter
    addShell(tank_data->x, tank_data->y, tank_data->direction);
    tank_data->cooldown_wait = true;
    std::cout << "Tank " << tank_data->tank_index << " for player " << tank_data->player_index 
              << " shot a shell. Remaining shells: " << tank_data->remaining_shells 
              << ", Overall shells left: " << overall_shells << "\n";
    std::cout << "-----------------------------------\n";
    return true;
}

/******************/
/* if Game Ended  */
/******************/
bool GameManager_322719139_211961057_A::isGameOver() {
    // Count alive tanks per player using unified tank storage
    int player1_tanks = 0;
    int player2_tanks = 0;
    
    for (const auto& tank : tanks_) {
        if (tank.data.is_alive) {
            if (tank.data.player_index == 1) {
                player1_tanks++;
            } else {
                player2_tanks++;
            }
        }
    }

    if(player1_tanks == 0 || player2_tanks == 0){
        return true;
    }
    if(overall_shells == 0 && current_step_ >= max_steps_){
        return true;
    }

    const int no_shell_steps_threshold = 40;
    if (no_shell_steps_count_ >= no_shell_steps_threshold) return true;
    
    return false;
}

/*********************/
/* check collisions  */
/*********************/
void GameManager_322719139_211961057_A::checkCollisions() {
    // Temporary vectors to store indices of items to be removed
    std::vector<size_t> shells_to_remove;
    std::vector<size_t> walls_to_remove;
    
    // Check shell-wall collisions
    for (size_t shell_idx = 0; shell_idx < shells_.size(); ++shell_idx) {
        bool shell_destroyed = false;
        const auto& shell = shells_[shell_idx];
        
        // Check if shell hit a wall
        if (board_[shell.y][shell.x] == '#' || board_[shell.y][shell.x] == '$') {
            // Find the wall and damage it
            for (size_t wall_idx = 0; wall_idx < walls_.size(); ++wall_idx) {
                auto& wall = walls_[wall_idx];
                if (wall.x == shell.x && wall.y == shell.y) {
                    wall.health--;
                    if (wall.health == 1) {
                        // Wall is damaged but not destroyed
                        board_[wall.y][wall.x] = '$';
                    } else if (wall.health <= 0) {
                        // Wall is destroyed
                        board_[wall.y][wall.x] = ' ';
                        walls_to_remove.push_back(wall_idx);
                    }
                    break;
                }
            }
            shell_destroyed = true;
        }
        
        // Check if shell hit another shell
        for (size_t other_idx = 0; other_idx < shell_idx; ++other_idx) {
            if (std::find(shells_to_remove.begin(), shells_to_remove.end(), other_idx) != shells_to_remove.end()) {
                continue; // Skip shells already marked for removal
            }
            
            const auto& other_shell = shells_[other_idx];
            if (other_shell.x == shell.x && other_shell.y == shell.y) {
                // Shells collided, destroy both
                shell_destroyed = true;
                shells_to_remove.push_back(other_idx);
                break;
            }
        }
        
        // Check if shell hit a tank using unified tank storage
        for (auto& tank : tanks_) {
            if (tank.data.is_alive && tank.data.x == shell.x && tank.data.y == shell.y) {
                // Tank is hit
                std::cout <<"shell hit tank\n";
                tank.data.is_alive = false;
                shell_destroyed = true;
                break;
            }
        }
        
        // Mark shell for removal if destroyed
        if (shell_destroyed) {
            shells_to_remove.push_back(shell_idx);
        }
    }
    
    // Remove shells in reverse order to avoid index invalidation
    std::sort(shells_to_remove.begin(), shells_to_remove.end(), std::greater<size_t>());
    for (size_t idx : shells_to_remove) {
        if (idx < shells_.size()) { // Safety check
            shells_[idx] = shells_.back();
            shells_.pop_back();
        }
    }
    
    // Remove walls in reverse order to avoid index invalidation
    std::sort(walls_to_remove.begin(), walls_to_remove.end(), std::greater<size_t>());
    for (size_t idx : walls_to_remove) {
        if (idx < walls_.size()) { // Safety check
            walls_[idx] = walls_.back();
            walls_.pop_back();
        }
    }
    
    // Check tank-mine collisions using unified tank storage
    std::vector<size_t> mines_to_remove;
    for (auto& tank : tanks_) {
        if (tank.data.is_alive) {
            for (size_t mine_idx = 0; mine_idx < mines_.size(); ++mine_idx) {
                const auto& mine = mines_[mine_idx];
                if (tank.data.x == mine.x && tank.data.y == mine.y) {
                    // Tank hit a mine
                    tank.data.is_alive = false;
                    mines_to_remove.push_back(mine_idx);
                    break;
                }
            }
        }
    }
    
    // Remove mines in reverse order
    std::sort(mines_to_remove.begin(), mines_to_remove.end(), std::greater<size_t>());
    for (size_t idx : mines_to_remove) {
        if (idx < mines_.size()) { // Safety check
            mines_[idx] = mines_.back();
            mines_.pop_back();
        }
    }
    
    // Check tank-tank collisions using unified tank storage
    for (size_t i = 0; i < tanks_.size(); ++i) {
        for (size_t j = i + 1; j < tanks_.size(); ++j) {
            if (tanks_[i].data.is_alive && tanks_[j].data.is_alive &&
                tanks_[i].data.x == tanks_[j].data.x && tanks_[i].data.y == tanks_[j].data.y) {
                // Tanks collided
                std::cout <<"tank collision\n";
                tanks_[i].data.is_alive = false;
                tanks_[j].data.is_alive = false;
            }
        }
    }
}

/********************/
/* Helper functions */
/********************/
bool GameManager_322719139_211961057_A::isValidCellChar(char c) const {
    return c == '1' || c == '2' || c == '#' || c == '@' || c == ' ';
}

bool GameManager_322719139_211961057_A::CanMoveBackward(GameManager_322719139_211961057_A::TankData* tank_data){
    if (tank_data->in_backward_move && tank_data->backward_move_counter == 3)
    {
        //can preform backward move
        tank_data->in_reverse_mode =true;
        return true;
    }
    return false;
}

void GameManager_322719139_211961057_A::updateTankCounters(GameManager_322719139_211961057_A::TankData* tank_data) {
    // Update cooldown timer
    if (tank_data->cooldown_wait) {
        tank_data->cooldown_timer++;

        if (tank_data->cooldown_timer >= 4) {
            tank_data->cooldown_timer = 0;
            tank_data->cooldown_wait = false;
        }
    }

    // Update backward move sequence
    if (tank_data->in_backward_move) {
        if (tank_data->backward_move_counter < 2) {
            tank_data->backward_move_counter++;
        } else {
            // Sequence completed - enter reverse mode
            tank_data->in_backward_move = false;
            tank_data->backward_move_counter = 0;
            tank_data->in_reverse_mode = true;
        }
    }
}

void GameManager_322719139_211961057_A::addShell(size_t x,size_t y,Direction dir){
    ShellData new_shell(x, y,dir);
    shells_.push_back(new_shell);
}

void GameManager_322719139_211961057_A::moveShells() {
    // Create a vector to track shells that need to be removed
    std::vector<size_t> shells_to_remove;
    
    // Process each shell's movement one tile at a time
    for (size_t i = 0; i < shells_.size(); ++i) {
        // Skip shells already marked for removal
        if (std::find(shells_to_remove.begin(), shells_to_remove.end(), i) != shells_to_remove.end()) {
            continue;
        }
        
        auto& shell = shells_[i];
        auto [dx, dy] = DirectionUtil::getMovement(shell.direction);
        
        // First move (one tile)
        size_t intermediate_x = (shell.x + dx + cols_) % cols_;
        size_t intermediate_y = (shell.y + dy + rows_) % rows_;
        
        // Update shell position to intermediate position
        shell.x = intermediate_x;
        shell.y = intermediate_y;
        
        // Check for collisions at intermediate position
        bool shell_destroyed = checkShellCollision(i, shells_to_remove);
        
        // If shell was destroyed at intermediate position, skip second move
        if (!shell_destroyed) { 

            // Second move (another tile)
            size_t final_x = (shell.x + dx + cols_) % cols_;
            size_t final_y = (shell.y + dy + rows_) % rows_;
            
            // Update shell position to final position
            shell.x = final_x;
            shell.y = final_y;
            
            // Check for collisions at final position
            checkShellCollision(i, shells_to_remove);
        }
    }
    
    // Remove destroyed shells in reverse order to avoid index invalidation
    std::sort(shells_to_remove.begin(), shells_to_remove.end(), std::greater<size_t>());
    for (size_t idx : shells_to_remove) {
        if (idx < shells_.size()) { // Safety check
            shells_[idx] = shells_.back();
            shells_.pop_back();
        }
    }
}

bool GameManager_322719139_211961057_A::checkShellCollision(size_t shell_idx, std::vector<size_t>& shells_to_remove) {
    const auto& shell = shells_[shell_idx];
    bool shell_destroyed = false;
    
    // Check if shell hit a wall
    if (board_[shell.y][shell.x] == '#' || board_[shell.y][shell.x] == '$') {
        // Find the wall and damage it
        for (size_t wall_idx = 0; wall_idx < walls_.size(); ++wall_idx) {
            auto& wall = walls_[wall_idx];
            if (wall.x == shell.x && wall.y == shell.y) {
                wall.health--;
                if (wall.health == 1) {
                    // Wall is damaged but not destroyed
                    board_[wall.y][wall.x] = '$';
                } else if (wall.health <= 0) {
                    // Wall is destroyed
                    board_[wall.y][wall.x] = ' ';
                    // Use swap-and-pop for efficient removal
                    walls_[wall_idx] = walls_.back();
                    walls_.pop_back();
                    // Adjust wall_idx if we're not at the end
                    if (wall_idx < walls_.size()) {
                        --wall_idx;
                    }
                }
                break;
            }
        }
        shell_destroyed = true;
    }
    
    // Check if shell hit another shell
    for (size_t other_idx = 0; other_idx < shells_.size(); ++other_idx) {
        // Skip self or shells already marked for removal
        if (other_idx == shell_idx || 
            std::find(shells_to_remove.begin(), shells_to_remove.end(), other_idx) != shells_to_remove.end()) {
            continue;
        }
        
        const auto& other_shell = shells_[other_idx];
        if (other_shell.x == shell.x && other_shell.y == shell.y) {
            // Shells collided, destroy both
            shell_destroyed = true;
            shells_to_remove.push_back(other_idx);
            break;
        }
    }
    
    // Check if shell hit a tank using unified tank storage
    for (auto& tank : tanks_) {
        if (tank.data.is_alive && tank.data.x == shell.x && tank.data.y == shell.y) {
            // Tank is hit
            tank.data.is_alive = false;
            shell_destroyed = true;
            break;
        }
    }
    
    // Mark shell for removal if destroyed
    if (shell_destroyed) {
        shells_to_remove.push_back(shell_idx);
    }
    
    return shell_destroyed;
}

//////// From HW1: Utility function to get movement deltas based on direction ////////

// for visualization
std::string toString2(Direction dir) {
    switch (dir) {
        case Direction::UP: return "U";
        case Direction::UP_RIGHT: return "UR";
        case Direction::RIGHT: return "R";
        case Direction::DOWN_RIGHT: return "DR";
        case Direction::DOWN: return "D";
        case Direction::DOWN_LEFT: return "DL";
        case Direction::LEFT: return "L";
        case Direction::UP_LEFT: return "UL";
        default: return "UNKNOWN";
    }
}


void GameManager_322719139_211961057_A::setGameLogFile(const std::string &filename) {
    game_log_file_ = filename;
}

//check
void GameManager_322719139_211961057_A::setVisualizationFile(const std::string &filename) {
    visualization_file_ = filename;

    std::ofstream test(filename, std::ios::app);
    if (!test) {
        std::cerr << "Warning: Cannot write to visualization file " << filename << std::endl;
    }
    test.close();
}

void GameManager_322719139_211961057_A::writeVisualizationState() {
    if (!visualization_stream_.is_open()) return;

    json state;
    state["step"] = current_step_;

    // Convert board to proper JSON format
    json board_json = json::array();
    for (const auto& row : board_) {
        json row_json;
        for (char cell : row) {
            row_json.push_back(std::string(1, cell));
        }
        board_json.push_back(row_json);
    }
    state["board"] = board_json;

    // Add tanks
    json tanks_json = json::array();
    for (const auto& tank : tanks_) {
        const auto& data = tank.data;
        if (data.is_alive) {
            json tank_info;
            tank_info["player"] = data.player_index;
            tank_info["id"] = data.tank_index;
            tank_info["x"] = data.x;
            tank_info["y"] = data.y;
            tank_info["dir"] = toString2(data.direction);
            tank_info["alive"] = data.is_alive;

            tanks_json.push_back(tank_info);
        }
    }
    state["tanks"] = tanks_json;

    // Write as a complete JSON object per line
    visualization_stream_ << state.dump() << '\n';
    visualization_stream_.flush();
}

void GameManager_322719139_211961057_A::writeRoundStateJson(std::ofstream &out, int round_number) {
    if (round_number >= 0 && static_cast<size_t>(round_number) < round_actions_log_.size()) {
        out << round_actions_log_[round_number] << std::endl;
    }
}

//std::unique_ptr<AbstractGameManager> safeFactory(bool verbose) {
//    std::cout << "[DEBUG] About to construct GameManager\n";
//    auto ptr = std::make_unique<GameManager_322719139_211961057_A>(verbose);
//    std::cout << "[DEBUG] Successfully constructed GameManager\n";
//    return ptr;
//}

//GameManagerRegistration register_gm(safeFactory);
//REGISTER_GAME_MANAGER(GameManager_322719139_211961057::GameManager_322719139_211961057_A);
//REGISTER_GAME_MANAGER(GameManager_322719139_211961057_A); //it looks like its wrong, according to pdf, check and if so, correct one below:
//REGISTER_GAME_MANAGER(GameManager_322719139_211961057);

} // namespace GameManager_322719139_211961057

// Bring the class name into global scope for the macro
using GameManager_322719139_211961057::GameManager_322719139_211961057_A;
REGISTER_GAME_MANAGER(GameManager_322719139_211961057_A);