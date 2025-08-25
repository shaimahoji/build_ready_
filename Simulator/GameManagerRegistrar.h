#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <iostream>

#include "../common/BattleInfo.h"
#include "../common/ActionRequest.h"
#include "../common/TankAlgorithm.h"
#include "../common/SatelliteView.h"
#include "../common/Player.h"
#include "../common/GameResult.h"
#include "../common/AbstractGameManager.h"

// Factory type: creates a new AbstractGameManager
using GameManagerFactory = std::function<std::unique_ptr<AbstractGameManager>(bool)>;

class GameManagerRegistrar {
    //std::unordered_map<std::string, GameManagerFactory> factories_;
    std::unique_ptr<std::unordered_map<std::string, GameManagerFactory>> factories_;
    static GameManagerRegistrar registrar_; // static singleton

public:
    GameManagerRegistrar();
    // Singleton accessor
    static GameManagerRegistrar& getGameManagerRegistrar();

    // Register with an explicit name (optional for UI or diagnostics)
    //void addGameManagerFactory(const std::string& name, GameManagerFactory factory) {
    //    factories_.emplace(name, std::move(factory));
    //}

    // Register with a generated name (used by REGISTER_GAME_MANAGER)
    //void addGameManagerFactory(GameManagerFactory factory);
    void addGameManagerFactory(GameManagerFactory factory, const std::string& name);

    // Lookup a factory by name
    GameManagerFactory getFactory(const std::string& name) const;

    // List all registered GameManager names
    std::vector<std::string> listRegistered() const;

    // Reset internal registry
    void clear();

    // How many factories are registered
    std::size_t count() const;
};
