#include "hello.h"

#include <cstdio>
#include <cstring>
#include <cstring>

#include <vulkan/vulkan_core.h>

#include "utility/trace.h"
#include "utility/openal_resource.h"
#include "utility/vulkan_resource.h"
#include "utility/xr_resource.h"

hello::hello(char *arguments[]) : 
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

    client.reset(new ::client(server));

    std::printf("Running.\n");
}

void hello::update(input& input, XrInstance instance, XrSession session) {
    scope_trace trace;
    
    if (session) {
        bool instance_lost = false;
        bool session_lost = false;
        while (true) {
            XrEventDataBuffer event {.type = XR_TYPE_EVENT_DATA_BUFFER,};
            XrResult result = xrPollEvent(instance, &event);
            if (result == XR_EVENT_UNAVAILABLE)
                break;
            check(result);
            auto type = event.type;
            std::printf("Event %d\n", type);
            if (type == XR_TYPE_EVENT_DATA_EVENTS_LOST) {
                std::printf("XR_TYPE_EVENT_DATA_EVENTS_LOST\n");
            } else if (type == XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING) {
                instance_lost = true;
            } else if (type == XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED) {
            
            } else if (
                type == XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING
            ) {
            
            } else if (type == XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED) {
                auto &change = 
                    reinterpret_cast<XrEventDataSessionStateChanged &>(
                        event
                    );
                auto state = change.state;
                std::printf("State %d\n", state);
                if (state == XR_SESSION_STATE_READY) {
                    XrSessionBeginInfo begin_info {
                        .type = XR_TYPE_SESSION_BEGIN_INFO,
                        .next = nullptr,
                        .primaryViewConfigurationType = 
                            XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
                    };
                    check(xrBeginSession(session, &begin_info));
                } else if (state == XR_SESSION_STATE_STOPPING) {
                    check(xrEndSession(session));
                } else if (state == XR_SESSION_STATE_EXITING) {
                    session_lost = true;
                } else if (state == XR_SESSION_STATE_LOSS_PENDING) {
                    session_lost = true;
                }
            }
        }
        if (instance_lost) {
            throw xr_error(XR_ERROR_INSTANCE_LOST);
        }
        if (session_lost) {
            throw xr_error(XR_SESSION_LOSS_PENDING);
        }
    }

    client->update(input);

    try {
        audio->update(*client);

    } catch (openal_error& error) {
        std::fprintf(stderr, "OpenAL error. %s\n", error.what());
    }
}
