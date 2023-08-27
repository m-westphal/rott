/*
This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include <SDL.h>
#include <GL/glu.h>
#include <math.h>
#include <assert.h>

#include "../rt_def.h"
#include "../lumpy.h"
#include "../rt_door.h"
#include "../rt_util.h"
#include "../rt_stat.h"
#include "../z_zone.h"
#include "../w_wad.h"
#include "../modexlib.h"
#include "rt_gl.h"
#include "rt_gl_hash.h"
#include "rt_gl_texture.h"

rtgl_pal rtgl_palette[256];

#define RTGL_TEXTURE_LIMIT_MiB	128
#define RTGL_TEXTURE_OVERHEAD	96*96*4
/* GLOBALS */
lumphash	uploaded_textures;
unsigned int	max_level_height;
int		floornum;
int		ceilingnum;
int		skytop;
int		skybottom;
int		current_lumpnum = 0;
float		fogstart;
float		fogend;
boolean		fogonmap = false;
boolean		fogset = false;
GLint		IFont;
float		billboarding_x[3] = {1,0,0};
float		billboarding_y[3] = {0,1,0};
float		shape_normal[3] = {1,0,0};

//screen resolution
unsigned int rtgl_screen_width = 640;
unsigned int rtgl_screen_height = 480;
float rtgl_hud_aspectratio = 4.0f / 3.0f;

// texture formats
GLint	rtgl_rgb;
GLint	rtgl_rgba;
GLint	rtgl_luminance_alpha;

//caps
int	rtgl_has_compression_rgb;
int	rtgl_has_compression_rgba;
int	rtgl_has_compression_luminance_alpha;
int	rtgl_has_texture_rectangle;
GLfloat rtgl_max_anisotropy;
GLint	rtgl_has_multisample;
int	rtgl_use_multisample;
int	rtgl_use_fog = 1;
int	rtgl_use_lighting = 1;
int	rtgl_use_mipmaps = 1;

int	rtgl_has_shader;
extern GLuint	rtgl_shader_program;

//DEBUG
unsigned int	total_uploaded_textures = 0;

void VGL_InitHash() {
	lumphash_init(&uploaded_textures, 127);
}

void VGL_DestroyHash() {
	lumphash_free(&uploaded_textures);
}

void VGL_Setup2DMode (void) {
	rtglDisable( GL_FOG );
	rtglDisable( GL_LIGHTING );
	rtglDisable( GL_DEPTH_TEST );

	rtglMatrixMode(GL_PROJECTION);
	rtglLoadIdentity();
	gluPerspective( 60.0f, (GLfloat) rtgl_screen_width / (GLfloat) rtgl_screen_height, 0.05f, 96.0f );
	rtglMatrixMode(GL_MODELVIEW);

	rtglLoadIdentity();
	gluLookAt (0.0f,0.0f, 1.72823f, 0.0f,0.0f,0.0f, 0.0f,1.0f,0.0f);		//set [1:-1] y
	rtglScalef((GLfloat) rtgl_hud_aspectratio, 1.0f, 1.0f);		//[1:-1] x
	rtglTranslatef(-1.0f,1.0f,0.0f);
	rtglScalef(1.0f / 160.0f, 1.0f / 100.0f, 1.0f);

	rtglEnable(GL_ALPHA_TEST);
	rtglEnable(GL_BLEND);

	rtglColor4f(1,1,1,1);
}

void VGL_Setup3DMode (void) {
	rtglMatrixMode(GL_MODELVIEW);
	rtglLoadIdentity();
	rtglDisable(GL_ALPHA_TEST);

	fogset = false;
}

static int _get_compressed_texture_space(int size) {
	int space = 0, mml = 0;
	GLint ret_space = 0;

	for ( ; size > 0 && rtglGetError() == GL_NO_ERROR; size /= 2, mml++) {
		rtglGetTexLevelParameteriv( GL_TEXTURE_2D, mml,  GL_TEXTURE_COMPRESSED_IMAGE_SIZE_ARB, &ret_space);
		space += ret_space;
	}

	assert (space >= 0);

	if (rtglGetError() == GL_NO_ERROR)
		return space;

	return 0;
}

static int _mipmapsize(int i, const int d) {
	if (i > 0)
		return _mipmapsize(i/2,d)+(i*i*d);

	return 0;
}

void
VGL_GrabFramebuffer(void) {
	GLuint store;
	GLuint *gltexture = lumphash_get(&uploaded_textures, RTGL_GRABBED_FRAMEBUFFER);

	assert (rtgl_has_texture_rectangle);

	if (gltexture) {
		rtglDisable(GL_TEXTURE_2D);
		rtglEnable(GL_TEXTURE_RECTANGLE_NV);
	}
	else {
		while( (unsigned int) RTGL_TEXTURE_LIMIT_MiB*1024*1024 < uploaded_textures.size + rtgl_screen_width*rtgl_screen_height*3+RTGL_TEXTURE_OVERHEAD) {
			assert (uploaded_textures.hash_fillcount > 0);
			lumphash_delete(&uploaded_textures, uploaded_textures.least_active->lumpnum);
		}
		rtglEnable(GL_TEXTURE_RECTANGLE_NV);
		gltexture = &store;
		rtglGenTextures (1, gltexture);

		lumphash_add(&uploaded_textures, RTGL_GRABBED_FRAMEBUFFER, *gltexture, rtgl_screen_width*rtgl_screen_height*3 + RTGL_TEXTURE_OVERHEAD);
	}

	rtglBindTexture(GL_TEXTURE_RECTANGLE_NV, *gltexture);
	rtglCopyTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, rtgl_rgb, 0, 0, rtgl_screen_width, rtgl_screen_height, 0);
}

void
VGL_Bind_DefaultTexture(const byte* pic, const unsigned int width, const unsigned int height) {
	unsigned int i, j;
	extern rtgl_pal rtgl_palette[256];
	GLubyte* texture_rendered = texture_allocquad(256, 3);
	GLubyte* ptr;

	assert(width <= 256);
	assert(height <= 256);

	rtglPixelStorei (GL_UNPACK_ALIGNMENT, 1);
	rtglBindTexture(GL_TEXTURE_2D, 0);

	current_lumpnum = RTGL_INVALID_LUMPNUM;

	for(i = 0; i < width; i++) {
		ptr = texture_rendered + i*256*sizeof(GLubyte)*3;
		for(j = 0; j < height; j++, pic++) {
			*ptr++ = rtgl_palette[*pic].r;
			*ptr++ = rtgl_palette[*pic].g;
			*ptr++ = rtgl_palette[*pic].b;
		}
	}
	rtglTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	rtglTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	rtglTexImage2D(GL_TEXTURE_2D, 0, rtgl_rgb, 256, 256, 0, GL_RGB, GL_UNSIGNED_BYTE, texture_rendered);
	free(texture_rendered);
}

