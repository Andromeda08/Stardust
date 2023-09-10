#include "GraphCompileStrategy.hpp"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <format>
#include <string>

namespace Nebula::Editor
{
    void GraphCompileStrategy::write_logs_to_file(const std::string& file_name)
    {
        auto path = std::format("{}.txt", file_name);
        std::fstream fs(path);
        fs.open(path, std::ios_base::out);
        std::ostream_iterator<std::string> os_it(fs, "\n");
        std::copy(logs.begin(), logs.end(), os_it);
        fs.close();
    }
}
