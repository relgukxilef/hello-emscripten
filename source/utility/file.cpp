#include "file.h"

#include <memory>
#include <exception>

void file_deleter::operator()(FILE*f) const { fclose(f); }

std::vector<uint8_t> read_file(const char* name) {
    std::unique_ptr<FILE, file_deleter> file(fopen(name, "rb"));
    if (!file.get())
        throw std::exception();
    fseek(file.get(), 0, SEEK_END);
    auto size = ftell(file.get());
    std::vector<uint8_t> content(size);
    fseek(file.get(), 0, SEEK_SET);
    fread(content.data(), sizeof(uint8_t), size, file.get());
    return content;
}
