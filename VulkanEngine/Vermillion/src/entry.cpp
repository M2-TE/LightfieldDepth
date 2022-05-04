#include "pch.hpp"
#include "core/application.hpp"

int main() {
    Application app;

    try {
        app.run();
    }
    catch (const std::exception& e) {

        VMI_LOG(e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}