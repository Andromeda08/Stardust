#include <memory>
#include "Application.hpp"

int main() {
    ApplicationSettings settings = {
        .logging = true,
        .raytracing = true,
        .windowSettings = {}
    };

    auto app = std::make_unique<Application>(settings);

    app->run();

    return 0;
}
