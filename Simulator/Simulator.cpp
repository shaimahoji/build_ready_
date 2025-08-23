// Simulator.cpp
#include "Simulator.h"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <ctime>
#include <dlfcn.h>


#include <thread>
#include <mutex>
#include <unordered_map>
#include <atomic>

#include "GameManagerRegistrar.h"
#include "AlgorithmRegistrar.h"
#include "ThreadPool.h"

namespace fs = std::filesystem;

static void parseHeaderLine(const std::string& line, const std::string& key, size_t& output) {
    auto pos = line.find('=');
    if (pos == std::string::npos) return;

    std::string k = line.substr(0, pos);
    std::string v = line.substr(pos + 1);

    if (k == key) {
        try {
            output = std::stoul(v);
        } catch (...) {}
    }
}

// --------------------
// Destructor
// --------------------
Simulator::~Simulator() {
    for (auto handle : gm_handles) {
        if (handle) dlclose(handle);
    }
    for (auto handle : algo_handles) {
        if (handle) dlclose(handle);
    }
}

// --------------------
// Comparative mode
// --------------------
void Simulator::runComparative(
    const std::string& game_map_path,
    const std::string& algo1_path,
    const std::string& algo2_path,
    const std::string& gameManagers_folder,
    int num_threads,
    bool verbose
) {
    // 1. Load managers and algorithms
    GameMapInfo map_info = loadGameMap(game_map_path);
    loadGameManagers(gameManagers_folder, verbose);
    loadTwoAlgorithms(algo1_path, algo2_path, verbose);

    // Get all GameManager names from the registrar
    auto& gm_registrar = GameManagerRegistrar::getGameManagerRegistrar();
    auto gm_names = gm_registrar.listRegistered();
    if (gm_names.empty()) {
        throw std::runtime_error("[Simulator] No GameManagers registered.");
    }

    // Get tank algorithm names
    std::string alg1_name; //without an o
    std::string alg2_name;
    auto& algo_registrar = AlgorithmRegistrar::getAlgorithmRegistrar();

    if (algo_registrar.count() == 0) {
        throw std::runtime_error("[Simulator] No tank algorithms registered.");
    }

    // First algorithm
    auto& first_entry = *algo_registrar.begin();
    alg1_name = first_entry.name();
    if (verbose) std::cout << "[Simulator] Tank Algorithm 1: " << alg1_name << "\n";

    // Second algorithm
    if (algo_registrar.count() > 1) {
        auto& second_entry = *(algo_registrar.begin() + 1);
        alg2_name = (algo_handles[0] == algo_handles[1]) 
                   ? alg1_name       // Same .so → reuse first name
                   : second_entry.name();
    } else {
        // Only one algorithm loaded → use the same for both
        alg2_name = alg1_name;
    }

    if (verbose) std::cout << "[Simulator] Tank Algorithm 2: " << alg2_name << "\n";

    ThreadPool pool(num_threads >= 1 ? num_threads : 1);

    for (const auto& gm_name : gm_names) {
        pool.enqueue([&, gm_name]() {
            auto result = runSingleGame(gm_name, game_map_path, alg1_name, alg2_name, verbose);

            std::lock_guard<std::mutex> lock(results_mutex);
            run_results.emplace_back(gm_name, std::move(result));
        });
    }

    pool.wait();  // Block until all tasks complete

    auto grouped = groupResults(map_info);

    writeComparativeOutput(
        gameManagers_folder,
        fs::path(game_map_path).filename().string(),
        fs::path(algo1_path).filename().string(),
        fs::path(algo2_path).filename().string(),
        grouped
    );
}

