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

static bool parseHeaderLine(const std::string& line, const std::string& key, size_t& output) {
    auto pos = line.find('=');
    if (pos == std::string::npos) return false;

    std::string k = line.substr(0, pos);
    std::string v = line.substr(pos + 1);

    // Trim both
    k.erase(std::remove_if(k.begin(), k.end(), ::isspace), k.end());
    v.erase(std::remove_if(v.begin(), v.end(), ::isspace), v.end());

    if (k == key) {
        try {
            output = std::stoul(v);
            return true;
        } catch (...) {
            std::cerr << "[Warning] Failed to parse numeric value: " << v << " in line: " << line << "\n";
            return false;
        }
    }

    return false;
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
    std::cout << "---------- runComparative ----------" << "\n";

    // Reset registrars before loading new .so files
    AlgorithmRegistrar::getAlgorithmRegistrar().clear();
    GameManagerRegistrar::getGameManagerRegistrar().clear();

    // 1. Load managers and algorithms
    std::cout << "[DEBUG] Loading game map...\n";
    //GameMapInfo map_info = loadGameMap(game_map_path);
    auto maybe_map = loadGameMap(game_map_path);
    if (!maybe_map) {
        std::cerr << "[FATAL] Aborting due to invalid map: " << game_map_path << "\n";
        return; // gracefully ends main
    }

    GameMapInfo map_info = std::move(*maybe_map);

    std::cout << "[DEBUG] Loading GameManagers...\n";
    loadGameManagers(gameManagers_folder, verbose);

    std::cout << "[DEBUG] Loading algorithms...\n";
    if(!loadTwoAlgorithms(algo1_path, algo2_path, verbose)) {
        std::cerr << "[FATAL] Aborting simulation due to failure loading algorithms.\n";
        return;
    }

    std::cout << "[DEBUG] Starting thread pool with " << num_threads << " threads...\n";

    // Get all GameManager names from the registrar
    auto& gm_registrar = GameManagerRegistrar::getGameManagerRegistrar();
    auto gm_names = gm_registrar.listRegistered();
    if (gm_names.empty()) {
        std::cerr << "[FATAL] No GameManagers loaded from folder: " << gameManagers_folder << "\n";
        return;
    }
    // For determinism, sort the names
    std::sort(gm_names.begin(), gm_names.end());

    // Get tank algorithm names
    std::string alg1_name; //without an o
    std::string alg2_name;
    auto& algo_registrar = AlgorithmRegistrar::getAlgorithmRegistrar();

    if (algo_registrar.count() == 0) {
        std::cerr << "[FATAL] No tank algorithms registered after loading .so files.\n";
        return;
    }

    // Collect and sort algorithm names deterministically
    std::vector<std::string> algo_names;
    for (const auto& entry : algo_registrar) {
        algo_names.push_back(entry.name());
    }
    std::sort(algo_names.begin(), algo_names.end());

    // Pick first two (or reuse first if only one available)
    alg1_name = algo_names[0];
    alg2_name = (algo_names.size() > 1)
                ? algo_names[1]
                : algo_names[0];

    if (verbose) {
        std::cout << "[Simulator] Tank Algorithm 1: " << alg1_name << "\n";
        std::cout << "[Simulator] Tank Algorithm 2: " << alg2_name << "\n";
    }

    if (num_threads <= 1) {
        for (const auto& gm_name : gm_names) {
            std::cout << "[DEBUG] Running single game with: " << gm_name << "[DEBUG]\n";

            auto tmp_result = runSingleGame(gm_name, map_info, alg1_name, alg2_name, verbose);
            if (!tmp_result) {
                std::cerr << "[Warning] Skipping game due to setup error.\n";
                return;
            }
            GameResult result = std::move(*tmp_result);

            run_results.emplace_back(gm_name, std::move(result));
        }
    } else {
        size_t max_tasks = gm_names.size(); // or however you store them
        ThreadPool pool(num_threads, max_tasks);    // new
        //ThreadPool pool(num_threads);
        for (const auto& gm_name : gm_names) {
            pool.enqueue([&, gm_name]() {
                std::cout << "[DEBUG] Running game: " << gm_name
                            << " on thread " << std::this_thread::get_id() << "[DEBUG]\n";

                auto tmp_result = runSingleGame(gm_name, map_info, alg1_name, alg2_name, verbose);
                if (!tmp_result) {
                    std::cerr << "[Warning] Skipping game due to setup error.\n";
                    return;
                }
                GameResult result = std::move(*tmp_result);

                std::lock_guard<std::mutex> lock(results_mutex);
                run_results.emplace_back(gm_name, std::move(result));
            });
        }
        pool.wait();
    }

    auto grouped = groupResults(map_info);

    writeComparativeOutput(
        gameManagers_folder,
        fs::path(game_map_path).filename().string(),
        fs::path(algo1_path).filename().string(),
        fs::path(algo2_path).filename().string(),
        grouped
    );
    std::cout << "[Simulator] Comparative results written.\n";
}


