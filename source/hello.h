#pragma once

#include <vulkan/vulkan_core.h>

#include "visuals/visuals.h"
#include "audio/audio.h"
#include "state/client.h"
#include "state/input.h"
#include "network/websocket-boost.h"

struct hello {
    hello(char *arguments[], VkInstance instance, VkSurfaceKHR surface);

    void draw(VkInstance instance, VkSurfaceKHR surface);
    void update(::input& input);

    boost_websockets websockets;
    std::unique_ptr<::client> client;
    std::unique_ptr<::visuals> visuals;
    std::unique_ptr<::audio> audio;
};
