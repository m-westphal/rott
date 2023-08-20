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

#include <assert.h>

#include "../rt_def.h"
#include "../rt_view.h"
#include "../lumpy.h"
#include "../rt_util.h"
#include "rt_gl_texture.h"
#include "rt_gl.h"

#define EDGE_ALPHA 192

inline static void
_copy_color( GLubyte* src, GLubyte* dest) {
	int i;

	for (i = 0; i < 3; i++)
		*(dest+i) = *(src+i);
}


void
texture_filter_rgba(GLubyte* texture, const unsigned int space) {
	unsigned int i,j;
	GLubyte *pixel = texture + (4*(space+1)+3)*sizeof(GLubyte);

	for (j = 1; j < space-1; j++) {
		for (i = 1; i < space-1; i++) {
			if ( *pixel == 255) {	/* solid pixel */
				if (*(pixel - space * 4 * sizeof(GLubyte)) == 0) {
					*pixel = EDGE_ALPHA;
					_copy_color(pixel - 3 * sizeof(GLubyte), pixel-(space*4+3)*sizeof(GLubyte));
				}
				if (*(pixel + space * 4 * sizeof(GLubyte)) == 0) {
					*pixel = EDGE_ALPHA;
					_copy_color(pixel - 3 * sizeof(GLubyte), pixel+(space*4-3)*sizeof(GLubyte));
				}
				if (*(pixel - 4 * sizeof(GLubyte)) == 0) {
					*pixel = EDGE_ALPHA;
					_copy_color(pixel - 3 * sizeof(GLubyte), pixel-7*sizeof(GLubyte));
				}
				if (*(pixel + 4 * sizeof(GLubyte)) == 0) {
					*pixel = EDGE_ALPHA;
					_copy_color(pixel - 3 * sizeof(GLubyte), pixel+1*sizeof(GLubyte));
				}
			}
			pixel += 4*sizeof(GLubyte);
		}
		pixel += 2 * 4*sizeof(GLubyte);
	}
}

void*
texture_allocquad(size_t xy, size_t colorchans) {
	void *ptr = NULL;

	if (xy > 512)
		Error("texture_allocquad: length > 512");

	assert(colorchans <= 4);
	assert(colorchans > 0);
	assert(xy > 0);

	ptr = calloc(xy * xy, colorchans * sizeof(GLubyte));
	if (!ptr)
		Error("Could not allocate memory");
	return ptr;
}


GLubyte*
texture_render_pic_trans(pic_t *texture, const unsigned int space) {
	unsigned int i, l, m;
	byte *src, *dest;

	src = &texture->data;

	assert ( (unsigned int) texture->width*4 <= space);
	assert ( texture->height <= space);

	dest = texture_allocquad(space, 4); /* 3 color channels */
	for (m = 0; m < 4; m++) {
		for (i = 0; i < texture->height; i++) {
			for (l = 0; l < texture->width; l++) {
				extern rtgl_pal rtgl_palette[256];
				unsigned short pal_color = *src++;
				GLubyte *ptr = dest+((l*4+m)*space*sizeof(GLubyte)*4)+(i*sizeof(GLubyte)*4);

				if (pal_color != 255) {
					*ptr++ = rtgl_palette[pal_color].r;
					*ptr++ = rtgl_palette[pal_color].g;
					*ptr++ = rtgl_palette[pal_color].b;
					*ptr = 255;
				}
			}
		}
	}

	return dest;
}


GLubyte*
texture_render_pic(pic_t *texture, const unsigned int space)
{
	unsigned int i, l, m;
	byte *src, *dest;

	src = &texture->data;

	assert ( (unsigned int) texture->width*4 <= space);
	assert ( texture->height <= space);

	dest = texture_allocquad(space, 3); /* 3 color channels */
	for (m = 0; m < 4; m++) {
		for (i = 0; i < texture->height; i++) {
			for (l = 0; l < texture->width; l++) {
				extern rtgl_pal rtgl_palette[256];
				unsigned short pal_color = *src++;
				GLubyte *ptr = dest+((l*4+m)*space*sizeof(GLubyte)*3)+(i*sizeof(GLubyte)*3);

				*ptr++ = rtgl_palette[pal_color].r;
				*ptr++ = rtgl_palette[pal_color].g;
				*ptr = rtgl_palette[pal_color].b;
			}
		}
	}

	return dest;
}


