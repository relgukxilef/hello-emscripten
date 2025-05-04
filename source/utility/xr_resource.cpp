#include "xr_resource.h"

const char *xr_error::what() const noexcept {
    switch (result) {
    case XR_ERROR_VALIDATION_FAILURE:
        return "The function usage was invalid in some way.";
    case XR_ERROR_RUNTIME_FAILURE:
        return 
            "The runtime failed to handle the function in an unexpected way "
            "that is not covered by another error result.";
    case XR_ERROR_OUT_OF_MEMORY:
        return "A memory allocation has failed.";
    case XR_ERROR_API_VERSION_UNSUPPORTED:
        return "The runtime does not support the requested API version.";
    case XR_ERROR_INITIALIZATION_FAILED:
        return "Initialization of object could not be completed.";
    case XR_ERROR_FUNCTION_UNSUPPORTED:
        return 
            "The requested function was not found or is otherwise unsupported.";
    default:
        return "Other OpenXR error.";
    }
}
