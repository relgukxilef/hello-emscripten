#pragma once

#include <ranges>

#include <opus.h>
#include <AL/al.h>

#include "../state/client.h"
#include "../utility/opus_resource.h"
#include "../utility/openal_resource.h"

constexpr std::size_t buffer_count = 8;
constexpr std::size_t buffer_size = 960;

struct audio {
    audio();

    void update(::client &client);

    unique_opus_encoder encoder;
    unique_opus_decoder decoder;

    unique_openal_capture_device capture_device;
    unique_openal_playback_device playback_device;
    unique_openal_context context;

    ALshort buffer_data[buffer_count * buffer_size];

    unique_openal_buffers<buffer_count> buffers;
    unique_openal_sources<1> sources;
};
