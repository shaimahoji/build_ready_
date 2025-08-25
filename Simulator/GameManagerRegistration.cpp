#include <dlfcn.h>
#include <filesystem>
#include <iostream>

#include "GameManagerRegistrar.h"
#include "../common/AbstractGameManager.h"
#include "../common/GameManagerRegistration.h"

GameManagerRegistration::GameManagerRegistration(GameManagerFactory factory) {
    auto& registrar = GameManagerRegistrar::getGameManagerRegistrar();

    Dl_info info;

    // Use a built-in address from inside this function to locate the .so being registered
    if (dladdr(__builtin_return_address(0), &info)) {
        std::string path(info.dli_fname);
        std::string so_name = std::filesystem::path(path).filename().string();
        std::cout << "[DEBUG] Registering GameManager with name: " << so_name << std::endl;
        registrar.addGameManagerFactory(std::move(factory), so_name);
    } else {
        std::cerr << "[ERROR] Could not resolve .so name for GameManager registration.\n";
    }
}
