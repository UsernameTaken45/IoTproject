/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2024, Linaro Ltd.
 *
 * Authors:
 * Bryan O'Donoghue <bryan.odonoghue@linaro.org>
 *
 * egl.cpp - Helper class for managing eGL interactions.
 */

#include "libcamera/internal/egl.h"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <libdrm/drm_fourcc.h>
#include <linux/dma-buf.h>
#include <linux/dma-heap.h>

namespace libcamera {

LOG_DEFINE_CATEGORY(eGL)

eGL::eGL()
{
}

eGL::~eGL()
{
}

int eGL::syncOutput(void)
{
	glFlush();
	eglClientWaitSyncKHR(display_, sync_, 0, EGL_FOREVER_KHR);

	return 0;
}

// Create linear image attached to previous BO object
int eGL::createDMABufTexture2D(eGLImage *eglImage, int fd, bool output)
{
	int ret = 0;

	EGLint image_attrs[] = {
		EGL_WIDTH, (EGLint)eglImage->width_,
		EGL_HEIGHT, (EGLint)eglImage->height_,
		EGL_LINUX_DRM_FOURCC_EXT, DRM_FORMAT_ARGB8888,
		EGL_DMA_BUF_PLANE0_FD_EXT, fd,
		EGL_DMA_BUF_PLANE0_OFFSET_EXT, 0,
		EGL_DMA_BUF_PLANE0_PITCH_EXT, (EGLint)eglImage->stride_,
		EGL_NONE, EGL_NONE,	/* modifier lo */
		EGL_NONE, EGL_NONE,	/* modifier hi */
		EGL_NONE,
	};

	eglImage->image_ = eglCreateImageKHR(display_, EGL_NO_CONTEXT,
					     EGL_LINUX_DMA_BUF_EXT,
					     NULL, image_attrs);

	if (eglImage->image_ == EGL_NO_IMAGE_KHR) {
		LOG(eGL, Error) << "eglCreateImageKHR fail";
		ret = -ENODEV;
		goto done;
	}

	// Bind texture unit and texture
	glActiveTexture(eglImage->texture_unit_);
	glBindTexture(GL_TEXTURE_2D, eglImage->texture_);

	// Generate texture with filter semantics
	glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, eglImage->image_);

	// Nearest filtering
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Wrap to edge to avoid edge artifacts
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	if (output) {
		// Generate a framebuffer from our texture direct to dma-buf handle buffer
		glBindFramebuffer(GL_FRAMEBUFFER, eglImage->fbo_);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, eglImage->texture_, 0);

		GLenum err = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (err!= GL_FRAMEBUFFER_COMPLETE) {
			LOG(eGL, Error) << "glFrameBufferTexture2D error " << err;
			ret = -ENODEV;
			goto done;
		}
	}
done:
	return ret;
}

int eGL::createInputDMABufTexture2D(eGLImage *eglImage, int fd)
{
	return createDMABufTexture2D(eglImage, fd, false);
}
int eGL::createOutputDMABufTexture2D(eGLImage *eglImage, int fd)
{
	return createDMABufTexture2D(eglImage, fd, true);
}

void eGL::destroyDMABufTexture(eGLImage *eglImage)
{
	eglDestroyImage(display_, eglImage->image_);
}

