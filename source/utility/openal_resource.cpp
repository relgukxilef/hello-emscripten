#include "openal_resource.h"
#include <AL/al.h>

const char* openal_error::what() const noexcept {
    switch (error) {
    case AL_INVALID_NAME:
        return "Invalid name parameter.";
    case AL_INVALID_ENUM:
        return "Invalid parameter.";
    case AL_INVALID_VALUE:
        return "Invalid enum parameter value.";
    case AL_INVALID_OPERATION:
        return "Illegal call";
    case AL_OUT_OF_MEMORY:
        return "Unable to allocate memory. (Or the device cannot capture)";
    default:
        return "Other openal error.";
    }
}

const char* openalc_error::what() const noexcept {
    switch (error) {
    case ALC_INVALID_DEVICE:
        return 
            "The device handle or specifier names an inaccessible "
            "driver/server.";
    case ALC_INVALID_CONTEXT:
        return "The Context argument does not name a valid context.";
    case ALC_INVALID_ENUM:
        return "A token used is not valid, or not applicable.";
    case ALC_INVALID_VALUE:
        return "A value (e.g. Attribute) is not valid, or not applicable.";
    case ALC_OUT_OF_MEMORY:
        return "Unable to allocate memory.";
    default:
        return "Other openalc error.";
    }
}