std::optional<GameMapInfo> Simulator::loadGameMap(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "[ERROR] Could not open map file: " << filename << "\n";
        return std::nullopt;
    }

    GameMapInfo info;
    std::string line;

    // Line 1: Map name
    std::getline(file, info.name);
    std::cout << "[DEBUG] Map name: " << info.name << "\n";

    bool found_rows = false, found_cols = false, found_max_steps = false;

    // Lines 2–5: Config
    std::getline(file, line); found_max_steps = parseHeaderLine(line, "MaxSteps", info.max_steps);
    std::getline(file, line); parseHeaderLine(line, "NumShells", info.num_shells);
    std::getline(file, line); found_rows = parseHeaderLine(line, "Rows", info.rows);
    std::getline(file, line); found_cols = parseHeaderLine(line, "Cols", info.cols);
    
    // "missing" or "explicitly set to zero" (max_steps=0 we can consider it a tie, not invalid)
    if (!found_rows || !found_cols || !found_max_steps || info.rows == 0 || info.cols == 0) {
        std::cerr << "[ERROR] Missing (or invalid) map config fields (MaxSteps / Rows / Cols) in: " << filename << "\n";
        return std::nullopt;
    }

    std::cout << "[DEBUG] Parsed config: MaxSteps=" << info.max_steps
              << ", NumShells=" << info.num_shells
              << ", Rows=" << info.rows
              << ", Cols=" << info.cols << "\n";


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
bool Simulator::loadTwoAlgorithms(
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
        
        if(!registrar.createAlgorithmFactoryEntry(name)) {
            std::cerr << "[Error] Failed to create AlgorithmFactoryEntry for: " << name << "\n";
            return nullptr;
        }

        void* handle = dlopen(path.c_str(), RTLD_NOW | RTLD_GLOBAL); // REQUIRED FLAGS
        if (!handle) {
            std::cerr << "[Error] Failed to load Algorithm: " << path << "\n";
            std::cerr << dlerror() << "\n";
            registrar.removeLast(); // roll back placeholder
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
        std::cerr << "[Simulator] Failed to load first algorithm.\n";
        return false;
    }

    // Load second algorithm
    void* handle2 = nullptr;
    if (algo2Path.empty()) {
        std::cerr << "[Simulator] Second algorithm path is empty.\n";
        return false;
    }

    // Check if the paths are the same absolute file
    if (fs::absolute(algo1Path) != fs::absolute(algo2Path)) {
        handle2 = loadSingle(algo2Path);
        if (!handle2) {
            std::cerr << "[Simulator] Failed to load second algorithm.\n";
            return false;
        }
    } else {
        // Same file → point to the first handle
        handle2 = handle1;

        if (verbose) {
            std::cout << "[Simulator] Second algorithm points to the same file as first. Using same name.\n";
        }
    }

    return true;
}

