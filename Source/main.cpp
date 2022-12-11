#include <memory>
#include "Application.hpp"

int main() {
    ApplicationSettings settings = {
        .logging = true,
        .raytracing = false,
        .windowSettings = {
            .title = "Vulkan Raytracing Application",
            .resolution = { 1760, 990 },
            .fullscreen = false,
            .resizable = false,
        }
    };

    auto app = std::make_unique<Application>(settings);

    app->run();

    return 0;
}
