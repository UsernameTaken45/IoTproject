#pragma once
#include <libcamera/base/log.h>

#include "libcamera/internal/egl.h"
#include "libcamera/framebuffer.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl32.h>


namespace libcamera {

class lens_shading_correction_egl {
public:
	lens_shading_correction_egl();
	void echo();

	void process(FrameBuffer *in, FrameBuffer *out);
	void configure();

private:
	eGL egl;
	GLuint progID;
	GLuint LoadShaders(const char *frag_shader_path, const char *vert_shader_path);
};
}