GameMapInfo Simulator::loadGameMap(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open map file: " + filename);
    }

    GameMapInfo info;
    std::string line;

    // Line 1: Map name
    std::getline(file, info.name);

    // Lines 2–5: Config
    std::getline(file, line); parseHeaderLine(line, "MaxSteps", info.max_steps);
    std::getline(file, line); parseHeaderLine(line, "NumShells", info.num_shells);
    std::getline(file, line); parseHeaderLine(line, "Rows", info.rows);
    std::getline(file, line); parseHeaderLine(line, "Cols", info.cols);

    std::vector<std::vector<char>> raw_board(info.rows, std::vector<char>(info.cols, ' '));
    info.player_tank_positions.clear();

    size_t row = 0;
    while (std::getline(file, line) && row < info.rows) {
        for (size_t col = 0; col < std::min(line.length(), info.cols); ++col) {
            char c = line[col];
            raw_board[row][col] = c;

            if (isdigit(c)) {
                int player = c - '0';
                info.player_tank_positions[player].emplace_back(col, row);
                if (player == 1) { info.x1 = col; info.y1 = row; }
                if (player == 2) { info.x2 = col; info.y2 = row; }
            }
        }
        row++;
    }

    // Construct the GameSatelliteView
    info.view = std::make_unique<UserCommon_322719139_211961057::GameSatelliteView>(raw_board, info.x1, info.y1, 1);

    return info;
}

// --------------------
// Load all GameManagers from folder
// --------------------
void Simulator::loadGameManagers(const std::string& folderPath, bool verbose) {
    gm_handles.clear(); //Clears the vector gmHandles so any previously loaded handles are removed.

    // entry is a directory_entry object that gives you information about each item in the folder.
    for (const auto& entry : fs::directory_iterator(folderPath)) {
        if (!entry.is_regular_file()) continue;
        //Skips files that don’t have the .so extension.
        if (entry.path().extension() != ".so") continue;


        std::string soPath = entry.path().string();
        //Calls dlopen to load the shared library into memory.
        void* handle = dlopen(soPath.c_str(), RTLD_LAZY);
        if (!handle) {
            std::cerr << "[Error] Failed to load GameManager .so: " << soPath << "\n";
            std::cerr << dlerror() << "\n";
            continue;
        }

        if (verbose) {
            std::cout << "[Simulator] Loaded GameManager: " << soPath << "\n";
        }

        //Stores the dlopen handle in the gmHandles vector so you can later dlclose() it in the destructor.
        //Without storing the handle, we can't safely unload the .so later.
        gm_handles.push_back(handle);
        // REGISTER_GAME_MANAGER macro inside .so runs now
    }
}

// --------------------
// Load one or two algorithm .so files
// --------------------
void Simulator::loadTwoAlgorithms(
    const std::string& algo1Path,
    const std::string& algo2Path,
    bool verbose
) {
    // Clear previous handles
    algo_handles.clear(); 

    // Lambda to load a single .so and return the handle
    auto loadSingle = [&](const std::string& path) -> void* {
        if (path.size() < 3 || path.substr(path.size() - 3) != ".so") {
            std::cerr << "[Error] Algorithm file must end with .so: " << path << "\n";
            return nullptr;
        }

        void* handle = dlopen(path.c_str(), RTLD_LAZY);
        if (!handle) {
            std::cerr << "[Error] Failed to load Algorithm: " << path << "\n";
            std::cerr << dlerror() << "\n";
            return nullptr;
        }

        if (verbose) {
            std::cout << "[Simulator] Loaded Algorithm: " << path << "\n";
        }

        algo_handles.push_back(handle);
        return handle;
    };

    // Load first algorithm
    void* handle1 = loadSingle(algo1Path);
    if (!handle1) {
        throw std::runtime_error("[Simulator] Failed to load first algorithm.");
    }

    // Load second algorithm
    void* handle2 = nullptr;
    if (algo2Path.empty()) {
        throw std::runtime_error("[Simulator] Second algorithm path is empty.");
    }

    // Check if the paths are the same absolute file
    if (fs::absolute(algo1Path) != fs::absolute(algo2Path)) {
        handle2 = loadSingle(algo2Path);
        if (!handle2) {
            throw std::runtime_error("[Simulator] Failed to load second algorithm.");
        }
    } else {
        // Same file → point to the first handle
        handle2 = handle1;

        if (verbose) {
            std::cout << "[Simulator] Second algorithm points to the same file as first. Using same name.\n";
        }
    }

    // At this point:
    // handle1 = first algorithm
    // handle2 = second algorithm (may be same as handle1)
}

