#pragma once

#include <span>
#include <ranges>
#include <cinttypes>

struct cache_file_lock {
    ~cache_file_lock();
    operator bool() const;
};

struct cache_range_lock {
    ~cache_range_lock();
    operator bool() const;
};

struct file_cache {
    file_cache();

    /**
     * @brief try_add_file returns a file if it exist or creates it if there is
     * enough space.
     * @param name the URI of the file
     * @return a handle to the file or empty
     */
    cache_file_lock try_get_file(std::span<char> name, bool &existed);

    unsigned &get_time(cache_file_lock &file);

    /**
     * @brief try_get_range returns the range with the given index if cached or
     * creates it if there is enough space.
     * @param file
     * @param index
     * @return
     */
    cache_range_lock try_get_range(
        cache_file_lock &file, unsigned index, bool &existed
    );

    std::ranges::subrange<std::uint8_t*> get_content(cache_range_lock &range);
};
