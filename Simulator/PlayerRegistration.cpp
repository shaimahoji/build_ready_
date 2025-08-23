#include "AlgorithmRegistrar.h"
#include "../common/BattleInfo.h"
#include "../common/ActionRequest.h"
#include "../common/TankAlgorithm.h"
#include "../common/SatelliteView.h"
#include "../common/Player.h"
#include "../common/PlayerRegistration.h"
//#include "AlgorithmRegistrar.h"

PlayerRegistration::PlayerRegistration(PlayerFactory factory) {
    auto& registrar = AlgorithmRegistrar::getAlgorithmRegistrar();
    registrar.addPlayerFactoryToLastEntry(std::move(factory));
}
