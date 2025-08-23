#pragma once

#include <string>
#include <unordered_map>
#include <memory>

#include "HandleGuard.h"

class DynamicLoader {
public:

    bool loadAlgorithmLibrary(const std::string& path, const std::string& name, std::ostream& error_stream);

    const std::unordered_map<std::string, std::shared_ptr<HandleGuard>>& getHandles() const {
        return loadedAlgorithms;
    }

private:
    std::unordered_map<std::string, std::shared_ptr<HandleGuard>> loadedAlgorithms;
};