GLubyte*
texture_render_lpic(lpic_t *texture) {
	int i, l;
	GLubyte *dest = texture_allocquad(512, 3);     //FIXME Only use 256x256 max

	//generate 512x512 RGB image (use palette)
	for ( i = 0; i < 512; i++) {
		for (l=0; l < 512; l++) {
			if (i < texture->width && l < texture->height) {
				extern rtgl_pal rtgl_palette[256];
				unsigned short pal_color = *(&texture->data + l + i*texture->height);
				//access x[a][b][c] -> *(x+(a*512*3 + 3*b + c))
				GLubyte *ptr = dest+(i*512*sizeof(GLubyte)*3)+(l*sizeof(GLubyte)*3);

				*ptr++ = rtgl_palette[pal_color].r;
				*ptr++ = rtgl_palette[pal_color].g;
				*ptr = rtgl_palette[pal_color].b;
			}
		}
	}

	return dest;
}


GLubyte*
texture_render_patch (patch_t *texture, const unsigned int space)
{
	extern rtgl_pal rtgl_palette[256];
	int i, l;
	GLubyte *dest = texture_allocquad(space, 4);

	assert (texture->leftoffset <= 0);
	assert (texture->topoffset <= 0);
	assert (space > 0);
	assert (texture->width - texture->leftoffset <= (int) space);
	assert (texture->height - texture->topoffset <= (int) space);

	for ( i = 0; i < texture->width; i++) {
		byte*	addr = (byte *) (texture->collumnofs[i] + (byte*) texture);
		unsigned short offset;

		while ( (offset = *addr) != 255) {
			unsigned short length = *(addr+1);
			GLubyte *ptr = dest+((i-texture->leftoffset)*space*sizeof(GLubyte)*4)+((-texture->topoffset+offset)*sizeof(GLubyte)*4);

			for (l = 0; l < length; l++) {
				unsigned short pal_color = *(addr+2+l);
				*ptr++ = rtgl_palette[pal_color].r;
				*ptr++ = rtgl_palette[pal_color].g;
				*ptr++ = rtgl_palette[pal_color].b;
				*ptr++ = 255;
			}
			addr += length+2;
		}
	}

	return dest;
}

GLubyte*
texture_render_imenu_item (patch_t *texture, const int space) {
	extern rtgl_pal rtgl_palette[256];
	int i, l;
	extern byte* intensitytable;
	GLubyte *dest = texture_allocquad(space, 4);

	assert (texture->leftoffset <= 0);
	assert (texture->topoffset <= 0);
	assert (space > 0);
	assert (texture->width - texture->leftoffset <= space);
	assert (texture->height - texture->topoffset <= space);

	for ( i = 0; i < texture->width; i++) {
		byte*	addr = (byte *) (texture->collumnofs[i] + (byte*) texture);
		unsigned short offset;

		while ( (offset = *addr) != 255) {
			unsigned short length = *(addr+1);
			GLubyte *ptr = dest+((i-texture->leftoffset)*space*sizeof(GLubyte)*4)+((-texture->topoffset+offset)*sizeof(GLubyte)*4);

			for (l = 0; l < length; l++) {
//            *(buf+d)=*(intensitytable+((*(src+s))<<8)+color);
				unsigned short pal_color = *(intensitytable + (*(addr+2+l)<<8) + 1);
				*ptr++ = rtgl_palette[pal_color].r;
				*ptr++ = rtgl_palette[pal_color].g;
				*ptr++ = rtgl_palette[pal_color].b;
				*ptr++ = 255;
			}
			addr += length+2;
		}
	}

	return dest;
}

