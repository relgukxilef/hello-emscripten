#include "trace.h"
#include <cstdio>
#include <thread>
#include <inttypes.h>

unsigned process_id;
FILE *trace_file;
std::uint64_t last_event = 0;

void start_trace(const char *filename, unsigned int process_id) {
    trace_file = fopen(filename, "wb");
    fputs("[", trace_file);
    ::process_id = process_id;
}

std::uint64_t precise_time() {
    auto now = std::chrono::high_resolution_clock::now();
    auto epoch = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::microseconds>(epoch).count();
}

scope_trace::scope_trace(const char* name, std::size_t line) {
    if (!trace_file)
        return;
    this->name = name;
    this->line = line;
    start_time = std::max(precise_time(), last_event + 1); // disambiguation
    last_event = start_time;
    start_thread_id_hash =
        std::hash<std::thread::id>()(std::this_thread::get_id());
}

scope_trace::~scope_trace() {
    if (!trace_file)
        return;
    fprintf(
        trace_file,
        "{\"name\":\"%s:%zu\",\"ph\":\"X\",\"pid\":%u,\"tid\":%zu,"
        "\"ts\":%llu,\"dur\":%llu},",
        name,
        line,
        process_id,
        std::hash<std::thread::id>()(std::this_thread::get_id()),
        (long long unsigned)start_time,
        (long long unsigned)precise_time() - start_time
    );
}
