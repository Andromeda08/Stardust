#include <memory>
#include <Application/Application.hpp>

int main()
{
    sd::ApplicationOptions options;
    options.window_options.set_resolution(1280, 720);

    auto app = std::make_unique<sd::Application>(options);
    app->run();

    return 0;
}