#pragma once

#include <opus.h>
#include <AL/al.h>

#include "../utility/opus_resource.h"
#include "../utility/openal_resource.h"

struct audio {
    audio();

    void update();

    unique_opus_encoder encoder;

    unique_openal_capture_device capture_device;
    unique_openal_playback_device playback_device;
    unique_openal_context context;

    ALshort buffer_data[4 * 8 * 1024];

    unique_openal_buffers<4> buffers;
    unique_openal_sources<1> sources;
};
