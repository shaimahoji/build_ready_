#include "GameManagerLoader.h"
#include "GameManagerRegistrar.h"
#include <iostream>

bool GameManagerLoader::loadGameManagerLibrary(const std::string& path, const std::string& name, std::ostream& error_stream) {
    auto& registrar = GameManagerRegistrar::getGameManagerRegistrar();
    auto before = registrar.count();

    void* handle = dlopen(path.c_str(), RTLD_NOW);
    if (!handle) {
        error_stream << "[DLERROR] Failed to open GameManager: " << path << "\nError: " << dlerror() << std::endl;
        return false;
    }

    size_t after = registrar.count();
    //if (after != before + 1) {
    if (after <= before) {
        error_stream << "[REGISTRATION ERROR] GameManager not registered: " << name << std::endl;
        dlclose(handle);
        return false;
    }

    loadedGameManagers[name] = std::make_shared<HandleGuard>(handle, name, &error_stream);
    return true;
}

const std::unordered_map<std::string, std::shared_ptr<HandleGuard>>& GameManagerLoader::getHandles() const {
    return loadedGameManagers;
}
