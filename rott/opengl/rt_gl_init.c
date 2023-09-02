#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "rt_gl_cap.h"
#include "rt_gl_shader.h"

#include "rt_gl_sdl_video_init.h"

#ifndef GL_VERSION_1_1
#error "GL_VERSION_1_1 not defined?"
#endif

#define _quit(s) { \
	printf ("RT_GL: %s", s); \
	return 0; \
}

#define _acc(s) { \
	printf ("RT_GL: %s", s); \
	return 1; \
}

static int _proxy_check_size(int size, GLenum format, GLenum dataformat) {
	GLint ret_size;

	rtglTexImage2D(GL_PROXY_TEXTURE_2D, 0, format, size, size, 0, dataformat, GL_UNSIGNED_BYTE, NULL);

	rtglGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &ret_size);

	if (size != ret_size || rtglGetError() == GL_TRUE)
		return 0;

	return 1;
}

static int _opengl_version(int a, int b) {
	int len;
	long int major = 0, minor = 0;
	char *str;

	char* start = (char*) rtglGetString(GL_VERSION);

	str = start;

	len = strlen (str);

	if(len > 0) {
		major = strtol(start, &str, 10);
		len -= str - start;
	}

	start = str;
	if (len > 0 && *start != '\0' && *(start+1) != '\0')
		minor = strtol(start+1, &str, 10);

	/* needs 1.1 for vertex arrays */
	if (major > a || (major == a && minor >= b))
		return 1;

	return 0;
}


static void _update(int *attr) {
	GLint cmpr;
	rtglGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_COMPRESSED_ARB, &cmpr);
	if (cmpr == GL_TRUE && *attr == 1)
		*attr = 1;
	else
		*attr = 0;
}

static int _select_best_texture_format(void) {
	int i;

#ifdef GL_VERSION_1_3
	/* probe texture compression */
	if (strstr ( (const char*) rtglGetString(GL_EXTENSIONS), "GL_ARB_texture_compression") != NULL || _opengl_version(1,3)) {
		rtglHint( GL_TEXTURE_COMPRESSION_HINT, GL_NICEST);
		rtgl_rgb = GL_COMPRESSED_RGB;
		rtgl_rgba = GL_COMPRESSED_RGBA;
		rtgl_luminance_alpha = GL_COMPRESSED_LUMINANCE_ALPHA;

		rtgl_has_compression_rgb = 1;
		rtgl_has_compression_rgba = 1;
		rtgl_has_compression_luminance_alpha = 1;

		for (i = 8; i <= 512; i *= 2) {
			if (!_proxy_check_size(i, rtgl_rgb, GL_RGB))
				rtgl_has_compression_rgb = 0;
			else
				_update(&rtgl_has_compression_rgb);

			if (!_proxy_check_size(i, rtgl_rgba, GL_RGBA))
				rtgl_has_compression_rgba = 0;
			else
				_update(&rtgl_has_compression_rgba);

			if (!_proxy_check_size(i, rtgl_luminance_alpha, GL_LUMINANCE_ALPHA))
				rtgl_has_compression_luminance_alpha = 0;
			else
				_update(&rtgl_has_compression_luminance_alpha);

		}
		if (rtgl_has_compression_rgb | rtgl_has_compression_rgba)
			_acc("using GL_texture_compression\n");
	}
#else
#warning "GL_VERSION_1_3 not defined."
#endif

	/* try forced 8Bit components / fallback to generic */
	rtgl_rgb = GL_RGB8;
	rtgl_rgba = GL_RGBA8;
	rtgl_luminance_alpha = GL_LUMINANCE_ALPHA;

	for (i = 8; i <= 512; i *= 2) {
		if (!_proxy_check_size(i, rtgl_rgb, GL_RGB))
			rtgl_rgb = GL_RGB;
		if (!_proxy_check_size(i, rtgl_rgba, GL_RGBA))
			rtgl_rgb = GL_RGBA;
		if (!_proxy_check_size(i, rtgl_luminance_alpha, GL_LUMINANCE_ALPHA))
			_quit("GL_LUMINANCE_ALPHA not supported or texture size too large\n");
	}

	/* final test */
	for (i = 8; i <= 512; i *= 2) {
		if (!_proxy_check_size(i, rtgl_rgb, GL_RGB))
			_quit("GL_RGB not supported or texture size too large\n");
		if (!_proxy_check_size(i, rtgl_rgba, GL_RGBA))
			_quit("GL_RGBA not supported or texture size too large\n");
		if (!_proxy_check_size(i, rtgl_luminance_alpha, GL_LUMINANCE_ALPHA))
			_quit("GL_LUMINANCE_ALPHA not supported or texture size too large\n");
	}

	_acc("using RGB(A) / luminance alpha\n");
}

GLfloat _get_max_anisotropy() {
	if (strstr ( (const char*) rtglGetString(GL_EXTENSIONS), "GL_EXT_texture_filter_anisotropic") != NULL) {
		GLfloat tmp;
		rtglGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &tmp);

		if (rtglGetError() != GL_NO_ERROR || tmp <= 1.0f) {
			_quit("no anisotropic filtering supported\n");
		}

		printf("RT_GL: %fx anisotropic filtering supported\n", tmp);
		return tmp;
	}

	_quit("no anisotropic filtering supported\n");
}

