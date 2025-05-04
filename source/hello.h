#pragma once

#include <vulkan/vulkan_core.h>

#include "audio/audio.h"
#include "state/client.h"
#include "state/input.h"

struct hello {
    hello(char *arguments[]);

    void update(::input& input);

    std::unique_ptr<::client> client;
    std::unique_ptr<::audio> audio;
};