// Generate a 2D texture from an input buffer directly
void eGL::createTexture2D(eGLImage *eglImage, GLint format, uint32_t width, uint32_t height, void *data)
{
	glActiveTexture(eglImage->texture_unit_);
	glBindTexture(GL_TEXTURE_2D, eglImage->texture_);

	// Generate texture, bind, associate image to texture, configure, unbind
	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

	// Nearest filtering
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Wrap to edge to avoid edge artifacts
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

int eGL::initEGLContext(GBM *gbmContext)
{
	EGLint configAttribs[] = {
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_ALPHA_SIZE, 8,
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_NONE
	};

	EGLint contextAttribs[] = {
		EGL_CONTEXT_MAJOR_VERSION, 2,
		EGL_NONE
	};

	EGLint numConfigs;
	EGLConfig config;
	EGLint major;
	EGLint minor;

	if (!eglBindAPI(EGL_OPENGL_ES_API)) {
		LOG(eGL, Error) << "API bind fail";
		goto fail;
	}

	display_ = eglGetDisplay(gbmContext->getDevice());
	if (display_ == EGL_NO_DISPLAY) {
		LOG(eGL, Error) << "Unable to get EGL display";
		goto fail;
	}

	if (eglInitialize(display_, &major, &minor) != EGL_TRUE) {
		LOG(eGL, Error) << "eglInitialize fail";
		goto fail;
	}

	LOG(eGL, Info) << "EGL: version " << major << "." << minor;
	LOG(eGL, Info) << "EGL: EGL_VERSION: " << eglQueryString(display_, EGL_VERSION);
	LOG(eGL, Info) << "EGL: EGL_VENDOR: " << eglQueryString(display_, EGL_VENDOR);
	LOG(eGL, Info) << "EGL: EGL_CLIENT_APIS: " << eglQueryString(display_, EGL_CLIENT_APIS);
	LOG(eGL, Info) << "EGL: EGL_EXTENSIONS: " << eglQueryString(display_, EGL_EXTENSIONS);

	eglCreateImageKHR = (PFNEGLCREATEIMAGEKHRPROC)eglGetProcAddress("eglCreateImageKHR");
	if (!eglCreateImageKHR) {
		LOG(eGL, Error) << "eglCreateImageKHR not found";
		goto fail;
	}

	eglDestroyImageKHR = (PFNEGLDESTROYIMAGEKHRPROC)eglGetProcAddress("eglDestroyImageKHR");
	if (!eglDestroyImageKHR) {
		LOG(eGL, Error) << "eglDestroyImageKHR not found";
		goto fail;
	}

	glEGLImageTargetTexture2DOES = (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)eglGetProcAddress("glEGLImageTargetTexture2DOES");
	if (!glEGLImageTargetTexture2DOES) {
		LOG(eGL, Error) << "glEGLImageTargetTexture2DOES not found";
		goto fail;
	}

	eglCreateSyncKHR = (PFNEGLCREATESYNCKHRPROC)eglGetProcAddress("eglCreateSyncKHR");
	if (!eglCreateSyncKHR) {
		LOG(eGL, Error) << "eglCreateSyncKHR not found";
		goto fail;
	}

	eglDestroySyncKHR = (PFNEGLDESTROYSYNCKHRPROC)eglGetProcAddress("eglDestroySyncKHR");
	if (!eglDestroySyncKHR) {
		LOG(eGL, Error) << "eglDestroySyncKHR not found";
		goto fail;
	}

	eglClientWaitSyncKHR = (PFNEGLCLIENTWAITSYNCKHRPROC)eglGetProcAddress("eglClientWaitSyncKHR");
	if (!eglClientWaitSyncKHR) {
		LOG(eGL, Error) << "eglClientWaitSyncKHR not found";
		goto fail;
	}

	if (eglChooseConfig(display_, configAttribs, &config, 1, &numConfigs) != EGL_TRUE) {
		LOG(eGL, Error) << "eglChooseConfig fail";
		goto fail;
	}

	context_ = eglCreateContext(display_, config, EGL_NO_CONTEXT, contextAttribs);
	if (context_ == EGL_NO_CONTEXT) {
		LOG(eGL, Error) << "eglContext returned EGL_NO_CONTEXT";
		goto fail;
	}

	makeCurrent();

	sync_ = eglCreateSyncKHR(display_, EGL_SYNC_FENCE_KHR, NULL);
	if (sync_ == EGL_NO_SYNC_KHR) {
		LOG(eGL, Error) << "eglCreateSyncKHR fail";
		goto fail;
	}

	return 0;
fail:

	return -ENODEV;
}

void eGL::cleanUp(void)
{
	if (sync_) {
		makeCurrent();
		eglDestroySyncKHR(display_, sync_);
	}

}
void eGL::makeCurrent(void)
{
	if (eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, context_) != EGL_TRUE) {
		LOG(eGL, Error) << "eglMakeCurrent fail";
	}
}

void eGL::swapBuffers(void)
{
	if (eglSwapBuffers(display_, surface_) != EGL_TRUE) {
		LOG(eGL, Error) << "eglSwapBuffers fail";
	}
}

void eGL::useProgram(GLuint programId)
{
	glUseProgram(programId);
}

void eGL::pushEnv(std::vector<std::string> &shaderEnv, const char *str)
{
	std::string addStr = str;

	addStr.push_back('\n');
	shaderEnv.push_back(addStr);
}

