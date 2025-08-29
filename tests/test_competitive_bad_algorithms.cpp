#include <gtest/gtest.h>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

TEST(CompetitiveRun, test_competitive_bad_algorithms) {
    std::string simulator = std::string(SOURCE_DIR) + "/build/Simulator/simulator_322719139_211961057";
    std::string gm_file = std::string(SOURCE_DIR) + "/game_managers_folder/GameManager_322719139_211961057.so";
    std::string bad_algo_folder = std::string(SOURCE_DIR) + "/bad_sos_for_testing_purposes";
    std::string map_folder = std::string(SOURCE_DIR) + "/maps";

    // Make sure the bad folder exists
    ASSERT_TRUE(fs::exists(bad_algo_folder)) << "Missing folder: " << bad_algo_folder;

    // Collect .so files from bad folder
    std::vector<std::string> bad_algos;
    for (const auto &entry : fs::directory_iterator(bad_algo_folder)) {
        if (entry.is_regular_file() && entry.path().extension() == ".so") {
            bad_algos.push_back(entry.path().string());
        }
    }
    ASSERT_FALSE(bad_algos.empty()) << "No .so files found in bad algorithms folder!";

    // Collect maps
    ASSERT_TRUE(fs::exists(map_folder)) << "Maps folder missing!";
    std::vector<std::string> maps;
    for (const auto &entry : fs::directory_iterator(map_folder)) {
        if (entry.is_regular_file() && entry.path().extension() == ".txt") {
            maps.push_back(entry.path().string());
        }
    }
    ASSERT_FALSE(maps.empty()) << "No maps found in maps folder!";

    // Construct the command
    std::string command = simulator + " -competition "
                          "game_maps_folder=" + map_folder + " "
                          "game_manager=" + gm_file + " "
                          "algorithms_folder=" + bad_algo_folder + " "
                          "num_threads=1 -verbose";

    std::cout << "[DEBUG] Running command with BAD algorithms: " << command << std::endl;

    // Run simulator and EXPECT a failure exit code
    int ret = std::system(command.c_str());
    int exit_code = WEXITSTATUS(ret);

    EXPECT_NE(exit_code, 0) << "Simulator should fail when using bad algorithms!";
}
