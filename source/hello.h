#pragma once

#include <vulkan/vulkan_core.h>
#include <openxr/openxr.h>

#include "audio/audio.h"
#include "state/client.h"
#include "state/input.h"

enum struct mode {
    desktop, vr,
};

struct hello {
    hello(char *arguments[]);

    void update(
        ::input& input, 
        XrInstance instance = XR_NULL_HANDLE, XrSession session = XR_NULL_HANDLE
    );

    ::mode mode = ::mode::desktop;
    std::unique_ptr<::client> client;
    std::unique_ptr<::audio> audio;
};
