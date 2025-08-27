#include "AlgorithmRegistrar.h"
#include <iostream>

// ----------------------------
// Singleton instance
// ----------------------------
AlgorithmRegistrar AlgorithmRegistrar::registrar;

AlgorithmRegistrar& AlgorithmRegistrar::getAlgorithmRegistrar() {
    std::cout << "[DEBUG] getAlgorithmRegistrar called\n";
    return registrar;
}


// ----------------------------
// AlgorithmAndPlayerFactories methods
// ----------------------------

AlgorithmRegistrar::AlgorithmAndPlayerFactories::AlgorithmAndPlayerFactories(const std::string& so_name)
    : so_name(so_name) {}

void AlgorithmRegistrar::AlgorithmAndPlayerFactories::setTankAlgorithmFactory(TankAlgorithmFactory&& factory) {
    assert(tankAlgorithmFactory == nullptr);
    tankAlgorithmFactory = std::move(factory);
}

void AlgorithmRegistrar::AlgorithmAndPlayerFactories::setPlayerFactory(PlayerFactory&& factory) {
    assert(playerFactory == nullptr);
    playerFactory = std::move(factory);
}

const std::string& AlgorithmRegistrar::AlgorithmAndPlayerFactories::name() const {
    return so_name;
}

const std::string& AlgorithmRegistrar::AlgorithmAndPlayerFactories::getName() const {
    return so_name;
}

std::unique_ptr<Player> AlgorithmRegistrar::AlgorithmAndPlayerFactories::createPlayer(
    int player_index, size_t x, size_t y, size_t max_steps, size_t num_shells) const {
    return playerFactory(player_index, x, y, max_steps, num_shells);
}

std::unique_ptr<TankAlgorithm> AlgorithmRegistrar::AlgorithmAndPlayerFactories::createTankAlgorithm(
    int player_index, int tank_index) const {
    return tankAlgorithmFactory(player_index, tank_index);
}

bool AlgorithmRegistrar::AlgorithmAndPlayerFactories::hasPlayerFactory() const {
    return playerFactory != nullptr;
}

bool AlgorithmRegistrar::AlgorithmAndPlayerFactories::hasTankAlgorithmFactory() const {
    return tankAlgorithmFactory != nullptr;
}

// ----------------------------
// AlgorithmRegistrar methods
// ----------------------------

void AlgorithmRegistrar::createAlgorithmFactoryEntry(const std::string& name) {
    std::cout << "[DEBUG] Creating AlgorithmFactoryEntry for: " << name << "\n";

    for (const auto& entry : algorithms) {
        std::cout << "[DEBUG] Comparing against: " << entry.name() << "\n";
        if (entry.name() == name) {
            std::cerr << "[FATAL] Duplicate algorithm registration name: " << name << "\n";
            throw std::runtime_error("Duplicate algorithm .so name: " + name);
        }
    }

    algorithms.emplace_back(name);
}


void AlgorithmRegistrar::addPlayerFactoryToLastEntry(PlayerFactory&& factory) {
    std::cout << "[DEBUG] Adding PlayerFactory\n";
    if (algorithms.empty()) {
        std::cerr << "[ERROR] Cannot add PlayerFactory — no AlgorithmFactoryEntry exists.\n";
    }
    else {
        std::cout << "[DEBUG] Adding PlayerFactory\n";
        algorithms.back().setPlayerFactory(std::move(factory));
    }
}

void AlgorithmRegistrar::addTankAlgorithmFactoryToLastEntry(TankAlgorithmFactory&& factory) {
    std::cout << "[DEBUG] Adding TankAlgorithmFactory\n";
    algorithms.back().setTankAlgorithmFactory(std::move(factory));
}

void AlgorithmRegistrar::validateLastRegistration() {
    std::cout << "[DEBUG] Validating last registration...\n";
    const auto& last = algorithms.back();
    bool hasName = (last.name() != "");
    std::cout << "[DEBUG] Registration details → name: " << last.name()
              << ", hasPlayerFactory: " << last.hasPlayerFactory()
              << ", hasTankFactory: " << last.hasTankAlgorithmFactory() << "\n";

    if (!hasName || !last.hasPlayerFactory() || !last.hasTankAlgorithmFactory()) {
        throw BadRegistrationException{
            .name = last.name(),
            .hasName = hasName,
            .hasPlayerFactory = last.hasPlayerFactory(),
            .hasTankAlgorithmFactory = last.hasTankAlgorithmFactory()
        };
    }

    std::cout << "[DEBUG] Registration validated.\n";
}

void AlgorithmRegistrar::removeLast() {
    algorithms.pop_back();
}

void AlgorithmRegistrar::clear() {
    algorithms.clear();
}

std::size_t AlgorithmRegistrar::count() const {
    return algorithms.size();
}
