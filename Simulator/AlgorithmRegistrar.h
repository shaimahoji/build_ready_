#pragma once

#include <functional>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <cassert>
#include <stdexcept>

#include "../common/ActionRequest.h" // Needed by TankAlgorithm.h
#include "../common/BattleInfo.h"    // Needed by TankAlgorithm.h
#include "../common/SatelliteView.h" // Needed by Player.h
#include "../common/TankAlgorithm.h" // Uses all above

#include "../common/Player.h"
//#include "../common/PlayerRegistration.h" // AFTER Player.h

class AlgorithmRegistrar {
private:
    static AlgorithmRegistrar registrar; // singleton instance

    class AlgorithmAndPlayerFactories {
        std::string so_name;
        TankAlgorithmFactory tankAlgorithmFactory = nullptr;
        PlayerFactory playerFactory = nullptr;

    public:
        AlgorithmAndPlayerFactories(const std::string& so_name);

        void setTankAlgorithmFactory(TankAlgorithmFactory&& factory);
        void setPlayerFactory(PlayerFactory&& factory);

        const std::string& name() const;
        const std::string& getName() const;

        std::unique_ptr<Player> createPlayer(int player_index, size_t x, size_t y, size_t max_steps, size_t num_shells) const;
        std::unique_ptr<TankAlgorithm> createTankAlgorithm(int player_index, int tank_index) const;

        bool hasPlayerFactory() const;
        bool hasTankAlgorithmFactory() const;
    };

    std::vector<AlgorithmAndPlayerFactories> algorithms;

public:
    static AlgorithmRegistrar& getAlgorithmRegistrar();

    void createAlgorithmFactoryEntry(const std::string& name);
    void addPlayerFactoryToLastEntry(PlayerFactory&& factory);
    void addTankAlgorithmFactoryToLastEntry(TankAlgorithmFactory&& factory);

    struct BadRegistrationException {
        std::string name;
        bool hasName, hasPlayerFactory, hasTankAlgorithmFactory;
    };

    void validateLastRegistration();
    void removeLast();
    void clear();

    auto begin() const { return algorithms.begin(); }
    auto end() const   { return algorithms.end(); }

    std::size_t count() const;
};