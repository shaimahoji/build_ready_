#include "AlgorithmRegistrar.h"
#include "../common/TankAlgorithmRegistration.h"


TankAlgorithmRegistration::TankAlgorithmRegistration(TankAlgorithmFactory factory) {
    auto& registrar = AlgorithmRegistrar::getAlgorithmRegistrar();
    registrar.addTankAlgorithmFactoryToLastEntry(std::move(factory));
}