// --------------------
// Run a single game
// --------------------
GameResult Simulator::runSingleGame(const std::string& gmName,const std::string& gameMapPath, const std::string& alg1, const std::string& alg2,bool verbose) {
    // 1. Get the GameManager factory
    auto& gmRegistrar = GameManagerRegistrar::getGameManagerRegistrar();
    auto gmFactory = gmRegistrar.getFactory(gmName);
    if (!gmFactory) {
        std::cerr << "[Error] GameManager factory not found for " << gmName << "\n";
        return GameResult{};
    }

    //TO DO: add factories
    // 2. Prepare TankAlgorithm factories for each algorithm
    auto& tankRegistrar = AlgorithmRegistrar::getAlgorithmRegistrar();

    auto tankAlgo1Factory = [&tankRegistrar, &alg1](int player_index, int tank_index) {
        // Find the last registered factory with this name
        for (const auto& entry : tankRegistrar) {
            if (entry.name() == alg1 && entry.hasTankAlgorithmFactory()) {
                return entry.createTankAlgorithm(player_index, tank_index);
            }
        }
        throw std::runtime_error("TankAlgorithm factory not found: " + alg1);
    };

    auto tankAlgo2Factory = [&tankRegistrar, &alg2](int player_index, int tank_index) {
        for (const auto& entry : tankRegistrar) {
            if (entry.name() == alg2 && entry.hasTankAlgorithmFactory()) {
                return entry.createTankAlgorithm(player_index, tank_index);
            }
        }
        throw std::runtime_error("TankAlgorithm factory not found: " + alg2);
    };

    // 3. Create GameManager instance for player 1 (You will create a separate GM instance per test)
    //auto gm = gmFactory(player1Factory, tankAlgo1Factory);
    auto gm = gmFactory(verbose);

    if (!gm) {
        std::cerr << "[Error] Failed to create GameManager instance.\n";
        return GameResult{};
    }

    // 4. Load map from file
    auto mapInfo = loadGameMap(gameMapPath);

    // Use AlgorithmRegistrar instead of PlayerRegistrar
    auto& algoRegistrar = AlgorithmRegistrar::getAlgorithmRegistrar();

    auto player1Factory = [&algoRegistrar, &alg1]() -> PlayerFactory {
        for (const auto& entry : algoRegistrar) {
            if (entry.name() == alg1 && entry.hasPlayerFactory()) {
                return [entry](int player_index, size_t x, size_t y, size_t max_steps, size_t num_shells) {
                    return entry.createPlayer(player_index, x, y, max_steps, num_shells);
                };
            }
        }
        throw std::runtime_error("Player factory not found: " + alg1);
    }();

    auto player2Factory = [&algoRegistrar, &alg2]() -> PlayerFactory {
        for (const auto& entry : algoRegistrar) {
            if (entry.name() == alg2 && entry.hasPlayerFactory()) {
                return [entry](int player_index, size_t x, size_t y, size_t max_steps, size_t num_shells) {
                    return entry.createPlayer(player_index, x, y, max_steps, num_shells);
                };
            }
        }
        throw std::runtime_error("Player factory not found: " + alg2);
    }();

    // Create players
    auto player1 = player1Factory(0, mapInfo.x1, mapInfo.y1, mapInfo.max_steps, mapInfo.num_shells);
    auto player2 = player2Factory(1, mapInfo.x2, mapInfo.y2, mapInfo.max_steps, mapInfo.num_shells);


    // 5. Run the game
    auto result = gm->run(mapInfo.cols, mapInfo.rows, *mapInfo.view, mapInfo.name, mapInfo.max_steps, mapInfo.num_shells,
        *player1, alg1,
        *player2, alg2,
        tankAlgo1Factory, tankAlgo2Factory);

    // 6. Write output file (optional)
    //std::string output_file = "output_" + gmName + ".txt";
    //gm->writeOutput(output_file);
    
    // 7. Extract the game result from gm or keep your own logic to gather it
    //return gm->getGameResult();  // You need to implement this or similar method
    return result;
}