boolean
VGL_Bind_Texture(const int lumpnum, const int texture_req)
{
	extern rtgl_pal rtgl_palette[256];
	GLuint store;
	unsigned int i;
	unsigned int size, space;
	patch_t* texture;
	byte* texture_mem;
	GLubyte *texture_rendered = NULL, *tmpptr;
	GLenum iformat, format;

	if (current_lumpnum == lumpnum)
		return true;

	current_lumpnum = lumpnum;

	GLuint *gltexture = lumphash_get(&uploaded_textures, lumpnum);

	if (gltexture) {
//		int stat;
		rtglBindTexture(GL_TEXTURE_2D, *gltexture);
//		glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_RESIDENT, &stat);
//		if (stat == GL_FALSE)
//			printf("Non-resident texture found\n");
		return rtglGetError() == GL_NO_ERROR;
	}

	/* default values */
	gltexture = &store;
	rtglPixelStorei (GL_UNPACK_ALIGNMENT, 1);
	rtglGenTextures (1, gltexture);
	rtglBindTexture (GL_TEXTURE_2D, *gltexture);

//	rtglTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_PRIORITY, 10);

	rtglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	rtglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	size = texture_req % 1024;
	assert(size > 0);

	switch ( ((((texture_req) - size) & ~RTGL_GENERATE_MIPMAPS) & ~RTGL_TEXTURE_SHIFT_WORKAROUND) & ~RTGL_FILTER_RGBA ) {
		case RTGL_TEXTURE_FONT:
			texture_rendered = texture_render_font ( (font_t*) W_CacheLumpNum ( lumpnum, PU_CACHE, Cvt_font_t, 1), size );
			size *= 16;
			format = GL_RGBA;
			iformat = rtgl_rgba;
			break;
		case RTGL_TEXTURE_CFONT:
			texture_rendered = texture_render_cfont ( (cfont_t*) W_CacheLumpNum ( lumpnum, PU_CACHE, Cvt_cfont_t, 1), size);
			size *= 16;
			format = GL_LUMINANCE_ALPHA;
			iformat = rtgl_luminance_alpha;
			break;
		case RTGL_TEXTURE_PIC:
			texture_rendered = texture_render_pic( (pic_t*) W_CacheLumpNum ( lumpnum, PU_CACHE, Cvt_pic_t, 1), size);
			format = GL_RGB;
			iformat = rtgl_rgb;
			break;
		case RTGL_TEXTURE_PIC_TRANS:
			texture_rendered = texture_render_pic_trans( (pic_t*) W_CacheLumpNum ( lumpnum, PU_CACHE, Cvt_pic_t, 1), size);
			format = GL_RGBA;
			iformat = rtgl_rgba;
			break;
		case RTGL_TEXTURE_LPIC:
			texture_rendered = texture_render_lpic( (lpic_t*) W_CacheLumpNum ( lumpnum, PU_CACHE, Cvt_lpic_t, 1));
			format = GL_RGB;
			iformat = rtgl_rgb;
			break;
		case RTGL_TEXTURE_PATCH:
			//WORKAROUND colored patches
			if (lumpnum > 0)
				texture_rendered = texture_render_patch( (patch_t*) W_CacheLumpNum ( lumpnum, PU_CACHE, Cvt_patch_t, 1), size);
			else
				texture_rendered = texture_render_colored_patch( (patch_t*) W_CacheLumpNum ( (-1 * lumpnum) / 12, PU_CACHE, Cvt_patch_t, 1), (-1 * lumpnum) % 12);
			format = GL_RGBA;
			iformat = rtgl_rgba;
			break;
		case RTGL_TEXTURE_SKY:
			texture_rendered = texture_render_sky( (byte*) W_CacheLumpNum ( lumpnum, PU_STATIC, CvtNull, 1));
			format = GL_RGB;
			iformat = rtgl_rgb;
			assert (size == 256);
			break;
		case RTGL_TEXTURE_FLOOR:
			texture = (patch_t*) W_CacheLumpNum ( lumpnum, PU_LEVELSTRUCT, Cvt_patch_t,1);

			rtglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			rtglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

			/* wtf? format? */
			texture_mem = (byte*) texture + 8;
			assert (size == 128);
			texture_rendered = texture_allocquad(size, 3);
			for (i = 0; i < size*size; i++) {
				texture_rendered[3*i] = rtgl_palette[ texture_mem[i] ].r;
				texture_rendered[3*i+1] = rtgl_palette[ texture_mem[i] ].g;
				texture_rendered[3*i+2] = rtgl_palette[ texture_mem[i] ].b;
			}

			format = GL_RGB;
			iformat = rtgl_rgb;
			break;
		case RTGL_TEXTURE_TRANSPATCH:
			if (texture_req & RTGL_TEXTURE_SHIFT_WORKAROUND)
				texture_rendered = texture_render_transpatch( (transpatch_t*) W_CacheLumpNum(lumpnum, PU_CACHE, Cvt_transpatch_t, 1), size, -1);
			else
				texture_rendered = texture_render_transpatch( (transpatch_t*) W_CacheLumpNum(lumpnum, PU_CACHE, Cvt_transpatch_t, 1), size, 0);

			format = GL_RGBA;
			iformat = rtgl_rgba;
			space = size*size*4;
			break;
		case RTGL_TEXTURE_WALL:
			texture_mem = (byte*) W_CacheLumpNum ( lumpnum, PU_CACHE, CvtNull, 1);
			assert(size == 64);

			texture_rendered = texture_allocquad(size, 3);

			rtglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			rtglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

			for ( i = 0; i < size; i++) {
				unsigned int j;
				for (j = 0; j < size; j++) {
					tmpptr = texture_rendered+i*size*sizeof(GLubyte)*3 + j*sizeof(GLubyte)*3;

					*tmpptr++ = rtgl_palette[*texture_mem].r;
					*tmpptr++ = rtgl_palette[*texture_mem].g;
					*tmpptr = rtgl_palette[*texture_mem].b;
					texture_mem++;
				}
			}

			format = GL_RGB;
			iformat = rtgl_rgb;
			break;
		case RTGL_TEXTURE_MENUPLANE:
			texture_mem = (byte*) W_CacheLumpNum ( lumpnum, PU_CACHE, CvtNull, 1);
			texture_mem += 8;

			assert (size == 512);
			texture_rendered = texture_allocquad(size, 3);

			rtglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			rtglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

			for ( i = 0; i < 288; i++) {
				int j;
				for (j = 0; j < 158; j++) {
					tmpptr = texture_rendered+i*size*sizeof(GLubyte)*3 + j*sizeof(GLubyte)*3;

					*tmpptr++ = rtgl_palette[*texture_mem].r;
					*tmpptr++ = rtgl_palette[*texture_mem].g;
					*tmpptr = rtgl_palette[*texture_mem].b;
					texture_mem++;
				}
			}
			format = GL_RGB;
			iformat = rtgl_rgb;
			break;
		case RTGL_TEXTURE_IMENU_ITEM:
			texture_rendered = texture_render_imenu_item( (patch_t*) W_CacheLumpNum ( lumpnum, PU_CACHE, Cvt_patch_t, 1), size);
			format = GL_RGBA;
			iformat = rtgl_rgba;
			break;
		case RTGL_TEXTURE_LBM:
			texture_rendered = texture_render_lbm ( (lbm_t *) W_CacheLumpNum( lumpnum, PU_CACHE, Cvt_lbm_t, 1), size);
			format = GL_RGB;
			iformat = rtgl_rgb;
			break;
		default:
			printf("RT_GL: Unsupported texture type: %d\n", texture_req);
			assert(false);
			return -1;
	}

	assert (texture_rendered != NULL);
	if (texture_req & RTGL_FILTER_RGBA) {
		assert (format == GL_RGBA);
		texture_filter_rgba (texture_rendered, size);
	}

	switch (format) {
		case GL_RGB:			space = size * size * 3; break;
		case GL_RGBA:			space = size * size * 4; break;
		case GL_LUMINANCE_ALPHA:	space = size * size * 2; break;
		default: assert(false);
	}

	/* texture limit */
	while( (unsigned int) RTGL_TEXTURE_LIMIT_MiB*1024*1024 < uploaded_textures.size + size*size*4+RTGL_TEXTURE_OVERHEAD) {
		assert (uploaded_textures.hash_fillcount > 0);
		lumphash_delete(&uploaded_textures, uploaded_textures.least_active->lumpnum);
	}

	assert (texture_rendered != NULL);
	if ((texture_req & RTGL_GENERATE_MIPMAPS) && rtgl_use_mipmaps) {
		assert (format != GL_LUMINANCE_ALPHA); /* makes no sense */

		rtglTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		rtglTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		if (rtgl_max_anisotropy > 1.0f)
			rtglTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, rtgl_max_anisotropy);	//FIXME
		gluBuild2DMipmaps(GL_TEXTURE_2D, iformat, size, size, format, GL_UNSIGNED_BYTE, texture_rendered);

		/* additional space for mipmaps */
		if ( iformat == GL_COMPRESSED_RGB_ARB || iformat == GL_COMPRESSED_RGBA_ARB) {
			int space2 = _get_compressed_texture_space(size);
			if (space2 != 0) space = space2;
		}
		else {
			switch(format) {
				case GL_RGB:			space += _mipmapsize(size/2, 3); break;
				case GL_RGBA:			space += _mipmapsize(size/2, 4); break;
				case GL_LUMINANCE_ALPHA:	space += _mipmapsize(size/2, 2); break;
			}
		}
	}
	else {
		rtglTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		rtglTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		rtglTexImage2D(GL_TEXTURE_2D, 0, iformat, size, size, 0, format, GL_UNSIGNED_BYTE, texture_rendered);

		if ( iformat == GL_COMPRESSED_RGB_ARB || iformat == GL_COMPRESSED_RGBA_ARB || iformat == GL_COMPRESSED_LUMINANCE_ALPHA) {
			GLint ret_space = 0;
			rtglGetTexLevelParameteriv( GL_TEXTURE_2D, 0,  GL_TEXTURE_COMPRESSED_IMAGE_SIZE_ARB, &ret_space);
			if (rtglGetError() == GL_NO_ERROR && ret_space > 0)
				space = ret_space;
		}
	}

	free(texture_rendered);

	lumphash_add(&uploaded_textures, lumpnum, *gltexture, space+RTGL_TEXTURE_OVERHEAD); //insert into hash thingy

	printf("Current # Of Textures In Hashtable: %zu / %zu / %f MiB | ", uploaded_textures.hash_fillcount, uploaded_textures.hash_size, (float) uploaded_textures.size / (1024.0f*1024.0f));
	printf("Total Textures Processed: %d\n", ++total_uploaded_textures);

	return true;
}

