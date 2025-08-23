/* Defines the static registrar instance and returns a reference to it when asked */

#include "AlgorithmRegistrar.h"

AlgorithmRegistrar AlgorithmRegistrar::registrar;

AlgorithmRegistrar& AlgorithmRegistrar::getAlgorithmRegistrar() {
    return registrar;
}
