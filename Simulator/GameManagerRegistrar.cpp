#include "GameManagerRegistrar.h"

// Define the static singleton instance
GameManagerRegistrar GameManagerRegistrar::registrar_;

GameManagerRegistrar::GameManagerRegistrar() {
    std::cout << "[DEBUG] GameManagerRegistrar constructor called\n";
    factories_ = std::make_unique<std::unordered_map<std::string, GameManagerFactory>>();
    std::cout << "[DEBUG] GameManagerRegistrar constructor done!\n";
}

GameManagerRegistrar& GameManagerRegistrar::getGameManagerRegistrar() {
    //static GameManagerRegistrar instance;
    //return instance;
    std::cout << "[DEBUG] getGameManagerRegistrar called\n";
    return registrar_;
}

GameManagerFactory GameManagerRegistrar::getFactory(const std::string& name) const {
    auto it = factories_->find(name);
    return (it != factories_->end()) ? it->second : nullptr;
}

std::vector<std::string> GameManagerRegistrar::listRegistered() const {
    std::vector<std::string> names;
    //for (const auto& [key, _] : factories_) {
    for (const auto& [key, _] : *factories_) {
        names.push_back(key);
    }
    return names;
}

void GameManagerRegistrar::clear() {
    factories_->clear();
}

std::size_t GameManagerRegistrar::count() const {
    return factories_->size();
}

void GameManagerRegistrar::addGameManagerFactory(GameManagerFactory factory) {
    std::cout << "[DEBUG] Entered addGameManagerFactory\n";
    static int counter = 0;
    std::cout << "[DEBUG] Before to_string\n";
    std::string name = "GameManager_" + std::to_string(counter++);
    std::cout << "[DEBUG] After to_string\n";

    //factories_.emplace(name, std::move(factory));

    //if (&factories_ == nullptr) {
    if (!factories_) {
        std::cout << "[DEBUG] factories_ was null, initializing now\n";
        factories_ = std::make_unique<std::unordered_map<std::string, GameManagerFactory>>();
        std::cout << "[DEBUG] factories_ initialized successfully\n";

    } else {
        std::cout << "[DEBUG] factories_ is valid, about to emplace\n";
        //factories_.emplace(name, std::move(factory));
        auto& map = *factories_;
map.emplace(name, std::move(factory));
        std::cout << "[DEBUG] After factories_.emplace\n";
    }

    std::cout << "[DEBUG] After factories_.emplace\n";
}