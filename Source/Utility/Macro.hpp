#pragma once

#define NON_COPIABLE(T) \
    T(T const&) = delete; \
    T& operator=(T const&) = delete;
