#pragma once

#include <cstdio>
#include <vector>

struct file_deleter {
    void operator()(FILE*f) const;
};

std::vector<uint8_t> read_file(const char* name);
