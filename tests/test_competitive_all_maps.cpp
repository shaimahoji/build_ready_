#include <gtest/gtest.h>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <vector>
#include <string>

namespace fs = std::filesystem;

TEST(CompetitiveRun, RunCompetitionAllMapsAndAlgorithms) {
    std::string simulator = std::string(SOURCE_DIR) + "/build/Simulator/simulator_322719139_211961057";
    std::string gm_file = std::string(SOURCE_DIR) + "/game_managers_folder/GameManager_322719139_211961057.so";
    std::string algo_folder = std::string(SOURCE_DIR) + "/algorithms_folder";
    std::string map_folder = std::string(SOURCE_DIR) + "/maps";

    // Collect all algorithms
    std::vector<std::string> algorithms;
    for (const auto& entry : fs::directory_iterator(algo_folder)) {
        if (entry.is_regular_file() && entry.path().extension() == ".so") {
            algorithms.push_back(entry.path().string());
        }
    }

    ASSERT_GE(algorithms.size(), 2) << "Need at least 2 algorithms to run competition!";

    // Collect all maps
    std::vector<std::string> maps;
    for (const auto& entry : fs::directory_iterator(map_folder)) {
        if (entry.is_regular_file() && entry.path().extension() == ".txt") {
            maps.push_back(entry.path().string());
        }
    }

    ASSERT_GE(maps.size(), 1) << "No maps found in maps folder!";

    // Construct the command
    std::string command = simulator + " -competition "
                          "game_maps_folder=" + map_folder + " "
                          "game_manager=" + gm_file + " "
                          "algorithms_folder=" + algo_folder + " "
                          "num_threads=1 -verbose";

    std::cout << "[DEBUG] Running command: " << command << std::endl;

    // Run the simulator in competitive mode
    int ret = std::system(command.c_str());
    int exit_code = WEXITSTATUS(ret);
    EXPECT_EQ(exit_code, 0) << "Simulator returned an error in competitive mode";

    // Check output file exists
    bool found_output = false;
    for (const auto &entry : fs::directory_iterator(algo_folder)) {
        if (entry.is_regular_file() &&
            entry.path().filename().string().find("competition_") == 0) {
            found_output = true;
            std::cout << "[DEBUG] Found competition output file: " << entry.path() << std::endl;
            break;
        }
    }
    EXPECT_TRUE(found_output) << "No competition_<time>.txt file created";
}
