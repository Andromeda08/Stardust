#include <memory>
#include <Application/Application.hpp>

std::unique_ptr<sd::Application> g_application;
sd::Extent sd::Application::s_extent = {};

int main()
{
    sd::ApplicationOptions options;
    options.window_options.set_resolution(2560, 1440);

    sd::Application::s_extent = sd::Extent(options.window_options.width(), options.window_options.height());

    g_application = std::make_unique<sd::Application>(options);
    g_application->run();

    return 0;
}