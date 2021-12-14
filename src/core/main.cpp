#include <iostream>

#include <rapi.hpp>
#include <window.hpp>

auto main(int argc, char* argv[]) -> int {
    try {
        auto rapi = krypton::rapi::getRenderApi();
        rapi->init();
        while (true) {}
        rapi->shutdown();
    } catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
    }
    return 0;
}
