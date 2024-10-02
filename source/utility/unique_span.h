#pragma once

#include <memory>
#include <ranges>

template<class In, class Out>
void copy(const In &source, Out destination) {
    auto size = std::min(std::size(source), std::size(destination));
    std::copy(source.begin(), source.begin() + size, destination.begin());
}

template<class T>
struct unique_span {
    unique_span() = default;
    unique_span(unsigned capacity) :
        values(std::make_unique<T[]>(capacity)), capacity(capacity)
    {}
    unique_span(const unique_span &other) : capacity(other.capacity) {
        copy(other, std::views::all(*this));
    }
    void reset(unsigned capacity) {
        this->capacity = capacity;
        values = std::make_unique<T[]>(capacity);
    }
    T &operator[](size_t index) {
        return values[index];
    }
    T *begin() {
        return values.get();
    }
    T *end() {
        return values.get() + capacity;
    }
    const T *begin() const {
        return values.get();
    }
    const T *end() const {
        return values.get() + capacity;
    }
    size_t size() const {
        return capacity;
    }
    unique_span<T> &operator=(const unique_span<T> &other) {
        copy(other, std::views::all(*this));
        return *this;
    }


    std::unique_ptr<T[]> values;
    unsigned capacity = 0;
};
