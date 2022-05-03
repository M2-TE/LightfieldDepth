#include "pch.hpp"

#include "core/application.hpp"

#define SYSTEM_PAUSE() system("pause")

int main() {
    Application app;

    try {

        app.run();
    }
    catch (const std::exception& e) {

        VMI_LOG(e.what());
        SYSTEM_PAUSE();
        return EXIT_FAILURE;
    }

    SYSTEM_PAUSE();
    return EXIT_SUCCESS;
}