void PrintGLError(void) {
	switch ( rtglGetError() ) {
		case GL_INVALID_OPERATION:	printf("GL_INVALID_OPERATION\n"); break;
		case GL_INVALID_ENUM:		printf("GL_INVALID_ENUM\n"); break;
		case GL_INVALID_VALUE:		printf("GL_INVALID_VALUE\n"); break;
		case GL_NO_ERROR:	//	printf("GL_NO_ERROR\n");
						break;
		default:			printf("PrintGLError: Unhandled GL_ERROR\n");
	}
}

//FIXME
rtgl_pal* VGL_GrabScreenshot(void) {
	rtgl_pal*	screen;
/*	GLubyte		fullscreen[640*480];

	if ( ( screen = malloc(320*200 * sizeof(rtgl_pal)) ) == NULL) {
		printf("Error");
		return NULL;
	}

	//glReadPixels (0, 0, 640, 480, GL_RGB, GL_UNSIGNED_BYTE, fullscreen);
	PrintGLError();

	//FIXME
	unsigned x;
	int y;
	unsigned j = 0;

	for (y = 480-1; y >= 0; y-=2)
		for (x = 0; x < 640; x+=2)
			if ( j < 320*200) {
			screen[j].r = 0;
			screen[j].g = 0;
			screen[j++].b = 0;
			}

	return screen;*/
	if ( (screen = malloc(320*200 * sizeof(rtgl_pal)) ) == NULL) {
		Error("malloc failed in VGL_GrabScreenshot\n");
	}

	return screen;
}

static inline void _generate_wall (const GLfloat x1, const GLfloat y1, const GLfloat x2, const GLfloat y2, const unsigned int start, const unsigned int end) {
	unsigned int i;

	assert (start < end);

	for (i = start; i < end; i++) {
		GLfloat vertex_and_tex[20] = {  1, 0, x1, (GLfloat) i, y1,
						1, 1, x2, (GLfloat) i, y2,
						0, 1, x2, (GLfloat) i + 1, y2,
						0, 0, x1, (GLfloat) i + 1, y1 };
		rtglInterleavedArrays(GL_T2F_V3F, 0, vertex_and_tex);
		rtglDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	}
}

