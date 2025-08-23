#include "HandleGuard.h"

HandleGuard::~HandleGuard() {
    if (handle) {
        if (dlclose(handle) != 0) {
            std::cerr << "[DLCLOSE ERROR] " << so_name << ": " << dlerror() << std::endl;
            if (diagnostics_stream_) {
                *diagnostics_stream_ << "[DLCLOSE ERROR] " << so_name << ": " << dlerror() << std::endl;
            }
        }
    }
}

HandleGuard::HandleGuard(void* h, std::string name, std::ostream* stream)
    : handle(h), so_name(std::move(name)), diagnostics_stream_(stream) {}