GLubyte*
texture_render_transpatch(transpatch_t *texture, const int space, const int shift)	//shift is a workaround!
{
	int i;
	GLubyte *fake = texture_allocquad(space, 4);

	assert ( texture->topoffset <= 0);
	assert ( texture->leftoffset <= 0);
	assert ( texture->width - texture->leftoffset <= space);
	assert ( texture->height - texture->topoffset <= space);

	for (i = 0; i < texture->width; i++) {
		unsigned short offset;
		byte * addr = (byte *) (texture->collumnofs[shift+i] + (byte*) texture);
		GLubyte *ptr = fake+(i-texture->leftoffset)*space*sizeof(GLubyte)*4+(-texture->topoffset)*sizeof(GLubyte)*4;
		GLubyte *ptr_org = ptr;

		while ( (offset = *addr++) != 255) {
			unsigned short length = *addr++;
			ptr = ptr_org+offset*sizeof(GLubyte)*4;

			if (*addr == 254) {	//tranparent line
				while ( length-- ) {
					*ptr++ = 0;
					*ptr++ = 0;
					*ptr++ = 0;
					*ptr++ = 255 - texture->translevel * 4;
				}
				addr++;
			}
			else {
				while ( length-- ) {
					unsigned short pal_color = *addr++;
					extern rtgl_pal rtgl_palette[256];

					*ptr++ = rtgl_palette[pal_color].r;
					*ptr++ = rtgl_palette[pal_color].g;
					*ptr++ = rtgl_palette[pal_color].b;
					*ptr++ = 255;
				}
			}
		}
	}

	return fake;
}


GLubyte*
texture_render_sky(byte *texture) {
	int i, l;
	extern rtgl_pal rtgl_palette[256];
	GLubyte *fake = texture_allocquad(256, 3);

	rtglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	rtglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	for (i = 0; i < 256; i++) {
		for (l = 0; l < 256; l++) {
			GLubyte *ptr = fake+3*(l*256 + i);
			unsigned short pal_color;
                        if (l < 200) // fill entire texture to avoid visibility of dark edges
                            pal_color = *(texture+i*200 + l);
                        else
			    pal_color = *(texture+i*200 + 199);

			*ptr++ = rtgl_palette[pal_color].r;
			*ptr++ = rtgl_palette[pal_color].g;
			*ptr = rtgl_palette[pal_color].b;
		}
	}

	return fake;
}


GLubyte*
texture_render_colored_patch (patch_t *texture, const unsigned int color) {
	extern rtgl_pal rtgl_palette[256];
	int i, l;
	byte* shadingtable = playermaps[color] + (1 << 12);
	GLubyte *dest = texture_allocquad(128, 4);

	assert (texture->leftoffset <= 0);
	assert (texture->topoffset <= 0);

	for ( i = 0; i < texture->width; i++) {
		byte*	addr = (byte *) (texture->collumnofs[i] + (byte*) texture);
		unsigned short offset;

		while ( (offset = *addr) != 255) {
			unsigned short length = *(addr+1);

			for (l = 0; l < length; l++) {
				unsigned short pal_color = *(addr+2+l);
				GLubyte *ptr = dest+((i-texture->leftoffset)*128*sizeof(GLubyte)*4)+((l-texture->topoffset+offset)*sizeof(GLubyte)*4);
				if (ptr < dest+128*128*sizeof(GLubyte)*4) {
					*ptr++ = rtgl_palette[shadingtable[pal_color]].r;
					*ptr++ = rtgl_palette[shadingtable[pal_color]].g;
					*ptr++ = rtgl_palette[shadingtable[pal_color]].b;
					*ptr = 255;
				}
			}
			addr += length+2;
		}
	}

	return dest;
}

