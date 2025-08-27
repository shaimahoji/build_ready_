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

// Inline file/folder validation
bool fileExists(const std::string& path) {
    std::ifstream f(path);
    return f.good();
}

bool folderExists(const std::string& path) {
    return fs::exists(path) && fs::is_directory(path);
}

bool folderHasFiles(const std::string& path) {
    if (!folderExists(path)) return false;
    for (const auto& entry : fs::directory_iterator(path)) {
        if (fs::is_regular_file(entry)) return true;
    }
    return false;
}

// Improved usage printer — handles all 4 error types cleanly
int printUsageAndExit(const std::vector<std::string>& errors, const std::string& mode,
                      const std::string& id1, const std::string& id2) {
    for (const std::string& err : errors) {
        std::cerr << "Error: " << err << "\n";
    }

    std::cerr << "\n--- Usage ---\n";
    std::string exe = "./simulator_" + id1 + "_" + id2;

    if (mode == "-comparative") {
        std::cerr << exe << " -comparative \\\n"
                  << "    game_map=<file> \\\n"
                  << "    game_managers_folder=<folder> \\\n"
                  << "    algorithm1=<file> \\\n"
                  << "    algorithm2=<file> \\\n"
                  << "    [num_threads=<int>] [-verbose]\n";
    } else if (mode == "-competition") {
        std::cerr << exe << " -competition \\\n"
                  << "    game_maps_folder=<folder> \\\n"
                  << "    game_manager=<file> \\\n"
                  << "    algorithms_folder=<folder> \\\n"
                  << "    [num_threads=<int>] [-verbose]\n";
    } else {
        std::cerr << exe << " -comparative [...]\n"
                  << exe << " -competition [...]\n";
    }

    std::cerr << "\nNotes:\n"
              << "  • All required arguments must be provided.\n"
              << "  • All file/folder paths must be valid and readable.\n";

    return 1;
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

int main(int argc, char* argv[]) {
    std::unordered_map<std::string, std::string> args;
    std::unordered_set<std::string> flags;
    std::vector<std::string> unsupported;
    std::string mode;
    //////////////////////////////
    std::string id1 = "ID1";
    std::string id2 = "";
    std::string exe_filename = fs::path(argv[0]).filename().string();
    std::string prefix = "simulator_";
    if (exe_filename.rfind(prefix, 0) == 0) {
        std::string id_part = exe_filename.substr(prefix.size());
        size_t underscore_pos = id_part.find('_');
        if (underscore_pos != std::string::npos) {
            id1 = id_part.substr(0, underscore_pos);
            id2 = id_part.substr(underscore_pos + 1);
        } else {
            id1 = id_part;
        }
    }
    //////////////////////////////

    std::ofstream diagnostics_file("diagnostics.txt");
    if (!diagnostics_file.is_open()) {
        std::cerr << "Warning: Could not open diagnostics.txt for writing.\n";
    }

    for (int i = 1; i < argc; ++i) {
        std::string token = argv[i];
        if (token == "-comparative" || token == "-competition") {
            if (!mode.empty()) {
                return printUsageAndExit({"Multiple modes specified"}, mode, id1, id2);
            }
            mode = token;
        } else if (token.find('=') != std::string::npos) {
            auto [key, val] = parse_argument(token);
            if (key.empty() || val.empty()) {
                return printUsageAndExit({"Invalid format for argument: " + token}, mode, id1, id2);
            }
            args[key] = val;
        } else if (token == "-verbose") {
            flags.insert("verbose");
        } else {
            unsupported.push_back(token);
        }
    }

    if (!unsupported.empty()) {
        std::string msg = "Unsupported command line arguments: ";
        for (size_t i = 0; i < unsupported.size(); ++i) {
            msg += unsupported[i];
            if (i + 1 < unsupported.size()) msg += ", ";
        }
        return printUsageAndExit({msg}, mode, id1, id2);
    }

    if (mode.empty()) {
        return printUsageAndExit({"Missing mode (-comparative or -competition)"}, "", id1, id2);
    }

    std::vector<std::string> errors;

    if (mode == "-comparative") {
        std::vector<std::string> required = {"game_map", "game_managers_folder", "algorithm1", "algorithm2"};
        for (const auto& key : required) {
            if (!args.count(key)) errors.push_back("Missing required argument: " + key);
        }

        if (args.count("game_map") && !fileExists(args["game_map"]))
            errors.push_back("Cannot open game_map file: " + args["game_map"]);

        if (args.count("game_managers_folder") && !folderHasFiles(args["game_managers_folder"]))
            errors.push_back("Invalid or empty game_managers_folder: " + args["game_managers_folder"]);

        if (args.count("algorithm1") && !fileExists(args["algorithm1"]))
            errors.push_back("Cannot open algorithm1 file: " + args["algorithm1"]);

        if (args.count("algorithm2") && !fileExists(args["algorithm2"]))
            errors.push_back("Cannot open algorithm2 file: " + args["algorithm2"]);
    }

    else if (mode == "-competition") {
        std::vector<std::string> required = {"game_maps_folder", "game_manager", "algorithms_folder"};
        for (const auto& key : required) {
            if (!args.count(key)) errors.push_back("Missing required argument: " + key);
        }

        if (args.count("game_maps_folder") && !folderHasFiles(args["game_maps_folder"]))
            errors.push_back("Invalid or empty game_maps_folder: " + args["game_maps_folder"]);

        if (args.count("game_manager") && !fileExists(args["game_manager"]))
            errors.push_back("Cannot open game_manager file: " + args["game_manager"]);

        if (args.count("algorithms_folder") && !folderHasFiles(args["algorithms_folder"]))
            errors.push_back("Invalid or empty algorithms_folder: " + args["algorithms_folder"]);
    }

    // Optional argument validation
    int num_threads = 1;
    if (args.count("num_threads")) {
        try {
            num_threads = std::stoi(args["num_threads"]);
            if (num_threads < 1) throw std::invalid_argument("num_threads < 1");
        } catch (const std::exception&) {
            errors.push_back("Invalid value for num_threads: must be integer >= 1");
        }
    }

    // If anything failed:
    if (!errors.empty()) {
        return printUsageAndExit(errors, mode, id1, id2);
    }

    bool verbose = flags.count("verbose") > 0;

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
        std::cout << "[Main] Comparative simulation completed.\n";
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
