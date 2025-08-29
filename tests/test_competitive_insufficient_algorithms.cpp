#include <gtest/gtest.h>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

TEST(CompetitiveRun, CompetitiveInsufficientAlgorithms) {
    std::string simulator = std::string(SOURCE_DIR) + "/build/Simulator/simulator_322719139_211961057";
    std::string gm_file = std::string(SOURCE_DIR) + "/game_managers_folder/GameManager_322719139_211961057.so";
    std::string bad_algo_folder = std::string(SOURCE_DIR) + "/insufficient_algorithms";
    std::string map_folder = std::string(SOURCE_DIR) + "/maps";

    // Verify that the bad folder exists
    ASSERT_TRUE(fs::exists(bad_algo_folder)) 
        << "Missing test folder: " << bad_algo_folder;

    // Count algorithms
    size_t algo_count = 0;
    for (const auto& entry : fs::directory_iterator(bad_algo_folder)) {
        if (entry.is_regular_file() && entry.path().extension() == ".so") {
            algo_count++;
        }
    }
    ASSERT_LT(algo_count, 2) << "Test folder has 2+ algorithms; this test expects fewer!";

    // Construct the command
    std::string command = simulator + " -competition "
                          "game_maps_folder=" + map_folder + " "
                          "game_manager=" + gm_file + " "
                          "algorithms_folder=" + bad_algo_folder + " "
                          "num_threads=1 ";

    std::cout << "[DEBUG] Running command: " << command << std::endl;

    // Run simulator
    int ret = std::system(command.c_str());
    int exit_code = WEXITSTATUS(ret);

    // Expect non-zero exit code (failure due to insufficient algorithms)
    EXPECT_NE(exit_code, 0) << "Simulator unexpectedly succeeded with insufficient algorithms!";
}
