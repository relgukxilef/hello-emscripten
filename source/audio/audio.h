#pragma once

#include <opus.h>
#include <AL/al.h>

#include "../utility/opus_resource.h"
#include "../utility/openal_resource.h"

constexpr std::size_t buffer_size = 4 * 1024;

struct audio {
    audio();

    void update();

    unique_opus_encoder encoder;

    unique_openal_capture_device capture_device;
    unique_openal_playback_device playback_device;
    unique_openal_context context;

    ALshort buffer_data[2 * buffer_size];

    unique_openal_buffers<2> buffers;
    unique_openal_sources<1> sources;
};