GLint _get_multisample (void) {
#ifdef GL_VERSION_1_3
	if (!(strstr ( (const char*) rtglGetString(GL_EXTENSIONS), "GL_ARB_multisample") != NULL || _opengl_version(1,3)))
		_quit("got multisample visual, but GL_ARB_multisample not supported\n");
#ifdef GL_MULTISAMPLE_FILTER_HINT_NV
	if (strstr ( (const char*) rtglGetString(GL_EXTENSIONS), "GL_NV_multisample_filter_hint") != NULL) {
		rtglHint ( GL_MULTISAMPLE_FILTER_HINT_NV, GL_NICEST);
		printf("RT_GL: GL_MULTISAMPLE_FILTER_HINT_NV set\n");
	}
#else
#warning "GL_MULTISAMPLE_FILTER_HINT_NV not defined."
#endif

	GLint tmp;
	rtglGetIntegerv(GL_SAMPLES_ARB, &tmp);
	printf("RT_GL: %d multisample buffers enabled\n", tmp);

	return tmp;
#else
#warning "GL_VERSION_1_3 not defined."
	_quit("Compiled without multisample support\n");
#endif
}

static void _report_glerror() {
	GLenum error;
	do {
		error = rtglGetError();
		switch(error) {
			case GL_INVALID_ENUM:
				printf("GL_ERROR: Invalid enum\n"); break;
			case GL_INVALID_VALUE:
				printf("GL_ERROR: Invalid value\n"); break;
			case GL_INVALID_OPERATION:
				printf("GL_ERROR: Invalid operation\n"); break;
			case GL_OUT_OF_MEMORY:
				printf("GL_ERROR: Out of memory\n"); break;
			case GL_TABLE_TOO_LARGE:
				printf("GL_ERROR: Table too large\n"); break;
			case GL_STACK_OVERFLOW:
				printf("GL_ERROR: Stack overflow\n"); break;
			case GL_STACK_UNDERFLOW:
				printf("GL_ERROR: Stack underflow\n"); break;
			default:
				printf("GL_ERROR: Unknown GL_ERROR returned\n");
			case GL_NO_ERROR: break;
		}
	} while (error != GL_NO_ERROR);
}

int rtgl_SupportedCard(void) {
	if ( !_get_req_gl_functions() )
		_quit( "_get_req_gl_functions() failed\n");

	if ( !_opengl_version(1,1) )
		_quit( "OpenGL >=1.1 required\n");

	if ( !_select_best_texture_format() )
		return 0;

	rtgl_max_anisotropy = _get_max_anisotropy();
	if (rtgl_has_multisample) rtgl_has_multisample = _get_multisample();
	rtgl_use_multisample = rtgl_has_multisample;

	_report_glerror();

	if ( (strstr ( (const char*) rtglGetString(GL_EXTENSIONS), "GL_ARB_texture_rectangle") != NULL) ||
		(strstr ( (const char*) rtglGetString(GL_EXTENSIONS), "GL_EXT_texture_rectangle") != NULL) ||
		(strstr ( (const char*) rtglGetString(GL_EXTENSIONS), "GL_NV_texture_rectangle") != NULL) ) {
			GLint width;
			GLint height;

			rtglEnable(GL_TEXTURE_RECTANGLE_NV);
			rtglTexImage2D(GL_PROXY_TEXTURE_RECTANGLE_NV, 0, rtgl_rgb, rtgl_screen_width, rtgl_screen_height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
			rtglGetTexLevelParameteriv(GL_PROXY_TEXTURE_RECTANGLE_NV, 0, GL_TEXTURE_WIDTH, &width);
			rtglGetTexLevelParameteriv(GL_PROXY_TEXTURE_RECTANGLE_NV, 0, GL_TEXTURE_HEIGHT, &height);
			if (width == (GLint) rtgl_screen_width && height == (GLint) rtgl_screen_height) {
				rtgl_has_texture_rectangle = 1;
				puts("RT_GL: GL_ARB_texture_rectangle supported");
			}
			else {
				rtgl_has_texture_rectangle = 0;
				puts("RT_GL: GL_ARB_texture_rectangle supported but glTexImage2D request failed");
			}

			rtglDisable(GL_TEXTURE_RECTANGLE_NV);
			rtglEnable(GL_TEXTURE_2D);
	}
	else {
		rtgl_has_texture_rectangle = 0;
		puts("RT_GL: GL_ARB_texture_rectangle not supported");
	}

	_report_glerror();

	rtgl_has_shader = 0;
	if ( _opengl_version(2,0) && _get_shader_gl_functions()) {
		if(rtglUploadTextureShader()) {
			rtgl_has_shader = 1;
			puts("RT_GL: Texture shader enabled\n");
		} else {
			printf("Texture Shader upload failed\n");
		}
	}
	if (!rtgl_has_shader)
		printf("RT_GL: texture shaders not supported\n");

	_report_glerror();

	return 1;
}
