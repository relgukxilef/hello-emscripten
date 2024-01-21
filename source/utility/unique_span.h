#pragma once

#include <memory>

template<class T>
struct unique_span {
    unique_span() = default;
    unique_span(unsigned capacity) :
        values(std::make_unique<T[]>(capacity)), capacity(capacity)
    {}
    T &operator[](size_t index) {
        return values[index];
    }
    T *begin() {
        return values.get();
    }
    T *end() {
        return values.get() + capacity;
    }
    size_t size() const {
        return capacity;
    }
    std::unique_ptr<T[]> values;
    unsigned capacity = 0;
};