static inline int _IsNormalWall(const word w) {
	if ( w & 0x8000 || w & 0x2000 || w == 0 )
		return 0;

	if (w & 0x1000)
		VGL_Bind_Texture ( animwalls[w&0x3ff].texture, RTGL_TEXTURE_WALL | RTGL_GENERATE_MIPMAPS | 64 );
	else
		VGL_Bind_Texture (w & 0x3ff, RTGL_TEXTURE_WALL | RTGL_GENERATE_MIPMAPS | 64 );

	return 1;
}

static inline void _generate_vertical_walls(const word *map, const unsigned xoff, const unsigned yoff, const unsigned short int *raw) {
	if (map[1] & 0x2000 && raw[yoff*128 + xoff + 128] != 13) {
		rtglNormal3f(0,0,-1);
		VGL_Bind_Texture( map[1] & 0x3ff, RTGL_TEXTURE_WALL | RTGL_GENERATE_MIPMAPS | 64);
		_generate_wall(xoff+1, yoff+1, xoff, yoff+1, 0, 1);
		if (max_level_height > 1) {
			VGL_Bind_Texture( raw[yoff*128 + xoff + 128] + 1, RTGL_TEXTURE_WALL | RTGL_GENERATE_MIPMAPS | 64);
			_generate_wall(xoff+1, yoff+1, xoff, yoff+1, 1, max_level_height);
		}
	} else
	if (_IsNormalWall(map[1])) {
		rtglNormal3f(0,0,-1);
		_generate_wall(xoff+1, yoff+1, xoff, yoff+1, 0, max_level_height);
	}

	if (map[-1] & 0x2000 && raw[yoff*128 + xoff - 128] != 13) {
		rtglNormal3f(0,0,1);
		VGL_Bind_Texture( map[-1] & 0x3ff, RTGL_TEXTURE_WALL | RTGL_GENERATE_MIPMAPS | 64);
		_generate_wall(xoff, yoff, xoff+1, yoff, 0, 1);
		if (max_level_height > 1) {
			VGL_Bind_Texture( raw[yoff*128 + xoff - 128] + 1, RTGL_TEXTURE_WALL | RTGL_GENERATE_MIPMAPS | 64);
			_generate_wall(xoff, yoff, xoff+1, yoff, 1, max_level_height);
		}
	} else
	if (_IsNormalWall(map[-1])) {
		rtglNormal3f(0,0,1);
		_generate_wall(xoff, yoff, xoff+1, yoff, 0, max_level_height);
	}
}

static inline void _generate_horizontal_walls(const word *map, const unsigned xoff, const unsigned yoff, const unsigned short int *raw) {
	if (map[128] & 0x2000 && raw[yoff*128 + xoff + 1] != 13) {
		rtglNormal3f(-1,0,0);
		VGL_Bind_Texture( map[128] & 0x3ff, RTGL_TEXTURE_WALL | RTGL_GENERATE_MIPMAPS | 64);
		_generate_wall(xoff+1, yoff, xoff+1, yoff+1, 0, 1);
		if (max_level_height > 1) {
			VGL_Bind_Texture( raw[yoff*128 + xoff + 1] + 1, RTGL_TEXTURE_WALL | RTGL_GENERATE_MIPMAPS | 64);
			_generate_wall(xoff+1, yoff, xoff+1, yoff+1, 1, max_level_height);
		}
	} else
	if (_IsNormalWall(map[128])) {
		rtglNormal3f(-1,0,0);
		_generate_wall(xoff+1, yoff, xoff+1, yoff+1, 0, max_level_height);
	}

	if (map[-128] & 0x2000 && raw[yoff*128 + xoff - 1] != 13) {
		rtglNormal3f(1,0,0);
		VGL_Bind_Texture( map[-128] & 0x3ff, RTGL_TEXTURE_WALL | RTGL_GENERATE_MIPMAPS | 64);
		_generate_wall(xoff, yoff+1, xoff, yoff, 0, 1);
		if (max_level_height > 1) {
			VGL_Bind_Texture( raw[yoff*128 + xoff - 1] + 1, RTGL_TEXTURE_WALL | RTGL_GENERATE_MIPMAPS | 64);
			_generate_wall(xoff, yoff+1, xoff, yoff, 1, max_level_height);
		}
	} else
	if (_IsNormalWall(map[-128])) {
		rtglNormal3f(1,0,0);
		_generate_wall(xoff, yoff+1, xoff, yoff, 0, max_level_height);
	}
}


static inline int _IsWall (const word w) {
	//FIXME WV-GH
	//if ( w & 0x8000 || w & 0x2000 || w == 0 )
	//
	if ( w & 0x8000 || w == 0 )
		return 0;

	return 1;
}


