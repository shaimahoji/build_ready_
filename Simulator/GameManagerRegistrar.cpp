#include "GameManagerRegistrar.h"

// Define the static singleton instance
GameManagerRegistrar GameManagerRegistrar::registrar;

GameManagerRegistrar& GameManagerRegistrar::getGameManagerRegistrar() {
    return registrar;
}

//void GameManagerRegistrar::addGameManagerFactory(const std::string& name, GameManagerFactory factory) {
//    factories[name] = std::move(factory);
//}

GameManagerFactory GameManagerRegistrar::getFactory(const std::string& name) const {
    auto it = factories.find(name);
    return (it != factories.end()) ? it->second : nullptr;
}

std::vector<std::string> GameManagerRegistrar::listRegistered() const {
    std::vector<std::string> names;
    for (auto& [key, _] : factories) {
        names.push_back(key);
    }
    return names;
}

void GameManagerRegistrar::clear() {
    factories.clear();
}

std::size_t GameManagerRegistrar::count() const {
    return factories.size();
}