int eGL::compileVertexShader(GLuint &shaderId, unsigned char *shaderData,
			     unsigned int shaderDataLen,
			     std::vector<std::string> shaderEnv)
{
	return compileShader(GL_VERTEX_SHADER, shaderId, shaderData, shaderDataLen, shaderEnv);
}

int eGL::compileFragmentShader(GLuint &shaderId, unsigned char *shaderData,
			       unsigned int shaderDataLen,
			       std::vector<std::string> shaderEnv)
{
	return compileShader(GL_FRAGMENT_SHADER, shaderId, shaderData, shaderDataLen, shaderEnv);
}

int eGL::compileShader(int shaderType, GLuint &shaderId, unsigned char *shaderData,
		       unsigned int shaderDataLen,
		       std::vector<std::string> shaderEnv)
{
	GLchar **shaderSourceData;
	GLint *shaderDataLengths;
	GLint success;
	GLsizei count;
	size_t i;

	count = 1 + shaderEnv.size();
	shaderSourceData = new GLchar *[count];
	shaderDataLengths = new GLint[count];

	// Prefix defines before main body of shader
	for (i = 0; i < shaderEnv.size(); i++) {
		shaderSourceData[i] = (GLchar *)shaderEnv[i].c_str();
		shaderDataLengths[i] = shaderEnv[i].length();
	}

	// Now the main body of the shader program
	shaderSourceData[i] = (GLchar *)shaderData;
	shaderDataLengths[i] = shaderDataLen;

	// And create the shader
	shaderId = glCreateShader(shaderType);
	glShaderSource(shaderId, count, shaderSourceData, shaderDataLengths);
	glCompileShader(shaderId);

	// Check status
	glGetShaderiv(shaderId, GL_COMPILE_STATUS, &success);
	if (success == GL_FALSE) {
		GLint sizeLog = 0;
		GLchar *infoLog;

		glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &sizeLog);
		infoLog = new GLchar[sizeLog];

		glGetShaderInfoLog(shaderId, sizeLog, &sizeLog, infoLog);
		LOG(eGL, Error) << infoLog;

		delete[] infoLog;
	}

	delete[] shaderSourceData;
	delete[] shaderDataLengths;

	return !(success == GL_TRUE);
}

void eGL::dumpShaderSource(GLuint shaderId)
{
	GLint shaderLength = 0;
	GLchar *shaderSource;

	glGetShaderiv(shaderId, GL_SHADER_SOURCE_LENGTH, &shaderLength);

	LOG(eGL, Debug) << "Shader length is " << shaderLength;

	if (shaderLength > 0) {
		shaderSource = new GLchar[shaderLength];
		if (!shaderSource)
			return;

		glGetShaderSource(shaderId, shaderLength, &shaderLength, shaderSource);
		if (shaderLength) {
			LOG(eGL, Debug) << "Shader source = " << shaderSource;
		}
		delete[] shaderSource;
	}
}

int eGL::linkProgram(GLuint &programId, GLuint vertexshaderId, GLuint fragmentshaderId)
{
	GLint success;
	GLenum err;

	programId = glCreateProgram();
	if (!programId)
		goto fail;

	glAttachShader(programId, vertexshaderId);
	if ((err = glGetError()) != GL_NO_ERROR) {
		LOG(eGL, Error) << "Attach compute vertex shader fail";
		goto fail;
	}

	glAttachShader(programId, fragmentshaderId);
	if ((err = glGetError()) != GL_NO_ERROR) {
		LOG(eGL, Error) << "Attach compute vertex shader fail";
		goto fail;
	}

	glLinkProgram(programId);
	if ((err = glGetError()) != GL_NO_ERROR) {
		LOG(eGL, Error) << "Link program fail";
		goto fail;
	}

	glDetachShader(programId, fragmentshaderId);
	glDetachShader(programId, vertexshaderId);

	// Check status
	glGetProgramiv(programId, GL_LINK_STATUS, &success);
	if (success == GL_FALSE) {
		GLint sizeLog = 0;
		GLchar *infoLog;

		glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &sizeLog);
		infoLog = new GLchar[sizeLog];

		glGetProgramInfoLog(programId, sizeLog, &sizeLog, infoLog);
		LOG(eGL, Error) << infoLog;

		delete[] infoLog;
		goto fail;
	}

	return 0;
fail:
	return -ENODEV;
}
} // namespace libcamera
