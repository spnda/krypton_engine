#include <iostream>

#include <rapi.hpp>
#include <window.hpp>

auto main(int argc, char* argv[]) -> int {
    try {
        auto rapi = krypton::rapi::getRenderApi();
        rapi->init();
        while (!rapi->window.shouldClose()) {
            rapi->window.pollEvents();
            rapi->drawFrame();
        }
        rapi->shutdown();
    } catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
    }
    return 0;
}
