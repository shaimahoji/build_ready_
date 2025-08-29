#include <gtest/gtest.h>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

TEST(ComparativeRun, SingleMapSingleAlgorithm) {
    
    // Build absolute paths using SOURCE_DIR defined in CMake
    std::string simulator = std::string(SOURCE_DIR) + "/build/Simulator/simulator_322719139_211961057";
    std::string game_map = std::string(SOURCE_DIR) + "/game_map.txt";
    std::string gm_folder = std::string(SOURCE_DIR) + "/game_managers_folder";
    std::string algo1 = std::string(SOURCE_DIR) + "/algorithms_folder/Algorithm_322719139_211961057.so";
    std::string algo2 = algo1; // can be same for this test

    // Ensure simulator exists
    ASSERT_TRUE(fs::exists(simulator)) << "Simulator executable not found: " << simulator;
    ASSERT_TRUE(fs::exists(game_map)) << "Game map not found: " << game_map;
    ASSERT_TRUE(fs::exists(gm_folder)) << "GameManagers folder not found: " << gm_folder;
    ASSERT_TRUE(fs::exists(algo1)) << "Algorithm file not found: " << algo1;

    // Construct command
    std::string command = simulator + " -comparative "
                          "game_map=" + game_map + " "
                          "game_managers_folder=" + gm_folder + " "
                          "algorithm1=" + algo1 + " "
                          "algorithm2=" + algo2 + " "
                          "num_threads=1 -verbose";

    std::cout << "[DEBUG] Running command: " << command << std::endl;

    // Run the simulator
    int ret = std::system(command.c_str());
    int exit_code = WEXITSTATUS(ret);  // decode actual exit code
    EXPECT_EQ(exit_code, 0) << "Simulator returned an error";

    // Check that the output file exists (should start with "comparative_results_")
    bool found_output = false;
    for (const auto &entry : fs::directory_iterator(gm_folder)) {
        if (entry.is_regular_file() &&
            entry.path().filename().string().find("comparative_results_") == 0) {
            found_output = true;
            std::cout << "[DEBUG] Found output file: " << entry.path() << std::endl;
            break;
        }
    }
    
    EXPECT_TRUE(found_output) << "No comparative_results file created";
}