static void _generate_cell (const word* map, const unsigned xoff, const unsigned yoff, const unsigned short int* raw) {
	//FIXME TEXTURE FLIPPING

	//sidepic
	if (*map & 0x8000) {
		if(*map & 0x4000) {		// maskobj at current spot
			if ( maskobjlist[*map&0x3ff]->sidepic) {	// has a sidepic
				if (maskobjlist[*map&0x3ff]->vertical) {
					if (_IsWall(map[1]) || _IsWall(map[-1]) ) {
						VGL_Bind_Texture ( maskobjlist[*map&0x3ff]->sidepic, RTGL_TEXTURE_WALL | RTGL_GENERATE_MIPMAPS | 64 );
						if (! (map[1] & 0x8000)) {
							rtglNormal3f(0,0,-1);
							_generate_wall(xoff+1, yoff+1, xoff, yoff+1, 0, max_level_height);
						}
						if (! (map[-1] & 0x8000)) {
							rtglNormal3f(0,0,1);
							_generate_wall(xoff, yoff, xoff+1, yoff, 0, max_level_height);
						}
					}
					_generate_horizontal_walls(map, xoff, yoff, raw);
					return;
				}
				else if (_IsWall(map[128]) || _IsWall(map[-128])) {
					VGL_Bind_Texture ( maskobjlist[*map&0x3ff]->sidepic, RTGL_TEXTURE_WALL | RTGL_GENERATE_MIPMAPS | 64 );
					if (! (map[128] & 0x8000)) {
						rtglNormal3f(-1,0,0);
						_generate_wall(xoff+1, yoff, xoff+1, yoff+1, 0, max_level_height);
					}
					if (! (map[-128] & 0x8000)) {
						rtglNormal3f(1,0,0);
						_generate_wall(xoff, yoff+1, xoff, yoff, 0, max_level_height);
					}
				}
				_generate_vertical_walls(map, xoff, yoff, raw);
				return;
			}
		}
		// door at current pos
		else if (doorobjlist[*map&0x3ff]->sidepic) {
			if (doorobjlist[*map&0x3ff]->vertical) {
				if (_IsWall(map[1]) || _IsWall(map[-1]) ) {
					VGL_Bind_Texture ( doorobjlist[*map&0x3ff]->sidepic, RTGL_TEXTURE_WALL | RTGL_GENERATE_MIPMAPS | 64 );
					if (! (map[1] & 0x8000)) {
						rtglNormal3f(0,0,-1);
						_generate_wall(xoff+1, yoff+1, xoff, yoff+1, 0, max_level_height);
					}
					if (! (map[-1] & 0x8000)) {
						rtglNormal3f(0,0,1);
						_generate_wall(xoff, yoff, xoff+1, yoff, 0, max_level_height);
					}
				}
				_generate_horizontal_walls(map, xoff, yoff, raw);
				return;
			}
			else {
				if (_IsWall(map[128]) || _IsWall(map[-128]) ) {
					VGL_Bind_Texture ( doorobjlist[*map&0x3ff]->sidepic, RTGL_TEXTURE_WALL | RTGL_GENERATE_MIPMAPS | 64 );
					if (! (map[128] & 0x8000)) {
						rtglNormal3f(-1,0,0);
						_generate_wall(xoff+1, yoff, xoff+1, yoff+1, 0, max_level_height);
					}
					if (! (map[-128] & 0x8000)) {
						rtglNormal3f(1,0,0);
						_generate_wall(xoff, yoff+1, xoff, yoff, 0, max_level_height);
					}
				}
			}
			_generate_vertical_walls(map, xoff, yoff, raw);
			return;
		}
	}

	_generate_vertical_walls(map, xoff, yoff, raw);
	_generate_horizontal_walls(map, xoff, yoff, raw);
}

/* workaround: GetFloorCeilingLump only defined in rt_floor.c */
int GetFloorCeilingLump ( int );

void	VGL_SetupLevel (const int height, const int fn, const int cn, const int baseminshade, const int basemaxshade, const int fog, int darknesslevel) {
	max_level_height = height;

	if (cn >= 234) {	//SKY
		ceilingnum = cn - 233;
		if (ceilingnum < 1 || ceilingnum > 6)
			Error("Illegal Sky Tile = %d\n", ceilingnum);
		//FIXME Horizon height stuff

		skybottom = W_GetNumForName("SKYSTART") + 2*ceilingnum - 1;
		skytop = skybottom + 1;

		ceilingnum = 0;	//enable drawing floor
	}
	else //disable drawing floor
		ceilingnum = GetFloorCeilingLump(cn - 197);

/* HORIZON HEIGHT:
	 crud=(word)MAPSPOT(1,0,1);
	 if ((crud>=90) && (crud<=97))
	    horizonheight=crud-89;
	else if ((crud>=450) && (crud<=457))
		 horizonheight=crud-450+9;
	 else
	Error("You must specify a valid horizon height sprite icon over the sky at (2,0) on map %ld\n",gamestate.mapon);
	// Check for lightning icon
*/

	floornum = GetFloorCeilingLump(fn);

	// fog
	fogset = false;
	rtglFogi(GL_FOG_MODE, GL_LINEAR);

	if (fog == 1) {
		fogstart = (float) baseminshade / 5.0f;
		fogend = (float) basemaxshade*2.0f;

		printf("Fog start/end: %f %f\n", fogstart, fogend);
		fogonmap = true;
		darknesslevel = 7;	//fix foggy mountain (...)
	}
	else {
		printf("No fog\n");
		fogstart = 0.0f;	//still used for gas
		fogend = 100.0f;
		fogonmap = false;
	}

	VGL_SetFog();

	//set light0
	{
		const GLfloat white[4] = {((float)  darknesslevel+10) / 22.0f,
				((float)  darknesslevel+10) / 22.0f,
				((float)  darknesslevel+10) / 22.0f,
				1.0f};
		rtglLightfv(GL_LIGHT0, GL_AMBIENT, white);
		rtglLightfv(GL_LIGHT0, GL_DIFFUSE, white);
	}
	{
		const GLfloat white[4] = { 0, 0, 0, 1.0f };
		rtglLightfv(GL_LIGHT0, GL_SPECULAR, white);
	}

	assert(!(darknesslevel < 0));
	assert(darknesslevel <= 7);

	rtglEnable(GL_LIGHT0);
	rtglLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 1.0f);
	rtglLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, (((float) baseminshade/2.0f + 8.0f) / ((GLfloat) darknesslevel+1.0f)) );
	rtglLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 50.0f * ((GLfloat) basemaxshade) / ( ((GLfloat) darknesslevel+1.0f) * ((GLfloat) darknesslevel+1.0f) * 1000.0f));	//guess... should be based on maxshade

	{
		const GLfloat values[] = { 1.0f, 1.0f, 1.0f, 1.0f};
		rtglMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, values);
	}

	{
		const GLfloat values[] = { 0.8f, 0.8f, 0.8f, 1.0f};
		rtglMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, values);
	}

	{
		const GLfloat values[] = { 0.2f, 0.2f, 0.2f, 1.0f};
		rtglMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, values);
	}

	{
		const GLfloat values[] = { 0.05f, 0.05f, 0.05f, 1.0f};
		rtglLightModelfv( GL_LIGHT_MODEL_AMBIENT, values);
	}
}

void VGL_SetGas( const int g ) {
	float div = 16 - g;
	float gas[4] = {0, 1.0f - ( (float) g / 70.0f ), 0, 1};

	gas[0] = 1.0f - ((float) g) / 15.0f;
	gas[2] = gas[0];

	rtglFogf(GL_FOG_START, (float) fogstart - (((float) fogstart) / div) );
	rtglFogf(GL_FOG_END, (float) fogend - (((float) fogend - 15.0f) / div) );

	rtglFogfv(GL_FOG_COLOR, gas);

	//change light color
	rtglLightfv(GL_LIGHT0, GL_AMBIENT, gas);
	rtglLightfv(GL_LIGHT0, GL_DIFFUSE, gas);

	rtglEnable(GL_FOG);
}

void VGL_SetFog() {
	if (fogonmap == false) {
		const GLfloat black[] = {0,0,0,0}; // shader always blends in fog color
		rtglFogfv(GL_FOG_COLOR, black);

		rtglFogf(GL_FOG_START, fogstart); // shader always requires values to be initialized
		rtglFogf(GL_FOG_END, fogend);

		rtglDisable(GL_FOG);
		return;
	}

	rtglEnable(GL_FOG);

	const GLfloat white[] = {1,1,1,1};
	rtglFogfv(GL_FOG_COLOR, white);

	rtglFogf(GL_FOG_START, fogstart);
	rtglFogf(GL_FOG_END, fogend);
}

