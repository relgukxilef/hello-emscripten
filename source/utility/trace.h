#pragma once
#include <cstddef>
#include <source_location>

void start_trace(const char* filename, unsigned process_id);

struct scope_trace {
    scope_trace(const char *name, std::size_t line);
    scope_trace(
        std::source_location location = std::source_location::current()
    ) : scope_trace(location.function_name(), location.line()) {};
    ~scope_trace();

    std::size_t start_thread_id_hash;
    std::uint64_t start_time;
    std::size_t line;
    const char *name;
};
