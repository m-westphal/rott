#include <SDL2/SDL_video.h>
#include "rt_gl_sdl_video_init.h"
#include "rt_gl_cap.h"

void (APIENTRY * rtglAlphaFunc) (GLenum, GLclampf);
void (APIENTRY * rtglBegin) (GLenum);
void (APIENTRY * rtglBindTexture) (GLenum, GLuint);
void (APIENTRY * rtglBlendFunc) (GLenum, GLenum);
void (APIENTRY * rtglClear) (GLbitfield);
void (APIENTRY * rtglClearDepth) (GLclampd);
void (APIENTRY * rtglClearColor) (GLclampf, GLclampf, GLclampf, GLclampf);
void (APIENTRY * rtglColor3f) (GLfloat, GLfloat, GLfloat);
void (APIENTRY * rtglColor4f) (GLfloat, GLfloat, GLfloat, GLfloat);
void (APIENTRY * rtglCopyTexImage2D) (GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLsizei, GLint);
void (APIENTRY * rtglDepthFunc) (GLenum);
void (APIENTRY * rtglDepthMask) (GLboolean);
void (APIENTRY * rtglDeleteTextures) (GLsizei, const GLuint *);
void (APIENTRY * rtglDisable) (GLenum);
void (APIENTRY * rtglDrawArrays) (GLenum, GLint, GLsizei);
void (APIENTRY * rtglEnable) (GLenum);
void (APIENTRY * rtglEnd) (void);
void (APIENTRY * rtglFlush) (void);
void (APIENTRY * rtglFogf) (GLenum, GLfloat);
void (APIENTRY * rtglFogfv) (GLenum, const GLfloat *);
void (APIENTRY * rtglFogi) (GLenum, GLint);
GLenum (APIENTRY * rtglGetError) (void);
void (APIENTRY * rtglGenTextures) (GLsizei, GLuint *);
void (APIENTRY * rtglGetFloatv) (GLenum, GLfloat*);
void (APIENTRY * rtglGetIntegerv) (GLenum, GLint*);
void (APIENTRY * rtglGetTexLevelParameteriv) (GLenum, GLint, GLenum, GLint *);
void (APIENTRY * rtglHint) (GLenum, GLenum);
void (APIENTRY * rtglInterleavedArrays) (GLenum, GLsizei, const GLvoid *);
void (APIENTRY * rtglLightf) (GLenum, GLenum, GLfloat);
void (APIENTRY * rtglLightfv) (GLenum, GLenum, const GLfloat *);
void (APIENTRY * rtglLightModeli) (GLenum, GLint);
void (APIENTRY * rtglLightModelfv) (GLenum, const GLfloat *);
void (APIENTRY * rtglLoadIdentity) (void);
void (APIENTRY * rtglMaterialfv) (GLenum, GLenum, const GLfloat *);
void (APIENTRY * rtglMatrixMode) (GLenum);
void (APIENTRY * rtglNormal3f) (GLfloat, GLfloat, GLfloat);
const GLubyte* (APIENTRY * rtglGetString) (GLenum);
void (APIENTRY * rtglPixelStorei) (GLenum, GLint);
void (APIENTRY * rtglPushAttrib) (GLbitfield);
void (APIENTRY * rtglPopAttrib) (void);
void (APIENTRY * rtglPushMatrix) (void);
void (APIENTRY * rtglPopMatrix) (void);
void (APIENTRY * rtglRotatef) (GLfloat, GLfloat, GLfloat, GLfloat);
void (APIENTRY * rtglScalef) (GLfloat, GLfloat, GLfloat);
void (APIENTRY * rtglShadeModel) (GLenum);
void (APIENTRY * rtglTranslatef) (GLfloat, GLfloat, GLfloat);
void (APIENTRY * rtglTexCoord2f) (GLfloat, GLfloat);
void (APIENTRY * rtglTexEnvi) (GLenum, GLenum, GLint);
void (APIENTRY * rtglTexImage2D) (GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid *);
void (APIENTRY * rtglTexParameteri) (GLenum, GLenum, GLint);
void (APIENTRY * rtglTexParameterf) (GLenum, GLenum, GLfloat);
void (APIENTRY * rtglVertex3f) (GLfloat, GLfloat, GLfloat);
void (APIENTRY * rtglViewport) (GLint, GLint, GLsizei, GLsizei);
// DEBUG
void (APIENTRY * rtglPolygonMode) (GLenum, GLenum);

