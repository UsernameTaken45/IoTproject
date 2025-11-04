#include "lens_shading_correction_egl.h"

#include <fstream>
#include <iostream>

#include <libcamera/base/log.h>

namespace libcamera {
LOG_DEFINE_CATEGORY(LensShading);

lens_shading_correction_egl::lens_shading_correction_egl()
{

}

void lens_shading_correction_egl::echo()
{
	LOG(LensShading, Error) << "TESTBERICHT";
}

void lens_shading_correction_egl::configure()
{
	progID = LoadShaders("fragshader.gl", "vertshader.gl"); // Dit is onze oplossing, niet die van libcamera
	egl.useProgram(progID);
}

void lens_shading_correction_egl::process(FrameBuffer *in, FrameBuffer *out)
{
	egl.makeCurrent();
	auto imageIn = new eGLImage(1920, 1080, 32, GL_TEXTURE4, 4);
	egl.createInputDMABufTexture2D(imageIn, GL_LUMINANCE, 1920, 1080, 4 , in->planes()[0].fd.get());

	eGLImage t = eGLImage(1920, 1080, 32, GL_TEXTURE4, 4);
	egl.createOutputDMABufTexture2D(&t, out->planes()[0].fd.get());


	glViewport(0, 0, 1920, 1080); // hardcoded
	glClear(GL_COLOR_BUFFER_BIT);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	GLenum err = glGetError();
	if (err != GL_NO_ERROR) {
		LOG(eGL, Error) << "Drawing scene fail " << err;
	} else {
		egl.syncOutput();
	}
}

GLuint lens_shading_correction_egl::LoadShaders(const char* frag_shader_path, const char* vert_shader_path){

	GLuint fragshaderID = glCreateShader(GL_FRAGMENT_SHADER);
	GLuint vertshaderID = glCreateShader(GL_VERTEX_SHADER);


	printf("loading\n");
	//loading in fragment shader from file
	std::string FragmentShaderCode;
	std::ifstream fragmentShaderStream(frag_shader_path, std::ios::in);
	if(fragmentShaderStream.is_open()){
		std::stringstream sstr;
		sstr << fragmentShaderStream.rdbuf();
		FragmentShaderCode = sstr.str();
		fragmentShaderStream.close();
	}

	std::string vertShaderCode;
	std::ifstream vertShaderStream(vert_shader_path, std::ios::in);
	if(vertShaderStream.is_open()){
		std::stringstream sstr;
		sstr << vertShaderStream.rdbuf();
		vertShaderCode = sstr.str();
		vertShaderStream.close();
	}

	GLint result = GL_FALSE;
	int Infologlength;

	//compile frag shader
	printf("compiling frag shader: %s\n", frag_shader_path);
	const char* fragsourcepointer = FragmentShaderCode.c_str();
	glShaderSource(fragshaderID, 1, &fragsourcepointer, NULL);
	glCompileShader(fragshaderID);

	//checking compiled shader
	printf("checking compiled shader\n");
	glGetShaderiv(fragshaderID, GL_COMPILE_STATUS, &result);
	glGetShaderiv(fragshaderID, GL_INFO_LOG_LENGTH, &Infologlength);
	if(Infologlength > 0){
		std::vector<char> FragmentShaderErrorMessage(Infologlength + 1);
		glGetShaderInfoLog(fragshaderID, Infologlength, NULL, &FragmentShaderErrorMessage[0]);
		printf("%s\n", &FragmentShaderErrorMessage[0]);
	}


	printf("compiling vert shader: %s\n", vert_shader_path);
	const char* vertsourcepointer = vertShaderCode.c_str();
	glShaderSource(vertshaderID, 1, &vertsourcepointer, NULL);
	glCompileShader(vertshaderID);

	//checking compiled shader
	printf("checking compiled vert shader\n");
	glGetShaderiv(vertshaderID, GL_COMPILE_STATUS, &result);
	glGetShaderiv(vertshaderID, GL_INFO_LOG_LENGTH, &Infologlength);
	if(Infologlength > 0){
		std::vector<char> vertShaderErrorMessage(Infologlength + 1);
		glGetShaderInfoLog(vertshaderID, Infologlength, NULL, &vertShaderErrorMessage[0]);
		printf("%s\n", &vertShaderErrorMessage[0]);
	}

	//linking shader
	printf("attatching\n");
	GLuint programID = glCreateProgram();
	glAttachShader(programID, fragshaderID);
	glAttachShader(programID, vertshaderID);
	glLinkProgram(programID);

	//check program
	printf("checking program\n");
	glGetProgramiv(programID, GL_LINK_STATUS, &result);
	glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &Infologlength);
	if(Infologlength > 0){
		std::vector<char> Programerrormsg(Infologlength+1);
		glGetProgramInfoLog(programID, Infologlength, NULL, &Programerrormsg[0]);
		printf("%s\n", &Programerrormsg[0]);

	}

	glDetachShader(programID, fragshaderID);
	glDetachShader(programID, vertshaderID);

	glDeleteShader(fragshaderID);
	glDeleteShader(vertshaderID);
	printf("shader done\n");
	return programID;
//	return 0;
}


}