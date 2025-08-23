#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>

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
    std::unordered_map<std::string, GameManagerFactory> factories;
    static GameManagerRegistrar registrar; // static singleton

public:
    // Singleton accessor
    static GameManagerRegistrar& getGameManagerRegistrar();

    // Add a factory with a name
    //void addGameManagerFactory(const std::string& name, GameManagerFactory factory);
    
    void addGameManagerFactory(const std::string& name, GameManagerFactory factory) {
        factories.emplace(name, std::move(factory));
    }

    void addGameManagerFactory(GameManagerFactory factory) {
    // Generate a unique name internally
    static int counter = 0;
    std::string name = "GameManager_" + std::to_string(counter++);
    factories.emplace(name, std::move(factory));
    }

    // Lookup a factory by name
    GameManagerFactory getFactory(const std::string& name) const;

    // List all registered names
    std::vector<std::string> listRegistered() const;

    void clear();
    std::size_t count() const;
};