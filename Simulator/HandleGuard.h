#pragma once

#include <string>
#include <dlfcn.h>
#include <iostream> // ADDED: Needed for std::cerr
#include <ostream>  // ADDED: Needed for std::ostream


// This utility ensures automatic dlclose for dynamically loaded .so handles.
// Used internally by DynamicLoader and GameManagerLoader only.

struct HandleGuard {
public:
    void* handle;
    std::string so_name;
    std::ostream* diagnostics_stream_ = nullptr;

    HandleGuard(void* h, std::string name, std::ostream* stream);
    ~HandleGuard();

};
