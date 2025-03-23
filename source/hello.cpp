#include "hello.h"

#include <cstdio>
#include <cstring>
#include <cstring>

#include <vulkan/vulkan_core.h>

#include "utility/trace.h"
#include "utility/openal_resource.h"
#include "utility/vulkan_resource.h"

hello::hello(
    char *arguments[], VkInstance instance, VkSurfaceKHR surface,
    ::websockets *websockets
) :
    audio(new ::audio())
{
    std::string_view server = "wss://hellovr.at:443/";
    for (auto argument = arguments; *argument != nullptr; argument++) {
        if (strcmp(*argument, "--server") == 0) {
            argument++;
            if (*argument != nullptr) {
                server = {*argument, *argument + strlen(*argument)};
            }
        } else if (strcmp(*argument, "--sine") == 0) {
            audio->play_sine = true;
        }
    }

    client.reset(new ::client(server, websockets));
    visuals.reset(new ::visuals(*client, instance, surface));

    std::printf("Running.\n");
}

void hello::draw(VkInstance instance, VkSurfaceKHR surface) {
    scope_trace trace;
    try {
        visuals->draw(*client, instance, surface);
    } catch (vulkan_error& error) {
        std::fprintf(stderr, "Vulkan error. %s\n", error.what());
        if (error.result == VK_ERROR_DEVICE_LOST) {
            visuals.reset();
            visuals.reset(new ::visuals(*client, instance, surface));
        } else {
            throw;
        }
    }
}

void hello::update(input& input) {
    scope_trace trace;
    client->update(input);

    try {
        audio->update(*client);

    } catch (openal_error& error) {
        std::fprintf(stderr, "OpenAL error. %s\n", error.what());
    }
}
