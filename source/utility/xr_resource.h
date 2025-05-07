#pragma once

#include <memory>
#include <stdexcept>

#include <openxr/openxr.h>

struct xr_error : public std::exception {
    xr_error(XrResult result) noexcept;

    const char *what() const noexcept override;

    XrResult result;
};

inline xr_error::xr_error(XrResult result) noexcept : result(result) {}

inline void check(XrResult result) {
    if (result != XR_SUCCESS) {
        throw xr_error(result);
    }
}

template<typename T, auto Deleter>
struct xr_deleter {
    typedef T pointer;
    void operator()(pointer object) {
        if (object != XR_NULL_HANDLE) {
            Deleter(object);
        }
    }
};

template<typename T, auto Deleter>
using unique_xr_resource =
    std::unique_ptr<T, xr_deleter<T, Deleter>>;

using unique_xr_instance = unique_xr_resource<XrInstance, xrDestroyInstance>;
using unique_xr_session = unique_xr_resource<XrSession, xrDestroySession>;
using unique_xr_swapchain = unique_xr_resource<XrSwapchain, xrDestroySwapchain>;
