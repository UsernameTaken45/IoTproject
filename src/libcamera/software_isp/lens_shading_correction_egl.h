#pragma once
#include "libcamera/internal/egl.h"
#include <libcamera/base/log.h>

namespace libcamera {

class lens_shading_correction_egl {
public:
	lens_shading_correction_egl();
	void echo();
};
}
