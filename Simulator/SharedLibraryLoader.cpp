#include "SharedLibraryLoader.h"

bool SharedLibraryLoader::loadLibrary(const std::string& path) {
    void* handle = dlopen(path.c_str(), RTLD_NOW | RTLD_GLOBAL);
    if (!handle) {
        std::cerr << "Failed to load " << path << ": " << dlerror() << std::endl;
        return false;
    }

    handles.push_back(handle);
    paths.push_back(path);
    return true;
}

void SharedLibraryLoader::unloadAll() {
    for (size_t i = 0; i < handles.size(); ++i) {
        if (handles[i]) {
            if (dlclose(handles[i]) != 0) {
                std::cerr << "Warning: Failed to unload " << paths[i] << ": " << dlerror() << std::endl;
            }
        }
    }
    handles.clear();
    paths.clear();
}

SharedLibraryLoader::~SharedLibraryLoader() {
    unloadAll(); // Ensures cleanup at program end
}

const std::vector<std::string>& SharedLibraryLoader::getLoadedLibraries() const {
    return paths;
}
