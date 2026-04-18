#include "reality.h"

#include "../utility/out_ptr.h"

reality::reality(XrInstance instance, XrSession xr_session) {
    this->xr_session = xr_session;
}
