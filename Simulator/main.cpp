#include "Simulator.h"

#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <filesystem>
#include <fstream>
#include <algorithm>

namespace fs = std::filesystem;

void print_usage_and_exit(const std::string& error_msg) {
    std::cerr << "Error: " << error_msg << "\n";
    std::cerr << "\nUsage for comparative mode:\n";
    std::cerr << "./simulator_<ids> -comparative game_map=<file> game_managers_folder=<folder> algorithm1=<file> algorithm2=<file> [num_threads=<int>] [-verbose]\n";
    std::cerr << "\nUsage for competition mode:\n";
    std::cerr << "./simulator_<ids> -competition game_maps_folder=<folder> game_manager=<file> algorithms_folder=<folder> [num_threads=<int>] [-verbose]\n";
    exit(1);
}

std::pair<std::string, std::string> parse_argument(const std::string& arg) {
    auto pos = arg.find('=');
    if (pos == std::string::npos)
        return {arg, ""};
    
    std::string key = arg.substr(0, pos);
    std::string value = arg.substr(pos + 1);
    key.erase(std::remove_if(key.begin(), key.end(), ::isspace), key.end());
    value.erase(std::remove_if(value.begin(), value.end(), ::isspace), value.end());
    return {key, value};
}

bool file_exists(const std::string& path) {
    return fs::exists(path) && fs::is_regular_file(path);
}

bool valid_folder_with_files(const std::string& path) {
    if (!fs::exists(path) || !fs::is_directory(path)) return false;
    for (const auto& entry : fs::directory_iterator(path)) {
        if (entry.is_regular_file()) return true;
    }
    return false;
}

int main(int argc, char* argv[]) {
    std::unordered_map<std::string, std::string> args;
    std::unordered_set<std::string> flags;
    std::vector<std::string> unsupported;
    std::string mode;

    std::ofstream diagnostics_file("diagnostics.txt");
    if (!diagnostics_file.is_open()) {
        std::cerr << "Warning: Could not open diagnostics.txt for writing.\n";
    }

    for (int i = 1; i < argc; ++i) {
        std::string token = argv[i];
        if (token == "-comparative" || token == "-competition") {
            if (!mode.empty()) {
                print_usage_and_exit("Multiple modes specified");
            }
            mode = token;
        } else if (token.find('=') != std::string::npos) {
            auto [key, val] = parse_argument(token);
            if (key.empty() || val.empty()) {
                print_usage_and_exit("Invalid format for argument: " + token);
            }
            args[key] = val;
        } else if (token == "-verbose") {
            flags.insert("verbose");
        } else {
            // Collect unsupported tokens (anything that wasn't a mode, -verbose, or k=v)
            if (token != "-comparative" && token != "-competition" && token != "-verbose" && token.find('=') == std::string::npos) {
                unsupported.push_back(token);
            }
        }
    }

    // Emit one usage that lists *all* unsupported args — just after the loop
    if (!unsupported.empty()) {
        std::string msg = "Unsupported command line arguments: ";
        for (size_t i = 0; i < unsupported.size(); ++i) {
            msg += unsupported[i];
            if (i + 1 < unsupported.size()) msg += ", ";
        }
        print_usage_and_exit(msg);
    }

    if (mode.empty()) {
        print_usage_and_exit("Missing mode (-comparative or -competition)");
    }

    // Validate required args per mode
    if (mode == "-comparative") {
        std::vector<std::string> required = {"game_map", "game_managers_folder", "algorithm1", "algorithm2"};
        std::vector<std::string> missing;
        for (const auto& key : required) if (!args.count(key)) missing.push_back(key);
        if (!missing.empty()) {
            std::string msg = "Missing required arguments: ";
            for (size_t i = 0; i < missing.size(); ++i) {
                msg += missing[i];
                if (i + 1 < missing.size()) msg += ", ";
            }
            print_usage_and_exit(msg);
        }
        if (!file_exists(args["game_map"]))
            print_usage_and_exit("game_map file does not exist or cannot be opened");
        if (!valid_folder_with_files(args["game_managers_folder"]))
            print_usage_and_exit("Invalid or empty game_managers_folder");
        if (!file_exists(args["algorithm1"]))
            print_usage_and_exit("algorithm1 file does not exist or cannot be opened");
        if (!file_exists(args["algorithm2"]))
            print_usage_and_exit("algorithm2 file does not exist or cannot be opened");
    } else if (mode == "-competition") {
        std::vector<std::string> required = {"game_maps_folder", "game_manager", "algorithms_folder"};
        std::vector<std::string> missing;
        for (const auto& key : required) if (!args.count(key)) missing.push_back(key);
        if (!missing.empty()) {
            std::string msg = "Missing required arguments: ";
            for (size_t i = 0; i < missing.size(); ++i) {
                msg += missing[i];
                if (i + 1 < missing.size()) msg += ", ";
            }
            print_usage_and_exit(msg);
        }
        if (!valid_folder_with_files(args["game_maps_folder"]))
            print_usage_and_exit("Invalid or empty game_maps_folder");
        if (!file_exists(args["game_manager"]))
            print_usage_and_exit("game_manager file does not exist or cannot be opened");
        if (!valid_folder_with_files(args["algorithms_folder"]))
            print_usage_and_exit("Invalid or empty algorithms_folder");
    }

    // Optional args
    int num_threads = 1;
    // Tighten `num_threads` check
    if (args.count("num_threads")) {
        try {
            num_threads = std::stoi(args["num_threads"]);
            if (num_threads < 1) throw std::invalid_argument("num_threads < 1");
        } catch (const std::exception&) {
            print_usage_and_exit("Invalid value for num_threads: must be integer >= 1");
        }
    }

    bool verbose = flags.count("verbose") > 0;

    // ==========================
    // Dispatch to actual logic
    // ==========================
    std::cout << "Running in mode: " << mode << "\n";
    for (const auto& [k, v] : args) {
        std::cout << k << " = " << v << "\n";
    }
    std::cout << "num_threads = " << num_threads << (verbose ? " (verbose enabled)\n" : "\n");

    Simulator sim;
    if (mode == "-comparative") {
        sim.runComparative(
            args["game_map"],
            args["algorithm1"],
            args["algorithm2"],
            args["game_managers_folder"],
            num_threads,
            verbose
        );
    } else if (mode == "-competition") {
        sim.runCompetitive(
            args["algorithms_folder"],
            args["game_maps_folder"],
            args["game_manager"],
            num_threads,
            verbose
        );
    }

    return 0;
}