void	VGL_DrawMaskedWall_Vertical (const fixed x, const fixed y, const int bot_text,  const int mid_text, const int top_text) {

	if ( bot_text >= 0) {
		VGL_Bind_Texture (bot_text, RTGL_TEXTURE_TRANSPATCH | RTGL_GENERATE_MIPMAPS | RTGL_FILTER_RGBA | 64);

		_generate_wall(x / 65536.0f + 0.5f , y / 65536.0f, x / 65536.0f + 0.5f, y/65536.0f+1, 0, 1);
	}
	//FIXME?
	if ( mid_text >= 0 && max_level_height > 2) {
		VGL_Bind_Texture (mid_text, RTGL_TEXTURE_TRANSPATCH | RTGL_GENERATE_MIPMAPS | RTGL_FILTER_RGBA | RTGL_TEXTURE_SHIFT_WORKAROUND | 64 );

		_generate_wall(x / 65536.0f + 0.5f , y / 65536.0f, x / 65536.0f + 0.5f, y/65536.0f+1, 1, max_level_height - 1);
	}
	if ( top_text >= 0 && max_level_height > 1) {
		VGL_Bind_Texture (top_text, RTGL_TEXTURE_TRANSPATCH | RTGL_GENERATE_MIPMAPS | RTGL_FILTER_RGBA | RTGL_TEXTURE_SHIFT_WORKAROUND | 64);

		_generate_wall(x / 65536.0f + 0.5f , y / 65536.0f, x / 65536.0f + 0.5f, y/65536.0f+1, max_level_height - 1, max_level_height);
	}
}

void	VGL_DrawMaskedWall_Horizontal (const fixed x, const fixed y, const int bot_text,  const int mid_text, const int top_text) {

	if ( bot_text >= 0) {
		VGL_Bind_Texture (bot_text, RTGL_TEXTURE_TRANSPATCH | RTGL_GENERATE_MIPMAPS | RTGL_FILTER_RGBA | 64);

		_generate_wall(x / 65536.0f, y / 65536.0f + 0.5f, x / 65536.0f + 1, y/65536.0f+0.5f, 0, 1);
	}
	//FIXME?
	if ( mid_text >= 0 && max_level_height > 2) {
		VGL_Bind_Texture (mid_text, RTGL_TEXTURE_TRANSPATCH | RTGL_GENERATE_MIPMAPS | RTGL_FILTER_RGBA | RTGL_TEXTURE_SHIFT_WORKAROUND | 64);

		_generate_wall(x / 65536.0f, y / 65536.0f + 0.5f, x / 65536.0f + 1, y/65536.0f+0.5f, 1, max_level_height - 1);
	}
	if ( top_text >= 0 && max_level_height > 1) {
		VGL_Bind_Texture (top_text, RTGL_TEXTURE_TRANSPATCH | RTGL_GENERATE_MIPMAPS | RTGL_FILTER_RGBA | RTGL_TEXTURE_SHIFT_WORKAROUND | 64);

		_generate_wall(x / 65536.0f, y / 65536.0f + 0.5f, x / 65536.0f + 1, y/65536.0f+0.5f, max_level_height - 1, max_level_height);
	}
}

void	VGL_DrawDoor_Vertical (const fixed x, const fixed y, const int bot_text,  const int top_text, const int base_text) {

	if ( bot_text >= 0) {
		if (bot_text != base_text) {
			rtglEnable( GL_ALPHA_TEST );
			rtglEnable( GL_BLEND );
			VGL_Bind_Texture (bot_text, RTGL_TEXTURE_TRANSPATCH | RTGL_GENERATE_MIPMAPS | RTGL_FILTER_RGBA | RTGL_TEXTURE_SHIFT_WORKAROUND | 64);

			_generate_wall(x / 65536.0f + 0.5f, y / 65536.0f, x / 65536.0f + 0.5f, y/65536.0f + 1, 0, 1);

			rtglDisable ( GL_ALPHA_TEST );
			rtglDisable ( GL_BLEND );
		}
		else {
			VGL_Bind_Texture (bot_text, RTGL_TEXTURE_WALL | RTGL_GENERATE_MIPMAPS | 64);
			_generate_wall(x / 65536.0f + 0.5f, y / 65536.0f, x / 65536.0f + 0.5f, y/65536.0f + 1, 0, 1);
		}
	}
	if ( top_text >= 0 && max_level_height > 1) {
		VGL_Bind_Texture (top_text, RTGL_TEXTURE_WALL | RTGL_GENERATE_MIPMAPS | 64);

		_generate_wall(x / 65536.0f + 0.5f, y / 65536.0f, x / 65536.0f + 0.5f, y/65536.0f + 1, 1, max_level_height);
	}
}

void	VGL_DrawDoor_Horizontal (const fixed x, const fixed y, const int bot_text,  const int top_text, const int base_text) {

	if (bot_text >= 0) {
		if (bot_text != base_text) {
			rtglEnable( GL_ALPHA_TEST );
			rtglEnable( GL_BLEND );
			VGL_Bind_Texture (bot_text, RTGL_TEXTURE_TRANSPATCH | RTGL_GENERATE_MIPMAPS | RTGL_FILTER_RGBA | RTGL_TEXTURE_SHIFT_WORKAROUND | 64);

			_generate_wall(x / 65536.0f, y / 65536.0f + 0.5f, x / 65536.0f + 1, y/65536.0f + 0.5f, 0, 1);

			rtglDisable ( GL_ALPHA_TEST );
			rtglDisable ( GL_BLEND );
		}
		else {
			VGL_Bind_Texture (bot_text, RTGL_TEXTURE_WALL | RTGL_GENERATE_MIPMAPS | 64);
			_generate_wall(x / 65536.0f, y / 65536.0f + 0.5f, x / 65536.0f + 1, y/65536.0f + 0.5f, 0, 1);
		}
	}
	if ( top_text >= 0 && max_level_height > 1) {
		VGL_Bind_Texture (top_text, RTGL_TEXTURE_WALL | RTGL_GENERATE_MIPMAPS | 64);

		_generate_wall(x / 65536.0f, y / 65536.0f + 0.5f, x / 65536.0f + 1, y/65536.0f + 0.5f, 1, max_level_height);
	}
}

