#pragma once

#include <chrono>

namespace sd
{
    class Clock
    {
    public:
        Clock()
        {
            start();
        }

        void start()
        {
            m_begin = std::chrono::high_resolution_clock::now();
        }

        float ms() const
        {
            auto current_time = std::chrono::high_resolution_clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - m_begin).count();
            return (float) ms;
        }

    private:
        std::chrono::time_point<std::chrono::high_resolution_clock> m_begin;
    };
}