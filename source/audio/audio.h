#pragma once

#include <opus.h>
#include <AL/al.h>

#include "../utility/opus_resource.h"
#include "../utility/openal_resource.h"

struct audio {
    audio();

    unique_opus_encoder encoder;
    unique_openal_capture_device capture_device;
    unique_openal_playback_device playback_device;
};
