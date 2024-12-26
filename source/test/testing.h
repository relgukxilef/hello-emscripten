#pragma once

#include <vector>
#include <functional>
#include <utility>
#include <cinttypes>
#include <span>
#include <concepts>
#include <optional>
#include <limits>

namespace testing {
    using namespace std;

    extern span<const uint8_t> fuzzer_input;

    vector<function<void()>> &tests();

    struct test {
        test(function<void()> run);
    };

    uint8_t any_byte();

    template<integral T>
    T any(optional<T> maximum = {}) {
        T result = 0;
        for (int i = 0; i < sizeof(T); i++)
            result = (result << 8) + any_byte();
        return min(result, maximum.value_or(numeric_limits<T>::max()));
    }
    
    template<integral T>
    vector<T> many(size_t maximum_size) {
        vector<T> result(any<size_t>(maximum_size));
        for (auto &element : result)
            element = any<T>();
        return result;
    }
}
