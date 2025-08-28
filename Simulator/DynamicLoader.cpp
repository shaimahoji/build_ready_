#include "DynamicLoader.h"
#include "AlgorithmRegistrar.h"
#include <iostream>
#include <cassert>

bool DynamicLoader::loadAlgorithmLibrary(const std::string& path, const std::string& name, std::ostream& error_stream) {
    auto& registrar = AlgorithmRegistrar::getAlgorithmRegistrar();
    // DO NOT call createAlgorithmFactoryEntry again here

    void* handle = dlopen(path.c_str(), RTLD_NOW);
    if (!handle) {
        error_stream << "[DLERROR] Failed to open: " << path << "\nError: " << dlerror() << std::endl;
        registrar.removeLast();  // Only valid if Simulator added entry
        return false;
    }

    try {
        registrar.validateLastRegistration();  // This should work as long as registration succeeded
        loadedAlgorithms[name] = std::make_shared<HandleGuard>(handle, name, &error_stream);
    } catch (const AlgorithmRegistrar::BadRegistrationException& e) {
        error_stream << "[REGISTRATION ERROR] " << name << "\n";
        error_stream << "  Tank factory?  " << std::boolalpha << e.hasTankAlgorithmFactory << "\n";
        error_stream << "  Player factory? " << std::boolalpha << e.hasPlayerFactory << "\n";
        dlclose(handle);
        registrar.removeLast();  // Clean up invalid entry
        return false;
    }

    return true;
}
