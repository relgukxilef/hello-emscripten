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

struct xr_instance_deleter {
    typedef XrInstance pointer;
    void operator()(XrInstance instance) {
        if (instance != XR_NULL_HANDLE) {
            xrDestroyInstance(instance);
        }
    }
};

struct xr_session_deleter {
    typedef XrSession pointer;
    void operator()(XrSession session) {
        if (session != XR_NULL_HANDLE) {
            xrDestroySession(session);
        }
    }
};

using unique_xr_instance = std::unique_ptr<XrInstance, xr_instance_deleter>;
using unique_xr_session = std::unique_ptr<XrSession, xr_session_deleter>;