GLubyte*
texture_render_font (font_t * font, const int space) {
	extern rtgl_pal rtgl_palette[256];
	int i, l, c;
	GLubyte *dest = texture_allocquad(16*space, 4);	//16 * 16 chars with space^2
	assert (font->height <= space);

	for ( c = 0; c < 256; c++ ) {
		byte*	addr = ( font->charofs[c]) + (byte*) font;
		if ( font->width[c] <= space )	/* don't render large characters (width = 71) */
			for (i = 0; i < font->width[c]; i++) {
				GLubyte* ptr = dest + ((c % space) * space + (c / space) * space * (16 * space) + i) * sizeof(GLubyte) * 4;
					for (l = 0; l < font->height; l++, addr++) {
						if (*addr != 0) {
							*ptr = rtgl_palette[*addr].r;
							*(ptr+1) = rtgl_palette[*addr].g;
							*(ptr+2) = rtgl_palette[*addr].b;
							*(ptr+3) = 255;
						}
						else
							*(ptr+3) = 0;
						ptr += (16 * space) * sizeof(GLubyte) * 4;
					}
			}
	}

	return dest;
}

GLubyte*
texture_render_cfont (cfont_t * cfont, const int space) {
	int i, l, c;
	GLubyte *dest = texture_allocquad(16*space, 2);	//16 * 16 chars with 16x16 space

	assert (cfont->height <= space);

	for ( c = 0; c < 256; c++) {
		byte*	addr = ( cfont->charofs[c]) + (byte*) cfont;
		if (cfont->width[c] <= space) {
			for ( i = 0; i < cfont->width[c]; i++) {
				GLubyte* ptr = dest + ((c % space) * space + (c / space) * space * 16*space + i) * sizeof(GLubyte) * 2;
				for (l = 0; l < cfont->height; l++, addr++) {
					if ( *addr != 0xFE ) {
						*ptr = 255 - *addr*32;
						*(ptr+1) = 255;
					}
					ptr += 16*space * sizeof(GLubyte) * 2;
				}
			}
		} else
			printf("RT_GL: ignored character in texture_render_cfont\n");
	}

	return dest;
}


GLubyte*
texture_render_lbm (lbm_t * pic, const int space) {
	GLubyte *dest = texture_allocquad(space, 3);
	GLubyte *ptr = dest;

	int  count;
	byte rept;
	byte lbm_pal[768];
	byte *source = (byte *)&pic->data;
	int height = pic->height;

	assert(pic->height <= space);
	assert(pic->width <= space);

	memcpy(lbm_pal,pic->palette,768);

	VL_NormalizePalette (lbm_pal);
	VL_SetPalette(lbm_pal);

	while (height--) {	/* lines */
		count = 0;

		ptr = dest + (pic->height - height - 1)*3;

		do {		/* columns */
			assert(ptr <= dest+space*space*sizeof(GLubyte)*3);
			rept = *source++;

			if (rept > 0x80) {
				int i;
				rept = (rept^0xff)+2;
				i = rept;

				while (i-- > 0) {
					assert(ptr <= dest+space*space*sizeof(GLubyte)*3);
					*ptr++ = (gammatable[(gammaindex<<6)+lbm_pal[(*source)*3+0]])*4;
					*ptr++ = (gammatable[(gammaindex<<6)+lbm_pal[(*source)*3+1]])*4;
					*ptr = (gammatable[(gammaindex<<6)+lbm_pal[(*source)*3+2]])*4;
					ptr += space*sizeof(GLubyte)*3-2;
				}

				source++;
			   }
			else if (rept < 0x80) {
				int i = ++rept;

				while (i-- > 0) {
					assert(ptr <= dest+space*space*sizeof(GLubyte)*3);
					*ptr++ = (gammatable[(gammaindex<<6)+lbm_pal[(*source)*3+0]])*4;
					*ptr++ = (gammatable[(gammaindex<<6)+lbm_pal[(*source)*3+1]])*4;
					*ptr = (gammatable[(gammaindex<<6)+lbm_pal[(*source)*3+2]])*4;
					ptr += space*sizeof(GLubyte)*3-2;
					source++;
				}
			   }
			else
				rept = 0;               // rept of 0x80 is NOP

			count += rept;

		} while (count < pic->width);
	}

	return dest;
}
