#include <gtest/gtest.h>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <vector>
#include <random>
#include <string>

namespace fs = std::filesystem;

TEST(ComparativeRun_RandomAlgorithms, MultipleMatches) {
    std::string algo_folder = std::string(SOURCE_DIR) + "/algorithms_folder";
    std::string gm_folder = std::string(SOURCE_DIR) + "/game_managers_folder";
    std::string game_map = std::string(SOURCE_DIR) + "/game_map.txt";
    std::string simulator = std::string(SOURCE_DIR) + "/build/Simulator/simulator_322719139_211961057";

    // Collect all .so files in Algorithm folder
    std::vector<std::string> algorithms;
    for (const auto& entry : fs::directory_iterator(algo_folder)) {
        if (entry.is_regular_file() && entry.path().extension() == ".so") {
            algorithms.push_back(entry.path().string());
        }
    }

    ASSERT_GE(algorithms.size(), 2) << algo_folder;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, algorithms.size() - 1);

    const int num_matches = 5; // number of matches to run

    for (int match = 1; match <= num_matches; ++match) {
        // Randomly select two different algorithms
        size_t idx1 = dis(gen);
        size_t idx2;
        do {
            idx2 = dis(gen);
        } while (idx2 == idx1);

        std::string algo1 = algorithms[idx1];
        std::string algo2 = algorithms[idx2];

        std::cout << "[INFO] Match " << match << " - Selected algorithms:\n"
                  << "  algo1: " << algo1 << "\n"
                  << "  algo2: " << algo2 << std::endl;

        // Construct the comparative command
        std::string command = simulator + " -comparative "
                              "game_map=" + game_map + " "
                              "game_managers_folder=" + gm_folder + " "
                              "algorithm1=" + algo1 + " "
                              "algorithm2=" + algo2 + " "
                              "num_threads=1 ";

        std::cout << "[DEBUG] Running command: " << command << std::endl;

        int ret = std::system(command.c_str());
        int exit_code = WEXITSTATUS(ret);
        EXPECT_EQ(exit_code, 0) << "Simulator returned an error for match " << match;

        // Check output file
        bool found_output = false;
        for (const auto &entry : fs::directory_iterator(gm_folder)) {
            if (entry.is_regular_file() &&
                entry.path().filename().string().find("comparative_results_") == 0) {
                found_output = true;
                std::cout << "[DEBUG] Found output file: " << entry.path() << std::endl;
                break;
            }
        }
        EXPECT_TRUE(found_output) << "No comparative_results file created for match " << match;
    }
}
