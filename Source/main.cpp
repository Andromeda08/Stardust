#include <memory>
#include "Core/Application.hpp"

int main() {
    ApplicationSettings settings = { true, {} };

    auto app = std::make_unique<Application>(settings);

    app->run();

    return 0;
}