void	VGL_DrawPushWall(const fixed x, const fixed y, const int texture) {
	int new_texture;

	if (texture & 0x1000)
		new_texture = animwalls[texture & 0x3ff].texture;
	else
		new_texture = texture & 0x3ff;


	if (new_texture > 0)
		VGL_Bind_Texture (new_texture, RTGL_TEXTURE_WALL | RTGL_GENERATE_MIPMAPS | 64);

	rtglNormal3f(-1,0,0);
	_generate_wall(x / 65536.0f - 0.5f, y / 65536.0f - 0.5f, x / 65536.0f - 0.5f, y/65536.0f + 0.5f, 0, max_level_height);

	rtglNormal3f(1,0,0);
	_generate_wall(x / 65536.0f + 0.5f, y / 65536.0f + 0.5f, x / 65536.0f + 0.5f, y/65536.0f - 0.5f, 0, max_level_height);

	rtglNormal3f(0,0,1);
	_generate_wall(x / 65536.0f - 0.5f, y / 65536.0f + 0.5f, x / 65536.0f + 0.5f, y/65536.0f + 0.5f, 0, max_level_height);

	rtglNormal3f(0,0,-1);
	_generate_wall(x / 65536.0f + 0.5f, y / 65536.0f - 0.5f, x / 65536.0f - 0.5f, y/65536.0f - 0.5f, 0, max_level_height);
}