// --------------------
// Run a single game
// --------------------
std::optional<GameResult> Simulator::runSingleGame(const std::string &gmName, const GameMapInfo& mapInfo, const std::string& alg1, const std::string& alg2, bool verbose) {   
    // 1. Get the GameManager factory
    auto& gmRegistrar = GameManagerRegistrar::getGameManagerRegistrar();
    auto gmFactory = gmRegistrar.getFactory(gmName);
    if (!gmFactory) {
        std::cerr << "[Error] GameManager factory not found for " << gmName << "\n";
        return std::nullopt;
    }

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
        std::cerr << "[Error] TankAlgorithm factory not found: " << alg1 << "\n";
        return std::unique_ptr<TankAlgorithm>{};
    };

    TankAlgorithmFactory tankAlgo2Factory = [&tankRegistrar, &alg2](int player_index, int tank_index) {
        std::cout << "[DEBUG] Registered algorithms in registrar:\n";
        for (const auto& entry : tankRegistrar) {
            if (entry.name() == alg2 && entry.hasTankAlgorithmFactory()) {
                return entry.createTankAlgorithm(player_index, tank_index);
            }
        }
        std::cerr << "[Error] TankAlgorithm factory not found: " << alg2 << "\n";
        return std::unique_ptr<TankAlgorithm>{};
    };

    // 3. Create GameManager instance for player 1 (You will create a separate GM instance per test)
    auto gm = gmFactory(verbose);

    if (!gm) {
        std::cerr << "[Error] Failed to create GameManager instance.\n";
        return std::nullopt;
    }

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
        std::cerr << "[Error] Player factory not found: " << alg1 << "\n";
        return nullptr;
    }();

    auto player2Factory = [&algoRegistrar, &alg2]() -> PlayerFactory {
        for (const auto& entry : algoRegistrar) {
            if (entry.name() == alg2 && entry.hasPlayerFactory()) {
                return [entry](int player_index, size_t x, size_t y, size_t max_steps, size_t num_shells) {
                    return entry.createPlayer(player_index, x, y, max_steps, num_shells);
                };
            }
        }
        std::cerr << "[Error] Player factory not found: " << alg2 << "\n";
        return nullptr;
    }();

    // Own the unique_ptrs to keep them alive
    auto* concrete_view = dynamic_cast<const UserCommon_322719139_211961057::GameSatelliteView*>(mapInfo.view.get());
    if (!concrete_view) {
        std::cerr << "[Error] Expected GameSatelliteView for Player construction, got nullptr or wrong type.\n";
        return std::nullopt;
    }

    std::unique_ptr<Player> player1 = player1Factory(1, mapInfo.cols, mapInfo.rows, mapInfo.max_steps, mapInfo.num_shells);
    std::unique_ptr<Player> player2 = player2Factory(2, mapInfo.cols, mapInfo.rows, mapInfo.max_steps, mapInfo.num_shells);
    
    if (!player1 || !player2) {
        std::cerr << "[Error] Failed to create Player instances.\n";
        return std::nullopt;
    }

    // Get raw pointers (safe because we retain ownership until run() ends)
    Player* player1_raw = player1.get();
    Player* player2_raw = player2.get();

    // 5. Run the game
    auto result = gm->run(mapInfo.cols, mapInfo.rows, *mapInfo.view, mapInfo.name, mapInfo.max_steps, mapInfo.num_shells,
        *player1_raw, alg1,
        *player2_raw, alg2,
        tankAlgo1Factory, tankAlgo2Factory);

    if (result.winner == 0 && result.rounds == 0 && result.gameState == nullptr) {
        std::cerr << "[Error] GameManager returned a failure GameResult (possibly from tank setup failure).\n";
        return std::nullopt;
    }

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

     
        //const std::string map = res.gameState
        //    ? serializeMap(*res.gameState, map_info.cols, map_info.rows)
        //    : "<no map>";
        std::string map;
        if (auto* gsv = dynamic_cast<const UserCommon_322719139_211961057::GameSatelliteView*>(&*res.gameState)) {

            map = res.gameState
                ? serializeMap2(*gsv, map_info.cols, map_info.rows)
                : "<no map>";
        }
        else{
            map = res.gameState
                ? serializeMap(*res.gameState, map_info.cols, map_info.rows)
                : "<no map>";
        }

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
    std::cout << "[DEBUG] Writing comparative :\n";

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

    // Copy grouped results into a vector to sort by group size descending
    std::vector<std::pair<Simulator::GroupedResults::key_type, std::vector<std::string>>> sorted_groups(
        grouped.begin(), grouped.end()
    );

    std::sort(sorted_groups.begin(), sorted_groups.end(),
              [](const auto& a, const auto& b) {
                  return a.second.size() > b.second.size(); // biggest group first
              });

    bool firstGroup = true;
    for (const auto& [key, gmNames] : sorted_groups) {
        const auto& [msg, round, map] = key;

        if (msg.empty()) std::cerr << "[WARN] msg is empty\n";
        if (map.empty()) std::cerr << "[WARN] map is empty\n";

        if (!firstGroup) out << "\n";
        firstGroup = false;

        // Write comma-separated list of game managers
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

std::string Simulator::serializeMap2(const UserCommon_322719139_211961057::GameSatelliteView &view, size_t width, size_t height)
{
    std::ostringstream oss;
    for (size_t y = 0; y < height; ++y) {
        for (size_t x = 0; x < width; ++x) {
            oss << view.getObjectAt2(x, y);
        }
        if (y + 1 < height) oss << "\n"; // newline after each row
    }
    return oss.str();
}


// Serialize final map. Replace with your real implementation.
std::string Simulator::serializeMap(const SatelliteView& view, size_t width, size_t height) {
    std::cout << "[DEBUG] Serializing map is called \n";
    std::ostringstream oss;
    for (size_t y = 0; y < height; ++y) {
        for (size_t x = 0; x < width; ++x) {
            oss << view.getObjectAt(x, y);
            std::cout << "object is : "<< view.getObjectAt(x, y) << "\n"; // Debug print each character
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
    std::cout << "---------- runCompetitive ----------" << "\n";
    // Reset registrars before loading new .so files
    AlgorithmRegistrar::getAlgorithmRegistrar().clear();
    GameManagerRegistrar::getGameManagerRegistrar().clear();

    // 1. Load algorithms
    if(!loadAlgorithms(algorithms_folder, verbose)) {
        std::cerr << "Error: Failed to load algorithms from folder: " << algorithms_folder << "\n";
        return;
    }

    size_t N = algo_handles.size();
    if (N < 2) {
        std::cerr << "Error: Need at least 2 algorithms for competition mode.\n";
        return;
    }

    // 2. Load maps (ensure deterministic order)
    auto maps = loadGameMaps(game_maps_folder);
    if (maps.empty()) {
        std::cerr << "[Error] No valid maps were loaded from folder: " << game_maps_folder << "\n";
        return;
    }

    std::sort(maps.begin(), maps.end(),
        [](const GameMapInfo& a, const GameMapInfo& b) {
            return a.name < b.name;
        });

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

    // 4. Scoreboard and Algorithm List
    std::map<std::string, int> scores;
    auto& algo_registrar = AlgorithmRegistrar::getAlgorithmRegistrar();
    if (algo_registrar.count() == 0) {
        std::cerr << "Error: No algorithms registered after loading from " << algorithms_folder << "\n";
        return;
    }

    // Define AlgoEntry type early
    using AlgoEntry = std::remove_reference_t<decltype(*algo_registrar.begin())>;
    std::vector<const AlgoEntry*> algo_entries;
    for (const auto& entry : algo_registrar) {
        scores[entry.name()] = 0;
        algo_entries.push_back(&entry);
    }

    // Sort algorithms deterministically
    std::sort(algo_entries.begin(), algo_entries.end(),
        [](const AlgoEntry* a, const AlgoEntry* b) {
            return std::filesystem::path(a->name()).filename().string() <
                   std::filesystem::path(b->name()).filename().string();
        });

    // 5. Build Task List (deduplicated matchups)
    std::vector<std::tuple<const AlgoEntry*, const AlgoEntry*, const GameMapInfo*>> tasks;
    

    // Build all unique matchups deterministically
    for (size_t k = 0; k < K; ++k) {
        const GameMapInfo* map_ptr = &maps[k];
        for (size_t i = 0; i < N; ++i) {
            for (size_t j = i + 1; j < N; ++j) {
                tasks.emplace_back(algo_entries[i], algo_entries[j], map_ptr);
            }
        }
    }

    // Sort tasks by map and algorithm names
    std::sort(tasks.begin(), tasks.end(),
          [](const auto& a, const auto& b) {
              auto [e1a, e2a, mpa] = a;
              auto [e1b, e2b, mpb] = b;
              if (mpa->name != mpb->name) return mpa->name < mpb->name;
              if (e1a->name() != e1b->name()) return e1a->name() < e1b->name();
              return e2a->name() < e2b->name();
          });

    std::cout << "[INFO] Prepared " << tasks.size() << " unique matchups across "
              << maps.size() << " maps and " << algo_entries.size() << " algorithms.\n";    

    // 6. Create ThreadPool with capped threads
    int capped_threads = std::min(static_cast<int>(tasks.size()), static_cast<int>(num_threads));
    ThreadPool pool(capped_threads);
    std::cout << "[INFO] Allocating " << capped_threads << " threads for " << tasks.size() << " games.\n";

    std::mutex score_mutex;

    // 7. Enqueue tasks
    for (const auto& [entry1_ptr, entry2_ptr, map_ptr] : tasks) {
        const std::string a1 = entry1_ptr->name();
        const std::string a2 = entry2_ptr->name();

        //const GameMapInfo& map_info = *map_ptr;
        //pool.enqueue([&, entry1_ptr, entry2_ptr, a1, a2]() {

        const GameMapInfo* map_info = map_ptr;
        pool.enqueue([entry1_ptr, entry2_ptr, map_info, a1, a2, &gm_factory, &scores, &score_mutex, verbose]() {
            std::cout << "[THREAD] Running matchup " << a1 << " (P1) vs " << a2 << " (P2)"
                      << " on map " << map_info->name << " — thread: "
                      << std::this_thread::get_id() << "\n";

            auto gm_instance = gm_factory(verbose);

            auto* concrete_view = dynamic_cast<UserCommon_322719139_211961057::GameSatelliteView*>(map_info->view.get());

            if (!concrete_view) {
                std::cerr << "[Error] Expected GameSatelliteView for Player construction, got nullptr or wrong type.\n";
                return;
            }

            auto player1 = entry1_ptr->createPlayer(1, map_info->cols, map_info->rows,
                                                    map_info->max_steps, map_info->num_shells);
            auto player2 = entry2_ptr->createPlayer(2, map_info->cols, map_info->rows,
                                                    map_info->max_steps, map_info->num_shells);
            
            if (!player1 || !player2) {
                std::cerr << "[Error] Failed to create one or both players for " 
                        << a1 << " vs " << a2 << " on map " << map_info->name << "\n";
                return;
            }

            GameResult result = gm_instance->run(
                map_info->cols, map_info->rows,
                *map_info->view, map_info->name,
                map_info->max_steps, map_info->num_shells,
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
    }

    pool.wait();

    // 8. Sort and write results
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


std::vector<GameMapInfo> Simulator::loadGameMaps(const std::string& game_maps_folder) {
    std::vector<GameMapInfo> maps;

    if (!fs::exists(game_maps_folder) || !fs::is_directory(game_maps_folder)) {
        std::cerr << "[Error] Invalid maps folder: " << game_maps_folder << "\n";
        return maps;  // empty
    }

    for (const auto& entry : fs::directory_iterator(game_maps_folder)) {
        if (entry.is_regular_file()) {
            std::string filename = entry.path().string();

            auto maybe_map = loadGameMap(filename);
            if (!maybe_map) {
                std::cerr << "[WARNING] Skipping map file " << filename << " due to invalid or missing config fields.\n";
                continue;
            }

            if (!maybe_map->view) {
                std::cerr << "[WARNING] Skipping map file " << filename << " due to missing SatelliteView.\n";
                continue;
            }

            maps.push_back(std::move(*maybe_map));
        }

    }


    return maps;
}

bool Simulator::loadAlgorithms(const std::string& algorithms_folder, bool verbose) {
    algo_handles.clear();
    DynamicLoader dynamic_loader;

    if (!fs::exists(algorithms_folder) || !fs::is_directory(algorithms_folder)) {
        std::cerr << "[Simulator] Invalid algorithms folder: " << algorithms_folder << "\n";
        return false;
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
        std::cerr << "[Simulator] Not enough valid algorithms found in folder. Need at least 2.\n";
        return false;
    }

    if (verbose) {
        std::cout << "[Simulator] Total algorithms registered: "
                  << AlgorithmRegistrar::getAlgorithmRegistrar().count() << "\n";
    }
    return true;
}