// --------------------
// Placeholder: group results
// --------------------
Simulator::GroupedResults Simulator::groupResults(const GameMapInfo& map_info) {
    using Key = std::tuple<std::string, size_t, std::string>; // (msg, round, map)
    std::map<Key, std::vector<std::string>> grouped;

    for (const auto& [gmName, res] : run_results) {
        const std::string msg = formatResultMessage(res);
        const size_t round = res.rounds;

        const std::string map = res.gameState
            ? serializeMap(*res.gameState, map_info.cols, map_info.rows)
            : "<no map>";

        Key key{msg, round, map};
        grouped[key].push_back(gmName);
    }

    std::cout << "[Simulator] Grouped results into " << grouped.size() << " distinct outcome(s).\n";
    int groupId = 1;
    for (const auto& [key, names] : grouped) {
        std::cout << "  Group #" << groupId++ << ": ";
        for (const auto& name : names) std::cout << name << " ";
        std::cout << "\n";
    }

    // Optional: store into internal variable if needed elsewhere
    // groupedResults = std::vector<std::pair<Key, std::vector<std::string>>>(grouped.begin(), grouped.end());
    return grouped;
}


// --------------------
// Output file writing
// --------------------
void Simulator::writeComparativeOutput(
    const std::string& outputFolder,
    const std::string& gameMapName,
    const std::string& algo1Name,
    const std::string& algo2Name,
    const GroupedResults& grouped
) {
    std::string filename = outputFolder + "/comparative_results_" + generateTimestamp() + ".txt";
    std::ofstream out(filename);
    if (!out) {
        std::cerr << "[Error] Could not open " << filename << " for writing.\n";
        return;
    }

    out << "game_map="   << gameMapName << "\n";
    out << "algorithm1=" << algo1Name   << "\n";
    out << "algorithm2=" << algo2Name   << "\n";
    out << "\n";

    bool firstGroup = true;
    for (const auto& [key, gmNames] : grouped) {
        const auto& [msg, round, map] = key;

        if (!firstGroup) out << "\n";
        firstGroup = false;

        for (size_t i = 0; i < gmNames.size(); ++i) {
            if (i) out << ", ";
            out << gmNames[i];
        }
        out << "\n";

        out << msg << "\n";
        out << round << "\n";
        out << map << "\n";
    }
}

// Helpers --------------------------------------------------------------

std::string Simulator::formatResultMessage(const GameResult& r) {
    auto rem = [&](int playerIdx)->size_t {
        if (playerIdx <= 0) return 0;
        size_t i = static_cast<size_t>(playerIdx - 1);
        return (i < r.remaining_tanks.size()) ? r.remaining_tanks[i] : 0;
    };

    switch (r.reason) {
        case GameResult::ALL_TANKS_DEAD:
            if (r.winner == 0) return "Tie - all tanks destroyed";
            // Matches the example: "Player 2 won with 2 tanks still alive"
            return "Player " + std::to_string(r.winner) +
                   " won with " + std::to_string(rem(r.winner)) +
                   " tanks still alive";
        case GameResult::MAX_STEPS:
            if (r.winner == 0) return "Tie - max steps reached";
            return "Player " + std::to_string(r.winner) + " won - max steps reached";
        case GameResult::ZERO_SHELLS:
            if (r.winner == 0) return "Tie - zero shells remaining";
            return "Player " + std::to_string(r.winner) + " won - zero shells remaining";
    }
    return "Unknown result";
}

// Serialize final map. Replace with your real implementation.
std::string Simulator::serializeMap(const SatelliteView& view, size_t width, size_t height) {
    std::ostringstream oss;
    for (size_t y = 0; y < height; ++y) {
        for (size_t x = 0; x < width; ++x) {
            oss << view.getObjectAt(x, y);
        }
        if (y + 1 < height) oss << "\n"; // newline after each row
    }
    return oss.str();
}

// --------------------
// Utility: timestamp
// --------------------
std::string Simulator::generateTimestamp() const {
    std::time_t t = std::time(nullptr);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", std::localtime(&t));
    return buf;
}

void Simulator::runCompetitive(
    const std::string& algorithms_folder,
    const std::string& game_maps_folder,
    const std::string& game_manager_so,
    size_t num_threads,
    bool verbose){}