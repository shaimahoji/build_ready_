#pragma once

#include <string>
#include <vector>
#include <dlfcn.h>
#include <iostream>

class SharedLibraryLoader {
public:
    ~SharedLibraryLoader();

    bool loadLibrary(const std::string& path);
    void unloadAll();
    const std::vector<std::string>& getLoadedLibraries() const;

private:
    std::vector<void*> handles;
    std::vector<std::string> paths;
};
