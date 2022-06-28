#include "pch.hpp"
#include "core/application.hpp"

int main() {

#ifdef _WIN32
    VMI_LOG("Windows-x64");
#else
    VMI_LOG("Linux-x64");
#endif
    DEBUG_ONLY(VMI_LOG("Debug build\n"));

    try {
        Application app;
        app.run();
    }
    catch (const std::exception& e) {

        VMI_LOG(e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}