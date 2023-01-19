#include <memory>
#include "Application.hpp"

int main() {
    ApplicationSettings settings = {
        .logging = true,
        .raytracing = true,
        .windowSettings = {}
    };

    settings.windowSettings.setSize(1920, 1080);

    auto app = std::make_unique<Application>(settings);

    app->run();

    return 0;
}
