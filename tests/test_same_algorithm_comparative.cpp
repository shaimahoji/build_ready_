#include <gtest/gtest.h>
#include <cstdlib>
#include <string>
#include <iostream>

TEST(SimulatorComparative, RunComparativeTest) {
    std::cout << "Running the first test\n";
    std::string simulator = std::string(SOURCE_DIR) + "/build/Simulator/simulator_322719139_211961057";

    std::string command = 
        simulator + " "
        "-comparative "
        "game_map=" + std::string(SOURCE_DIR) + "/game_map.txt "
        "game_managers_folder=" + std::string(SOURCE_DIR) + "/game_managers_folder "
        "algorithm1=" + std::string(SOURCE_DIR) + "/algorithms_folder/Algorithm_322719139_211961057.so "
        "algorithm2=" + std::string(SOURCE_DIR) + "/algorithms_folder/Algorithm_322719139_211961057.so "
        "num_threads=1 "
        "-verbose";

    std::cout << "[DEBUG] Running command: " << command << std::endl;

    int ret = std::system(command.c_str());
    EXPECT_EQ(ret, 0);
}
