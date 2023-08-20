/*
Copyright (C) 1994-1995 Apogee Software, Ltd.

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

#include "rt_gl_cap.h"

typedef struct {
	GLubyte r;
	GLubyte g;
	GLubyte b;
} rtgl_pal;

extern rtgl_pal rtgl_palette[256];

boolean		VGL_Bind_Texture(const int lumpnum, const int texture_type);
void		VGL_Setup2DMode(void);
void		VGL_Setup3DMode(void);
void		PrintGLError(void);
rtgl_pal*	VGL_GrabScreenshot(void);
void		VGL_Upload_rotationbuf(const rtgl_pal*);
void		VGL_SetupLevel (const int, const int, const int, const int, const int, const int, int);
void		VGL_DrawPushWall(const fixed, const fixed, const int);
void		VGL_DrawMaskedWall_Vertical (const fixed, const fixed, const int,  const int, const int);
void		VGL_DrawMaskedWall_Horizontal (const fixed, const fixed, const int,  const int, const int);
void		VGL_DrawDoor_Horizontal (const fixed, const fixed, const int,  const int, const int);
void		VGL_DrawDoor_Vertical (const fixed, const fixed, const int,  const int, const int);
void		VGL_DrawShapeFan (const fixed x, const fixed, const fixed);
void		VGL_InitHash(void);
void		VGL_DestroyHash(void);
void		VGL_DrawSky(const fixed);
void		VGL_DrawSpotVis(const byte *, const word *, const unsigned short int *, const int *, const unsigned long *lights);
void		VGL_SetGas( const int );
void		VGL_SetFog(void);
void		VGL_Draw2DTexture(const int x, const int y, const unsigned int width, const unsigned int height, const float texx, const float texy);
void		_update_near_lights(const unsigned long* lights, const int x, const int y);
void		VGL_GrabFramebuffer(void);
void		VGL_Bind_DefaultTexture(const byte*, const unsigned int, const unsigned int);


/* texture types and filters */
#define	RTGL_GENERATE_MIPMAPS		1024
#define	RTGL_TEXTURE_SHIFT_WORKAROUND	2048
#define	RTGL_TEXTURE_PIC		4096
#define	RTGL_TEXTURE_LPIC		8192
#define	RTGL_TEXTURE_WALL		16384
#define	RTGL_TEXTURE_MENUPLANE		32768
#define	RTGL_TEXTURE_TRANSPATCH		65536
#define	RTGL_TEXTURE_FLOOR		131072
#define	RTGL_TEXTURE_SKY		262144
#define	RTGL_TEXTURE_CFONT		524288
#define	RTGL_TEXTURE_PIC_TRANS		1048576
#define	RTGL_TEXTURE_FONT		2097152
#define	RTGL_FILTER_RGBA		4194304
#define	RTGL_TEXTURE_PATCH		8388608
#define	RTGL_TEXTURE_IMENU_ITEM		16777216
#define	RTGL_TEXTURE_LBM		16777216*2

/* reserved lumpnums */
#define RTGL_GRABBED_FRAMEBUFFER	999999999
#define RTGL_INVALID_LUMPNUM		999999998
