#include "pch.hpp"
#include "core/application.hpp"

int main() {

    DEBUG_ONLY(VMI_LOG("Debug build.\n"));

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