void	VGL_DrawShapeFan ( const fixed x, const fixed y, const fixed z) {
	float width = 1;
	float height = 1;

	//NOTE: billboarding only alignes to camera plane
	rtglNormal3f ( shape_normal[0], shape_normal[1], shape_normal[2] );

	const float shape_fan[20] = {	1, 0,  x / 65536.0f - billboarding_x[0] * width * 0.5f, max_level_height-1-(float)z / 64.0f - billboarding_x[1] * width * 0.5f, y / 65536.0f - billboarding_x[2] * (width * 0.5f),
				1, 1, x / 65536.0f + billboarding_x[0] * width * 0.5f, max_level_height-1-(float)z / 64.0f + billboarding_x[1] * width * 0.5f, y / 65536.0f + billboarding_x[2] * width * 0.5f,
				0, 1, x / 65536.0f + billboarding_x[0] * width * 0.5f + billboarding_y[0] * height, max_level_height-1-(float)z / 64.0f + billboarding_x[1] * width * 0.5f + billboarding_y[1] * height, y / 65536.0f + billboarding_x[2] * width * 0.5f + height * billboarding_y[2],
				0, 0, x / 65536.0f - billboarding_x[0] * width * 0.5f + billboarding_y[0] * height, max_level_height-1-(float)z / 64.0f - billboarding_x[1] * width * 0.5f + billboarding_y[1] * height, y / 65536.0f - billboarding_x[2] * width * 0.5f + height * billboarding_y[2] };

	rtglInterleavedArrays(GL_T2F_V3F, 0, shape_fan);
	rtglDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void VGL_DrawSky(const fixed z) {
	unsigned int i;

	if (ceilingnum == 0) {
		float shift = -1 * ((float) -z / 64.0f  + max_level_height - 2) / 50.0f;

		//FIXME HORIZONOFFSET

		if (shift < -2)
			shift = -2;
		else if (shift > 2)
			shift = 2;

		rtglPushMatrix();

		rtglDepthMask(GL_FALSE);
		rtglDisable(GL_DEPTH_TEST);

		rtglTranslatef(0, shift, 0);


		VGL_Bind_Texture(skytop, RTGL_TEXTURE_SKY | 256);
		{
			const GLfloat vertex_and_tex[30] = {
							0, 200.0f/256.0f,	-0.4143f, 0, -1,
							1, 200.0f/256.0f,	0.4143f, 0, -1,
							0,   0.0f/256.0f,	-0.4143f, 1, -1,
							1,   0.0f/256.0f,	0.4143f, 1,  -1,
							0, 200.0f/256.0f,	-0.4143f, 5, -1,
							1, 200.0f/256.0f,	0.4143f, 5, -1
						};

			rtglInterleavedArrays(GL_T2F_V3F, 0, vertex_and_tex);
			for (i=0; i < 8; i++) {
				rtglDrawArrays(GL_TRIANGLE_STRIP, 0, 6);
				rtglRotatef(45.0f, 0, 1.0f, 0);
			}
		}

		VGL_Bind_Texture(skybottom, RTGL_TEXTURE_SKY | 256);
		{
			const GLfloat vertex_and_tex[30] = {
							0, 0.0f/256.0f,		-0.4143f, -5, -1,
							1, 0.0f/256.0f,		0.4143f, -5, -1,
							0, 200.0f/256.0f,	-0.4143f, -1, -1,
							1, 200.0f/256.0f,	0.4143f, -1, -1,
							0, 0.0f/256.0f,		-0.4143f, 0, -1,
							1, 0.0f/256.0f,		0.4143f, 0,  -1
						};

			rtglInterleavedArrays(GL_T2F_V3F, 0, vertex_and_tex);
			for (i=0; i < 8; i++) {
				rtglDrawArrays(GL_TRIANGLE_STRIP, 0, 6);
				rtglRotatef(45.0f, 0, 1.0f, 0);
			}
		}

		rtglEnable(GL_DEPTH_TEST);
		rtglDepthMask(GL_TRUE);
		rtglPopMatrix();
	}
}


void _update_near_lights(const unsigned long* lights, const int x, const int y) {
	int i, j, intensity = 0, nr = 0, pos_x = 0, pos_y = 0;

	const int dist = 2;

	for (i = max(0, x - dist); i <= min(127, x + dist); i++)
		for (j = max(0, y - dist); j <= min(127, y + dist); j++) {
			assert(i >= 0 && i <= 127);
			assert(j >= 0 && j <= 127);

			const unsigned long val = *(lights+(i*128)+j);
			if (val != 0) {
				const int dist = 5 * abs(i-x) + 5 * abs(j-y) + 1;
				intensity += (val >> 20) / dist;
				pos_x += i - x;
				pos_y += j - y;
				nr++;
			}
		}

	if(rtgl_has_shader && nr == 0) {
		assert(intensity == 0);
		nr++;
	}
	else if (nr == 0) {
		rtglDisable(GL_LIGHT1);
		return;
	}

	float light_strength = ((float) intensity) / 4096.0f;

	light_strength = fmin(1.0, light_strength);

	if(!rtgl_has_shader)
		rtglEnable(GL_LIGHT1);
	GLfloat white[4] = { (GLfloat) light_strength, (GLfloat) light_strength, (GLfloat) light_strength,1};
	rtglLightfv(GL_LIGHT1, GL_AMBIENT, white);
	rtglLightfv(GL_LIGHT1, GL_DIFFUSE, white);
	rtglLightfv(GL_LIGHT1, GL_SPECULAR, white);
	rtglLightf(GL_LIGHT1, GL_QUADRATIC_ATTENUATION, 3.0f);

	GLfloat pos[4] = { (float)pos_x/ (float) nr + (float) x + 0.5f, 0.5f, (float) pos_y / (float) nr + (float) y + 0.5f, 1};
	rtglLightfv(GL_LIGHT1, GL_POSITION, pos);
}

void VGL_DrawSpotVis (const byte spotvis[MAPSIZE][MAPSIZE], const word tilemap[MAPSIZE][MAPSIZE], const unsigned short int* raw, const int *lightsource, const unsigned long* lights) {
	unsigned int i;
	unsigned int j;
	boolean visible[MAPSIZE][MAPSIZE];

	for (i = 0; i < MAPSIZE; i++)
		for (j = 0; j < MAPSIZE; j++)
			visible[i][j] = false;

	for (i = 1; i < MAPSIZE-1; i++)
		for (j = 1; j < MAPSIZE-1; j++)
			if(spotvis[i][j] == 1)
				visible[i][j] = 1;
			else
				visible[i][j] = 0;

	//Get current billboarding vars
	//NOTE: billboarding only alignes to camera plane
	{
		float modelview[16];
		rtglGetFloatv(GL_MODELVIEW_MATRIX, modelview);

		billboarding_x[0] = modelview[0];
		billboarding_x[1] = modelview[4];
		billboarding_x[2] = modelview[8];
		billboarding_y[0] = modelview[1];
		billboarding_y[1] = modelview[5];
		billboarding_y[2] = modelview[9];

		//calc shape normals
		shape_normal[0] = modelview[4] * modelview[9] - modelview[8] * modelview[5];
		shape_normal[1] = -modelview[0] * modelview[9] + modelview[8] * modelview[1];
		shape_normal[2] = modelview[0] * modelview[5] - modelview[4] * modelview[1];
	}

	//floor
	//floor gfx are too bright
	{
		GLfloat values[4] = { 1.0f, 1.0f, 1.0f, 1.0f};
		rtglMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, values);
	}

	if(rtgl_has_shader) {
		if (*lightsource)
			rtglEnable(GL_LIGHT1);
		rtglUseProgram(rtgl_shader_program);

		GLint tex = rtglGetUniformLocation(rtgl_shader_program, "tex0");
		rtglUniform1i(tex, 0);

		//FIXME: put this somewhere else...
		rtglValidateProgram(rtgl_shader_program);
		GLint ok;
		rtglGetProgramiv(rtgl_shader_program, GL_VALIDATE_STATUS, &ok);
		if(!ok) {
			puts("glValidate() == GL_FALSE\n");
			exit(1);
		}
	}

	rtglNormal3f(0,1,0);
	VGL_Bind_Texture ( floornum, RTGL_TEXTURE_FLOOR | RTGL_GENERATE_MIPMAPS | 128);
	for (i = 1; i < MAPSIZE-1; i++)
		for (j = 1; j < MAPSIZE-1; j++) {
			if (visible[i][j] == true) {
                                GLfloat t_s1, t_s2, t_t1, t_t2;
                                if (i % 2 == 0) {
                                    t_s1 = 0.0f;
                                    t_s2 = 0.5f;
                                }
                                else {
                                    t_s1 = 0.5f;
                                    t_s2 = 1.0f;
                                }
                                if (j % 2 == 0) {
                                    t_t1 = 0.0f;
                                    t_t2 = 0.5f;
                                }
                                else {
                                    t_t1 = 0.5f;
                                    t_t2 = 1.0f;
                                }
				GLfloat vertex_and_tex[20] = {	t_s2, t_t1, (GLfloat) i, 0, (GLfloat) j,
								t_s2, t_t2, (GLfloat) i, 0, (GLfloat) j + 1,
								t_s1, t_t2, (GLfloat) i + 1, 0, (GLfloat) j + 1,
								t_s1, t_t1, (GLfloat) i + 1, 0, (GLfloat) j };
				if (*lightsource) _update_near_lights(lights, i, j);
				rtglInterleavedArrays(GL_T2F_V3F, 0, vertex_and_tex);
				rtglDrawArrays(GL_TRIANGLE_FAN, 0, 4);
			}
		}

	//ceiling
	if (ceilingnum != 0) {
		rtglNormal3f(0,-1,0);
		VGL_Bind_Texture ( ceilingnum, RTGL_TEXTURE_FLOOR | RTGL_GENERATE_MIPMAPS | 128);
		for (i = 1; i < MAPSIZE-1; i++)
			for (j = 1; j < MAPSIZE-1; j++)
				if (visible[i][j] == true) {
						GLfloat	vertex_and_tex[20] = {	1, 0, (GLfloat) i + 1, max_level_height, (GLfloat) j,
										1, 1, (GLfloat) i + 1, max_level_height, (GLfloat) j + 1,
										0, 1, (GLfloat) i, max_level_height, (GLfloat) j + 1,
										0, 0, (GLfloat) i, max_level_height, (GLfloat) j };
						if (*lightsource) _update_near_lights(lights, i, j);
						rtglInterleavedArrays(GL_T2F_V3F, 0, vertex_and_tex);
						rtglDrawArrays(GL_TRIANGLE_FAN, 0, 4);
				}
	}

	{
		GLfloat values[4] = { 1.0f, 1.0f, 1.0f, 1.0f};
		rtglMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, values);
	}

	for (i = 1; i < MAPSIZE-1; i++)
		for (j = 1; j < MAPSIZE-1; j++)
			if (visible[i][j] == true) {
				if (*lightsource) _update_near_lights(lights, i, j);
				_generate_cell (&tilemap[i][j], i, j, raw);
			}

	if(rtgl_has_shader)
		rtglUseProgram((GLuint) 0);

	if (*lightsource)
		rtglDisable(GL_LIGHT1);
}

void VGL_Draw2DTexture(const int x, const int y, const unsigned int width, const unsigned int height, const float texx, const float texy) {
	const GLfloat coords[20] = {	0, 0,		x, -y, 0,
				texy, 0,	x, -y - (int) height, 0,
				texy, texx,	x+(int) width, -y - (int) height, 0,
				0, texx,	x+(int) width, -y, 0
			};
	rtglInterleavedArrays(GL_T2F_V3F, 0, coords);
	rtglDrawArrays(GL_QUADS, 0, 4);
}
