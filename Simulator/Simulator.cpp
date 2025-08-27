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
#include "DynamicLoader.h"
#include "GameManagerLoader.h"

namespace fs = std::filesystem;

static void parseHeaderLine(const std::string& line, const std::string& key, size_t& output) {
    auto pos = line.find('=');
    if (pos == std::string::npos) return;

    std::string k = line.substr(0, pos);
    std::string v = line.substr(pos + 1);

    // Trim both
    k.erase(std::remove_if(k.begin(), k.end(), ::isspace), k.end());
    v.erase(std::remove_if(v.begin(), v.end(), ::isspace), v.end());

    if (k == key) {
        try {
            output = std::stoul(v);
        } catch (...) {
            std::cerr << "[Warning] Failed to parse numeric value: " << v << " in line: " << line << "\n";
        }
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
    std::cout << "[DEBUG] Loading game map...\n";
    GameMapInfo map_info = loadGameMap(game_map_path);

    std::cout << "[DEBUG] Loading GameManagers...\n";
    loadGameManagers(gameManagers_folder, verbose);

    std::cout << "[DEBUG] Loading algorithms...\n";
    loadTwoAlgorithms(algo1_path, algo2_path, verbose);

    std::cout << "[DEBUG] Starting thread pool with " << num_threads << " threads...\n";

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
            std::cout << "[DEBUG] Running single game with: " << gm_name << "\n";
            auto result = runSingleGame(gm_name, map_info, alg1_name, alg2_name, verbose);

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

/*
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
    
    std::cout << "[DEBUG] Before raw_board, Map size: rows = " << info.rows << ", cols = " << info.cols << "\n";
    std::vector<std::vector<char>> raw_board(info.rows, std::vector<char>(info.cols, ' '));
    std::cout << "[DEBUG] Parsed map dimensions: rows=" << info.rows << ", cols=" << info.cols << "\n";

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
*/

GameMapInfo Simulator::loadGameMap(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open map file: " + filename);
    }

    GameMapInfo info;
    std::string line;

    // Line 1: Map name
    std::getline(file, info.name);
    std::cout << "[DEBUG] Map name: " << info.name << "\n";

    // Lines 2–5: Config
    std::getline(file, line); parseHeaderLine(line, "MaxSteps", info.max_steps);
    std::getline(file, line); parseHeaderLine(line, "NumShells", info.num_shells);
    std::getline(file, line); parseHeaderLine(line, "Rows", info.rows);
    std::getline(file, line); parseHeaderLine(line, "Cols", info.cols);

    std::cout << "[DEBUG] Parsed config: MaxSteps=" << info.max_steps
              << ", NumShells=" << info.num_shells
              << ", Rows=" << info.rows
              << ", Cols=" << info.cols << "\n";

    if (info.rows == 0 || info.cols == 0) {
        std::cerr << "[ERROR] Invalid board dimensions (0 rows or cols)\n";
    }

    std::vector<std::vector<char>> raw_board(info.rows, std::vector<char>(info.cols, ' '));

    info.player_tank_positions.clear();

    size_t row = 0;
    while (std::getline(file, line) && row < info.rows) {
        std::cout << "[DEBUG] Reading row " << row << ": " << line << "\n";
        for (size_t col = 0; col < std::min(line.length(), info.cols); ++col) {
            char c = line[col];
            raw_board[row][col] = c;

            if (isdigit(c)) {
                int player = c - '0';
                info.player_tank_positions[player].emplace_back(col, row);
                std::cout << "[DEBUG] Found tank for player " << player
                          << " at (" << col << "," << row << ")\n";

                if (player == 1) { info.x1 = col; info.y1 = row; }
                if (player == 2) { info.x2 = col; info.y2 = row; }
            }
        }
        row++;
    }

    std::cout << "[DEBUG] Parsed " << row << " board rows\n";
    std::cout << "[DEBUG] Final player 1 tank pos: (" << info.x1 << "," << info.y1 << ")\n";
    std::cout << "[DEBUG] Final player 2 tank pos: (" << info.x2 << "," << info.y2 << ")\n";

    info.view = std::make_unique<UserCommon_322719139_211961057::GameSatelliteView>(raw_board);

    return info;
}


// --------------------
// Load all GameManagers from folder
// --------------------
void Simulator::loadGameManagers(const std::string& folderPath, bool verbose) {
    gm_handles.clear(); //Clears the vector gmHandles so any previously loaded handles are removed.

    std::cout << "[DEBUG] Entered loadGameManagers with folder: " << folderPath << "\n";

    // entry is a directory_entry object that gives you information about each item in the folder.
    for (const auto& entry : fs::directory_iterator(folderPath)) {
        std::cout << "[DEBUG] Found file in folder: " << entry.path() << "\n";

        if (!entry.is_regular_file()) {
            std::cout << "[DEBUG] Skipped non-regular file: " << entry.path() << "\n";
            continue;
        }
        //Skips files that don’t have the .so extension.
        if (entry.path().extension() != ".so") {
            std::cout << "[DEBUG] Skipped non-.so file: " << entry.path() << "\n";
            continue;
        }

        std::string soPath = entry.path().string();
        std::cout << "[DEBUG] Attempting dlopen on: " << soPath << "\n";

        //Calls dlopen to load the shared library into memory.
        void* handle = dlopen(soPath.c_str(), RTLD_LAZY);

        std::cout << "[DEBUG] Returned from dlopen for: " << soPath << "\n";

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
        std::cout << "[DEBUG] Stored dlopen handle for: " << soPath << "\n";
    }
    std::cout << "[DEBUG] Finished loadGameManagers()\n";
}

// --------------------
// Load one or two algorithm .so files
// --------------------
void Simulator::loadTwoAlgorithms(
    const std::string& algo1Path,
    const std::string& algo2Path,
    bool verbose
) {
    std::cout << "---------- loadTwoAlgorithms ----------" << "\n";
    // Clear previous handles
    algo_handles.clear(); 

    // Lambda to load a single .so and return the handle
    auto loadSingle = [&](const std::string& path) -> void* {
        if (path.size() < 3 || path.substr(path.size() - 3) != ".so") {
            std::cerr << "[Error] Algorithm file must end with .so: " << path << "\n";
            return nullptr;
        }

        // Extract algorithm name from path (you can tweak this if needed)
        std::string name = fs::path(path).stem().string(); // removes .so
        std::cout << "[DEBUG] Creating AlgorithmFactoryEntry for: " << name << "\n";
        auto& registrar = AlgorithmRegistrar::getAlgorithmRegistrar();
        registrar.createAlgorithmFactoryEntry(name); // 🔥 REQUIRED

        void* handle = dlopen(path.c_str(), RTLD_NOW | RTLD_GLOBAL); // 🔥 REQUIRED FLAGS
        if (!handle) {
            std::cerr << "[Error] Failed to load Algorithm: " << path << "\n";
            std::cerr << dlerror() << "\n";
            registrar.removeLast(); // 🔄 roll back placeholder
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
GameResult Simulator::runSingleGame(const std::string &gmName, const GameMapInfo& mapInfo, const std::string& alg1, const std::string& alg2, bool verbose) {   
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

    TankAlgorithmFactory tankAlgo1Factory = [&tankRegistrar, &alg1](int player_index, int tank_index) {
        // Find the last registered factory with this name
        std::cout << "[DEBUG] Registered algorithms in registrar:\n";
        for (const auto& entry : tankRegistrar) {
            if (entry.name() == alg1 && entry.hasTankAlgorithmFactory()) {
                return entry.createTankAlgorithm(player_index, tank_index);
            }
        }
        throw std::runtime_error("TankAlgorithm factory not found: " + alg1);
    };

    TankAlgorithmFactory tankAlgo2Factory = [&tankRegistrar, &alg2](int player_index, int tank_index) {
        std::cout << "[DEBUG] Registered algorithms in registrar:\n";
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
    //auto mapInfo = loadGameMap(gameMapPath);
    //const GameMapInfo& mapInfo = map_info_ref;


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

    // Own the unique_ptrs to keep them alive
    //adan
    auto* concrete_view = dynamic_cast<const UserCommon_322719139_211961057::GameSatelliteView*>(mapInfo.view.get());
    if (!concrete_view) {
        throw std::runtime_error("Expected concrete GameSatelliteView for Player construction");
    }

    std::unique_ptr<Player> player1 = player1Factory(0, concrete_view->getBoardWidth(), concrete_view->getBoardHeight(), mapInfo.max_steps, mapInfo.num_shells);
    std::unique_ptr<Player> player2 = player2Factory(1, concrete_view->getBoardWidth(), concrete_view->getBoardHeight(), mapInfo.max_steps, mapInfo.num_shells);

    //std::unique_ptr<Player> player1 = player1Factory(0, mapInfo.x1, mapInfo.y1, mapInfo.max_steps, mapInfo.num_shells);
    //std::unique_ptr<Player> player2 = player2Factory(1, mapInfo.x2, mapInfo.y2, mapInfo.max_steps, mapInfo.num_shells);

    // Get raw pointers (safe because we retain ownership until run() ends)
    Player* player1_raw = player1.get();
    Player* player2_raw = player2.get();

    // 5. Run the game
    auto result = gm->run(mapInfo.cols, mapInfo.rows, *mapInfo.view, mapInfo.name, mapInfo.max_steps, mapInfo.num_shells,
        *player1_raw, alg1,
        *player2_raw, alg2,
        tankAlgo1Factory, tankAlgo2Factory);

    // unique_ptrs destruct after this line, which is fine
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
    //std::string filename = outputFolder + "/comparative_results_" + generateTimestamp() + ".txt";
    std::string filename = generateFilename("comparative_results", outputFolder);
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
std::string Simulator::generateFilename(const std::string& prefix, const std::string& folder) const {
    return folder + "/" + prefix + "_" + generateTimestamp() + ".txt";
}


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

//-------------------------------------------------
// Competitive Run Mode
//-------------------------------------------------

void Simulator::runCompetitive(
    const std::string& algorithms_folder,
    const std::string& game_maps_folder,
    const std::string& game_manager_so,
    size_t num_threads,
    bool verbose)
{
    // 1. Load algorithms
    loadAlgorithms(algorithms_folder, verbose);
    size_t N = algo_handles.size();
    if (N < 2) {
        std::cerr << "Error: Need at least 2 algorithms for competition mode.\n";
        return;
    }

    // 2. Load maps
    auto maps = loadGameMaps(game_maps_folder);
    size_t K = maps.size();
    if (K == 0) {
        std::cerr << "Error: No maps found in folder " << game_maps_folder << "\n";
        return;
    }

    // 3. Load GameManager
    GameManagerLoader gm_loader;
    std::string gm_name = std::filesystem::path(game_manager_so).filename().string();

    if (!gm_loader.loadGameManagerLibrary(game_manager_so, gm_name, std::cerr)) {
        std::cerr << "Error: Could not load or register GameManager: " << gm_name << "\n";
        return;
    }

    auto& gm_registrar = GameManagerRegistrar::getGameManagerRegistrar();
    if (gm_registrar.count() == 0) {
        std::cerr << "Error: No GameManager registered after loading " << gm_name << "\n";
        return;
    }

    auto gm_factory = gm_registrar.getFactory(gm_name);
    if (!gm_factory) {
        std::cerr << "Error: No factory found for GameManager " << gm_name << "\n";
        return;
    }

    // 4. Scoreboard
    std::map<std::string, int> scores;
    auto& algo_registrar = AlgorithmRegistrar::getAlgorithmRegistrar();
    if (algo_registrar.count() == 0) {
        throw std::runtime_error("[Simulator] No tank algorithms registered.");
    }

    for (const auto& entry : algo_registrar) {
        scores[entry.name()] = 0;
    }

    ThreadPool pool(num_threads >= 1 ? num_threads : 1);
    std::mutex score_mutex;

    using AlgoEntry = std::remove_reference_t<decltype(*algo_registrar.begin())>;
    std::vector<const AlgoEntry*> algo_entries;
    for (const auto& entry : algo_registrar) {
        algo_entries.push_back(&entry);
    }

    // Making it deterministic
    std::sort(algo_entries.begin(), algo_entries.end(),
        [](const AlgoEntry* a, const AlgoEntry* b) {
            return std::filesystem::path(a->name()).filename().string() <
                std::filesystem::path(b->name()).filename().string();
        });


    std::set<std::tuple<std::string, std::string, std::string>> executed_pairs;
    //static std::mutex dedup_mutex;
    std::mutex dedup_mutex;

    for (size_t k = 0; k < K; ++k) {
        const GameMapInfo* map_ptr = &maps[k];  // will capture this pointer

        //for (size_t i = 0; i < N; ++i) {
            //for (size_t j = i + 1; j < N; ++j) {
        for (size_t i = 0; i < N; ++i) {
            size_t j = (i + 1 + (k % (N - 1))) % N; // Pairing Formula set by the assignment

            if (i == j) continue;  // Safety check, though should never happen

            const AlgoEntry* entry1_ptr = algo_entries[i];
            const AlgoEntry* entry2_ptr = algo_entries[j];

            std::string a1 = entry1_ptr->name();
            std::string a2 = entry2_ptr->name();
            std::string map_name = map_ptr->name;

            auto ordered_pair = std::minmax(a1, a2);
            auto key = std::make_tuple(map_name, ordered_pair.first, ordered_pair.second);

            {
                std::lock_guard<std::mutex> lock(dedup_mutex);
                std::cout << "[DEBUG] Dedup key check for: " << map_name
                        << " (" << ordered_pair.first << " vs " << ordered_pair.second << ")\n";

                if (executed_pairs.count(key)) {
                    std::cout << "[DEBUG] Skipping duplicate for: " << map_name << "\n";
                    continue;
                }

                executed_pairs.insert(key);
            }

            pool.enqueue([&, entry1_ptr, entry2_ptr, map_ptr, a1, a2]() {
                const GameMapInfo& map_info = *map_ptr;  // safely dereferenced inside thread

                std::cout << "[DEBUG] Running game: " << map_info.name
                        << " | " << a1 << " vs " << a2 << std::endl;

                auto gm_instance = gm_factory(verbose);

                //adan
                auto* concrete_view = dynamic_cast<UserCommon_322719139_211961057::GameSatelliteView*>(map_info.view.get());
                if (!concrete_view) {
                    throw std::runtime_error("Failed to cast SatelliteView");
                }

                auto board_width = concrete_view->getBoardWidth();
                auto board_height = concrete_view->getBoardHeight();

                auto player1 = entry1_ptr->createPlayer(0, board_width, board_height, map_info.max_steps, map_info.num_shells);
                auto player2 = entry2_ptr->createPlayer(1, board_width, board_height, map_info.max_steps, map_info.num_shells);

                //auto player1 = entry1_ptr->createPlayer(0, map_info.x1, map_info.y1, map_info.max_steps, map_info.num_shells);
                //auto player2 = entry2_ptr->createPlayer(1, map_info.x2, map_info.y2, map_info.max_steps, map_info.num_shells);



                GameResult result = gm_instance->run(
                    map_info.cols, map_info.rows,
                    *map_info.view, map_info.name,
                    map_info.max_steps, map_info.num_shells,
                    *player1, a1,
                    *player2, a2,
                    [=](int player, int tank) { return entry1_ptr->createTankAlgorithm(player, tank); },
                    [=](int player, int tank) { return entry2_ptr->createTankAlgorithm(player, tank); }
                );

                std::lock_guard<std::mutex> lock(score_mutex);
                if (result.winner == 1) {
                    scores[a1] += 3;
                } else if (result.winner == 2) {
                    scores[a2] += 3;
                } else {
                    scores[a1] += 1;
                    scores[a2] += 1;
                }
            });
            //}
        }
    }

    pool.wait();

    // 6. Sort and write results
    std::vector<std::pair<std::string, int>> ranking(scores.begin(), scores.end());
    std::sort(ranking.begin(), ranking.end(), [](auto& a, auto& b) { return a.second > b.second; });

    std::string filename = generateFilename("competition", algorithms_folder);
    std::ofstream out(filename);
    std::ostream& os = out ? out : std::cout;

    os << "game_maps_folder=" << game_maps_folder << "\n";
    os << "game_manager=" << game_manager_so << "\n\n";

    for (auto& [name, score] : ranking) {
        os << name << " " << score << "\n";
    }

    if (out) {
        std::cout << "Competition results saved to " << filename << "\n";
    }
}

/*
std::vector<GameMapInfo> Simulator::loadGameMaps(const std::string& game_maps_folder) {

    std::vector<GameMapInfo> maps;

    try {
        if (!fs::exists(game_maps_folder) || !fs::is_directory(game_maps_folder)) {
            throw std::runtime_error("Invalid maps folder: " + game_maps_folder);
        }

        for (const auto& entry : fs::directory_iterator(game_maps_folder)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().string();

                try {
                    GameMapInfo map_info = loadGameMap(filename);
                    maps.push_back(std::move(map_info));
                } catch (const std::exception& e) {
                    std::cerr << "Skipping map file " << filename
                              << " due to error: " << e.what() << "\n";
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error loading maps from folder " << game_maps_folder
                  << ": " << e.what() << "\n";
    }

    return maps;
}
*/

std::vector<GameMapInfo> Simulator::loadGameMaps(const std::string& game_maps_folder) {
    std::vector<GameMapInfo> maps;

    try {
        if (!fs::exists(game_maps_folder) || !fs::is_directory(game_maps_folder)) {
            throw std::runtime_error("Invalid maps folder: " + game_maps_folder);
        }

        for (const auto& entry : fs::directory_iterator(game_maps_folder)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().string();

                try {
                    GameMapInfo map_info = loadGameMap(filename);

                    if (!map_info.view) {
                        std::cerr << "[WARNING] Skipping map " << filename
                                  << ": SatelliteView is null\n";
                        continue;
                    }

                    maps.push_back(std::move(map_info));
                } catch (const std::exception& e) {
                    std::cerr << "Skipping map file " << filename
                              << " due to error: " << e.what() << "\n";
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error loading maps from folder " << game_maps_folder
                  << ": " << e.what() << "\n";
    }

    return maps;
}

/*
void Simulator::loadAlgorithms(const std::string& algorithms_folder, bool verbose) {
    // Clear previous handles
    algo_handles.clear();

    namespace fs = std::filesystem;
    DynamicLoader dynamic_loader;

    if (!fs::exists(algorithms_folder) || !fs::is_directory(algorithms_folder)) {
        throw std::runtime_error("[Simulator] Invalid algorithms folder: " + algorithms_folder);
    }

    for (const auto& entry : fs::directory_iterator(algorithms_folder)) {
        if (!entry.is_regular_file()) continue;

        const std::string path = entry.path().string();
        const std::string filename = entry.path().filename().string();

        if (path.size() < 3 || path.substr(path.size() - 3) != ".so") {
            if (verbose)
                std::cerr << "[Simulator] Skipping non-.so file: " << path << "\n";
            continue;
        }

        if (dynamic_loader.loadAlgorithmLibrary(path, filename, std::cerr)) {
            algo_handles.push_back(dlopen(path.c_str(), RTLD_LAZY));  // Only for destructor cleanup
            if (verbose)
                std::cout << "[Simulator] Successfully registered algorithm: " << filename << "\n";
        } else {
            std::cerr << "[Simulator] Failed to register algorithm: " << filename << "\n";
        }
    }

    if (algo_handles.size() < 2) {
        throw std::runtime_error("[Simulator] Not enough valid algorithms found in folder. Need at least 2.");
    }

    if (verbose) {
        std::cout << "[Simulator] Total algorithms registered: "
                  << AlgorithmRegistrar::getAlgorithmRegistrar().count() << "\n";
    }
}
*/

void Simulator::loadAlgorithms(const std::string& algorithms_folder, bool verbose) {
    algo_handles.clear();
    DynamicLoader dynamic_loader;

    if (!fs::exists(algorithms_folder) || !fs::is_directory(algorithms_folder)) {
        throw std::runtime_error("[Simulator] Invalid algorithms folder: " + algorithms_folder);
    }

    std::unordered_set<std::string> seen_names;  // To prevent duplicate names

    for (const auto& entry : fs::directory_iterator(algorithms_folder)) {
        if (!entry.is_regular_file()) continue;

        const std::string path = entry.path().string();
        const std::string filename = entry.path().filename().string();

        if (entry.path().extension() != ".so") {
            if (verbose)
                std::cerr << "[Simulator] Skipping non-.so file: " << path << "\n";
            continue;
        }

        // Normalize name for registration (strip .so suffix)
        std::string algo_name = entry.path().stem().string();

        if (seen_names.count(algo_name)) {
            std::cerr << "[Simulator] Skipping duplicate algorithm name: " << algo_name << "\n";
            continue;
        }
        seen_names.insert(algo_name);

        if (verbose) {
            std::cout << "[Simulator] Attempting to register: " << algo_name
                      << " from file: " << filename << "\n";
        }

        // Prepare registrar entry BEFORE dlopen
        auto& registrar = AlgorithmRegistrar::getAlgorithmRegistrar();
        try {
            registrar.createAlgorithmFactoryEntry(algo_name);
        } catch (const std::exception& e) {
            std::cerr << "[Simulator] Registration failed before dlopen: " << e.what() << "\n";
            continue;
        }

        void* handle = dlopen(path.c_str(), RTLD_NOW | RTLD_GLOBAL);
        if (!handle) {
            std::cerr << "[Simulator] dlopen failed: " << path << "\n";
            std::cerr << "Reason: " << dlerror() << "\n";
            registrar.removeLast();  // cleanup
            continue;
        }

        // Validate registration
        if (dynamic_loader.loadAlgorithmLibrary(path, algo_name, std::cerr)) {
            algo_handles.push_back(handle);  // Keep if registration succeeded
            if (verbose)
                std::cout << "[Simulator] Successfully registered algorithm: " << algo_name << "\n";
        } else {
            std::cerr << "[Simulator] Registration failed for algorithm: " << algo_name << "\n";
            dlclose(handle);  // cleanup
            registrar.removeLast();  // safety cleanup
        }
    }

    if (algo_handles.size() < 2) {
        throw std::runtime_error("[Simulator] Not enough valid algorithms found in folder. Need at least 2.");
    }

    if (verbose) {
        std::cout << "[Simulator] Total algorithms registered: "
                  << AlgorithmRegistrar::getAlgorithmRegistrar().count() << "\n";
    }
}
