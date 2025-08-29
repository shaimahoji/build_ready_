#include <gtest/gtest.h>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

TEST(ComparativeRun_MultiMap, GameManagerWithTwoAlgorithms) {
    std::cout << "\n[TEST INFO] Comparative Run Test: Multiple maps, 2 algorithms\n";
    std::cout << "[TEST INFO] This test checks:\n"
              << "  1. Simulator runs comparative mode on all maps\n"
              << "  2. Output files are created\n"
              << "  3. No crashes or errors\n\n";

    // Paths using SOURCE_DIR defined in CMake
    std::string simulator = std::string(SOURCE_DIR) + "/build/Simulator/simulator_322719139_211961057";
    std::string gm_folder = std::string(SOURCE_DIR) + "/game_managers_folder";
    std::string algo1 = std::string(SOURCE_DIR) + "/algorithms_folder/Algorithm_322719139_211961057.so";
    std::string algo2 = std::string(SOURCE_DIR) + "/algorithms_folder/Algorithm_322868852_340849710.so";
    std::string maps_folder = std::string(SOURCE_DIR) + "/maps";

    // Ensure simulator, folder, and algorithms exist
    ASSERT_TRUE(fs::exists(simulator)) << "Simulator executable not found: " << simulator;
    ASSERT_TRUE(fs::exists(gm_folder)) << "GameManagers folder not found: " << gm_folder;
    ASSERT_TRUE(fs::exists(algo1)) << "Algorithm1 file not found: " << algo1;
    ASSERT_TRUE(fs::exists(algo2)) << "Algorithm2 file not found: " << algo2;
    ASSERT_TRUE(fs::exists(maps_folder)) << "Maps folder not found: " << maps_folder;

    // Iterate all maps in folder
    for (const auto &map_entry : fs::directory_iterator(maps_folder)) {
        if (!map_entry.is_regular_file()) continue;

        std::string game_map = map_entry.path().string();
        std::cout << "[DEBUG] Running comparative on map: " << game_map << std::endl;

        std::string command = simulator + " -comparative "
                              "game_map=" + game_map + " "
                              "game_managers_folder=" + gm_folder + " "
                              "algorithm1=" + algo1 + " "
                              "algorithm2=" + algo2 + " "
                              "num_threads=1 -verbose";

        std::cout << "[DEBUG] Command: " << command << std::endl;

        // Run simulator
        int ret = std::system(command.c_str());
        int exit_code = WEXITSTATUS(ret);
        EXPECT_EQ(exit_code, 0) << "Simulator returned an error on map: " << game_map;

        // Check output file created
        bool found_output = false;
        for (const auto &entry : fs::directory_iterator(gm_folder)) {
            if (entry.is_regular_file() &&
                entry.path().filename().string().find("comparative_results_") == 0) {
                found_output = true;
                std::cout << "[DEBUG] Found output file: " << entry.path() << std::endl;
                break;
            }
        }
        EXPECT_TRUE(found_output) << "No comparative_results file created for map: " << game_map;
    }
}
