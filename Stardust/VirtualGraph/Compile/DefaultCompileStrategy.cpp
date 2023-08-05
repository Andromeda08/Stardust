#include "DefaultCompileStrategy.hpp"
#include <sstream>
#include <Benchmarking.hpp>
#include <VirtualGraph/Node.hpp>
#include <VirtualGraph/Compile/Algorithm/TopologicalSort.hpp>

namespace Nebula::Editor
{
    CompileResult DefaultCompileStrategy::compile(const std::vector<std::shared_ptr<Node>>& nodes)
    {
        CompileResult result = {};

        std::chrono::milliseconds tsort_time;
        auto topological_sort = std::make_unique<TopologicalSort>(nodes);
        std::vector<std::shared_ptr<Node>> topological_ordering;
        try
        {
            tsort_time = sd::bm::measure<std::chrono::milliseconds>([&](){
                topological_ordering = topological_sort->execute();
            });
        }
        catch (const std::runtime_error& ex)
        {
            result.success = false;
            result.logs.push_back(ex.what());
            result.failure_message = logs.back();
            return result;
        }

        std::stringstream tsort_log;
        tsort_log << std::format("[Info] Generated topological ordering ({}ms):", tsort_time.count());
        for (const auto& node : topological_ordering)
        {
            tsort_log << std::format(" [{}]", node->name());
        }
        logs.push_back(tsort_log.str());

        result.logs = logs;
        result.success = true;
        return result;
    }
}