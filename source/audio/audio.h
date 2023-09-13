#pragma once

#include <opus.h>

#include "../utility/opus_resource.h"

struct audio {
    audio();

    unique_opus_encoder encoder;
};