/* opengl 2.0 shader functions */
void (APIENTRY * rtglShaderSource) (GLuint, GLsizei, const GLchar**, const GLint*);
GLuint (APIENTRY * rtglCreateShader) (GLenum);
void (APIENTRY * rtglCompileShader) (GLuint);
void (APIENTRY * rtglAttachShader) (GLuint, GLuint);
void (APIENTRY * rtglGetShaderiv) (GLuint, GLenum, GLint *);
void (APIENTRY * rtglGetShaderInfoLog) (GLuint, GLsizei, GLsizei*, GLchar *);
void (APIENTRY * rtglDeleteShader) (GLuint);
void (APIENTRY * rtglLinkProgram) (GLuint);
GLuint (APIENTRY * rtglCreateProgram) (GLvoid);
void (APIENTRY * rtglUseProgram) (GLuint);
void (APIENTRY * rtglGetProgramiv) (GLuint, GLenum, GLint *);
void (APIENTRY * rtglValidateProgram) (GLuint);
GLint (APIENTRY * rtglGetUniformLocation) (GLuint, const GLchar *);
void (APIENTRY * rtglUniform1i) (GLint, GLint);

void (APIENTRY * rtglActiveTexture) (GLenum);


#define GPA(v, f) { \
	v = SDL_GL_GetProcAddress (f); \
	if (v == NULL) {\
		printf("RT_GL: Couldn't get %s address\n", f); \
		return 0; \
	} \
}


int _get_req_gl_functions (void) {
	GPA(rtglAlphaFunc, "glAlphaFunc");
	GPA(rtglBegin, "glBegin");
	GPA(rtglBindTexture, "glBindTexture");
	GPA(rtglBlendFunc, "glBlendFunc");
	GPA(rtglClear, "glClear");
	GPA(rtglClearDepth, "glClearDepth");
	GPA(rtglClearColor, "glClearColor");
	GPA(rtglColor3f, "glColor3f");
	GPA(rtglColor4f, "glColor4f");
	GPA(rtglCopyTexImage2D, "glCopyTexImage2D");
	GPA(rtglDepthFunc, "glDepthFunc");
	GPA(rtglDepthMask, "glDepthMask");
	GPA(rtglDeleteTextures, "glDeleteTextures");
	GPA(rtglDisable, "glDisable");
	GPA(rtglDrawArrays, "glDrawArrays");
	GPA(rtglEnable, "glEnable");
	GPA(rtglEnd, "glEnd");
	GPA(rtglFogf, "glFogf");
	GPA(rtglFogfv, "glFogfv");
	GPA(rtglFogi, "glFogi");
	GPA(rtglGetError, "glGetError");
	GPA(rtglGenTextures, "glGenTextures");
	GPA(rtglGetFloatv, "glGetFloatv");
	GPA(rtglGetIntegerv, "glGetIntegerv");
	GPA(rtglGetTexLevelParameteriv, "glGetTexLevelParameteriv");
	GPA(rtglHint, "glHint");
	GPA(rtglInterleavedArrays, "glInterleavedArrays");
	GPA(rtglLightf, "glLightf");
	GPA(rtglLightfv, "glLightfv");
	GPA(rtglLightModeli, "glLightModeli");
	GPA(rtglLightModelfv, "glLightModelfv");
	GPA(rtglLoadIdentity, "glLoadIdentity");
	GPA(rtglMaterialfv, "glMaterialfv");
	GPA(rtglMatrixMode, "glMatrixMode");
	GPA(rtglNormal3f, "glNormal3f");
	GPA(rtglGetString, "glGetString");
	GPA(rtglPixelStorei, "glPixelStorei");
	GPA(rtglPushAttrib, "glPushAttrib");
	GPA(rtglPopAttrib, "glPopAttrib");
	GPA(rtglPushMatrix, "glPushMatrix");
	GPA(rtglPopMatrix, "glPopMatrix");
	GPA(rtglRotatef, "glRotatef");
	GPA(rtglScalef, "glScalef");
	GPA(rtglShadeModel, "glShadeModel");
	GPA(rtglTranslatef, "glTranslatef");
	GPA(rtglTexCoord2f, "glTexCoord2f");
	GPA(rtglTexEnvi, "glTexEnvi");
	GPA(rtglTexImage2D, "glTexImage2D");
	GPA(rtglTexParameteri, "glTexParameteri");
	GPA(rtglTexParameterf, "glTexParameterf");
	GPA(rtglVertex3f, "glVertex3f");
	GPA(rtglViewport, "glViewport");

	return 1;
}

int _get_shader_gl_functions (void) {
	GPA(rtglShaderSource, "glShaderSource");
	GPA(rtglCreateShader, "glCreateShader");
	GPA(rtglCompileShader, "glCompileShader");
	GPA(rtglAttachShader, "glAttachShader");
	GPA(rtglGetShaderiv, "glGetShaderiv");
	GPA(rtglGetShaderInfoLog, "glGetShaderInfoLog");
	GPA(rtglDeleteShader, "glDeleteShader");
	GPA(rtglLinkProgram, "glLinkProgram");
	GPA(rtglCreateProgram, "glCreateProgram");
	GPA(rtglUseProgram, "glUseProgram");
	GPA(rtglGetProgramiv, "glGetProgramiv");
	GPA(rtglValidateProgram, "glValidateProgram");
	GPA(rtglGetUniformLocation, "glGetUniformLocation");
	GPA(rtglUniform1i, "glUniform1i");
	GPA(rtglActiveTexture, "glActiveTexture");

	return 1;
}
