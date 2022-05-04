#include "pch.hpp"
#include "core/application.hpp"

int main() {
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