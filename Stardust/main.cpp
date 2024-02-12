#include <fstream>
#include <memory>
#include <nlohmann/json.hpp>
#include <Application/Application.hpp>
#include <Application/MeshShaderApp.hpp>

std::unique_ptr<sd::Application> g_application;
sd::Extent sd::Application::s_extent = {};

int main()
{
    sd::ApplicationOptions options;
    options.window_options.set_resolution(sd::Resolution::e1600x900);
    //options.window_options.set_resolution(2240, 1260);
    options.window_options.set_title("Nebula");

    sd::Application::s_extent = sd::Extent(options.window_options.width(), options.window_options.height());

    g_application = std::make_unique<sd::Application>(options);
    g_application->run();

    // const auto mesh_shader_app = std::make_unique<Nebula::MeshShaderApp>();
    // mesh_shader_app->run();

    return 0;
}