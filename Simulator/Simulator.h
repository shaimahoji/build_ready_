#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <map>
#include <mutex>
#include <set>
#include <unordered_set>

#include "../common/BattleInfo.h"
#include "../common/ActionRequest.h"
#include "../common/TankAlgorithm.h"
#include "../common/SatelliteView.h"
#include "../common/Player.h"
#include "../common/GameResult.h"
#include "../common/AbstractGameManager.h"

#include "../UserCommon/GameSatelliteView.h"

struct GameMapInfo {
    std::string name;
    size_t max_steps;
    size_t num_shells;
    size_t rows;
    size_t cols;
    size_t x1 = 0, y1 = 0;
    size_t x2 = 0, y2 = 0;
    std::unique_ptr<SatelliteView> view;
    //std::unique_ptr<UserCommon_322719139_211961057::GameSatelliteView> view;
    //std::unique_ptr<GameSatelliteView> view;
    //std::unique_ptr<SatelliteView> view;
    std::map<int, std::vector<std::pair<size_t, size_t>>> player_tank_positions;
};

class Simulator {
private:
    std::mutex results_mutex;
    std::vector<void*> gm_handles;   // dlopen handles for GameManagers
    std::vector<void*> algo_handles; // dlopen handles for Algorithms
    std::vector<std::string> algo_paths; // Paths to the loaded algorithm .so files

    // Example storage for results (define your own struct if needed)
    //std::vector<GameResult> runResults;
    std::vector<std::pair<std::string, GameResult>> run_results;

    //internal helper
    GameMapInfo loadGameMap(const std::string& filename);
    std::string generateFilename(const std::string& prefix, const std::string& folder) const;

public:
    ~Simulator();

    void runComparative(
        const std::string& game_map_path,
        const std::string& algo1_path,
        const std::string& algo2_path,
        const std::string& gameManagers_folder,
        int num_threads,
        bool verbose
    );

    void loadGameManagers(const std::string& folder_path, bool verbose);

    //void loadAlgorithms(const std::string& algorithms_folder, bool verbose);
    void loadTwoAlgorithms(const std::string& algo1_path, const std::string& algo2_path, bool verbose);

    GameResult runSingleGame(const std::string &gmName, const GameMapInfo& mapInfo, const std::string& alg1, const std::string& alg2, bool verbose);
    using GroupedResults = std::map<std::tuple<std::string, size_t, std::string>, std::vector<std::string>>;
    GroupedResults groupResults(const GameMapInfo& map_info);

    void writeComparativeOutput(
        const std::string& output_folder,
        const std::string& game_map_name,
        const std::string& algo1_name,
        const std::string& algo2_name,
        const GroupedResults& grouped
    );

    std::string serializeMap(const SatelliteView& view, size_t width, size_t height);
    std::string formatResultMessage(const GameResult& r);

    std::string generateTimestamp() const;

    //void runCompetitive(const std::string &algorithms_folder, const std::string &game_maps_folder, const std::string &game_manager_so, size_t num_threads, bool verbose);
    void runCompetitive(
        const std::string& algorithms_folder,
        const std::string& game_maps_folder,
        const std::string& game_manager_so,
        size_t num_threads,
        bool verbose);

        

    std::vector<GameMapInfo> loadGameMaps(const std::string& game_maps_folder);

    void loadAlgorithms(const std::string& algorithms_folder, bool verbose);

};

