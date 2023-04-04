#include <memory>
#include <Application/Application.hpp>

int main()
{
    sd::ApplicationOptions options;
    options.window_options.set_resolution(1920, 1080);

    auto app = std::make_unique<sd::Application>(options);
    app->run();

    return 0;
}