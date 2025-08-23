
#include "GameManagerRegistrar.h"
#include "../common/AbstractGameManager.h"  // defines GameManagerFactory
#include "../common/GameManagerRegistration.h"


GameManagerRegistration::GameManagerRegistration(GameManagerFactory factory) {
    auto& registrar = GameManagerRegistrar::getGameManagerRegistrar();
    registrar.addGameManagerFactory(std::move(factory));
}
