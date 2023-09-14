#pragma once

template<typename T, auto Deleter, auto Null = T{}>
struct unique_resource {
    typedef T pointer;

    unique_resource() : value(Null) {}
    unique_resource(T value) : value(value) {}
    unique_resource(const unique_resource&) = delete;
    unique_resource(unique_resource&& o) : value(o.value) {
        o.value = Null;
    }
    ~unique_resource() {
        if (value != Null)
            Deleter(&value);
    }
    unique_resource& operator= (const unique_resource&) = delete;
    unique_resource& operator= (unique_resource&& o) {
        if (value != Null)
            Deleter(&value);
        value = o.value;
        o.value = Null;
        return *this;
    }
    auto operator*() {
        return *value;
    }
    T* operator->() {
        return &value;
    }
    operator bool() const {
        return value != Null;
    }

    const T& get() const {
        return value;
    };

    T& get() {
        return value;
    };

private:
    T value;
};
