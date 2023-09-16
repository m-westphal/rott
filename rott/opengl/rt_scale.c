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
#include "../rt_def.h"
#include <stdio.h>
#include <string.h>

#include "../rt_util.h"
#include "../rt_draw.h"
#include "../rt_scale.h"
#include "../_rt_scal.h"
#include "../rt_sc_a.h"
#include "../engine.h"
#include "../w_wad.h"
#include "../z_zone.h"
#include "../lumpy.h"
#include "../rt_main.h"
#include "../rt_ted.h"
#include "../rt_vid.h"
#include "../rt_view.h"
#include "../rt_playr.h"

#include <assert.h>
#include "rt_gl.h"

/*
=============================================================================

                               GLOBALS

=============================================================================
*/

int transparentlevel=0;

/*
==========================
=
= SetPlayerLightLevel
=
==========================
*/

void SetPlayerLightLevel (void) {
	int intercept;

	whereami=23;

	/* not required for opengl
	if (MISCVARS->GASON==1) {
		shadingtable=greenmap+(MISCVARS->gasindex<<8);
		return;
	} */

	if (fulllight || fog) {
		shadingtable=colormap+(1<<12);
		return;
	}

	if (player->angle < FINEANGLES/8 || player->angle > 7*FINEANGLES/8)
		intercept=(player->x>>11)&0x1c;
	else if (player->angle < 3*FINEANGLES/8)
		intercept=(player->y>>11)&0x1c;
	else if (player->angle < 5*FINEANGLES/8)
		intercept=(player->x>>11)&0x1c;
	else
		intercept=(player->y>>11)&0x1c;

	if (lightsource) {
		int lv = (((LightSourceAt(player->x>>16,player->y>>16)>>intercept)&0xf)>>1);
		int i = maxshade-(PLAYERHEIGHT>>normalshade)-lv;

		if (i<minshade) i=minshade;
		shadingtable=colormap+(i<<8);
	}
	else {
		int i = maxshade-(PLAYERHEIGHT>>normalshade);

		if (i<minshade) i=minshade;
		shadingtable=colormap+(i<<8);
      }
}


/*
==========================
=
= SetLightLevel
=
==========================
*/

void SetLightLevel (const int height) {
	whereami=24;

	/* not required for opengl
	if (MISCVARS->GASON==1) {
		shadingtable=greenmap+(MISCVARS->gasindex<<8);
		return;
	}*/

	if (fulllight) {
		shadingtable=colormap+(1<<12);
		return;
	}

	if (fog) {
		int i = (height>>normalshade)+minshade;

		if (i>maxshade) i=maxshade;
		shadingtable=colormap+(i<<8);
	}
	else {
		int i=maxshade-(height>>normalshade);

		if (i<minshade) i=minshade;
		shadingtable=colormap+(i<<8);
	}
}

/*
=======================
=
= ScaleWeapon
=
=======================
*/

void ScaleWeapon (const int xoff, const int y, const int shapenum) {
	unsigned int size;

	/* crosshair */
	rtglDisable(GL_TEXTURE_2D);
	rtglDisable(GL_LIGHTING);
	rtglColor3f(1,1,1);
	rtglBegin(GL_LINES);
	rtglVertex3f(160.0f,-100.0f-1.0f, 0);
	rtglVertex3f(160.0f,-100.0f+1.0f, 0);
	rtglEnd();
	rtglBegin(GL_LINES);
	rtglVertex3f(160.0f - 1.3f, -100.0f, 0);
	rtglVertex3f(160.0f + 1.3f, -100.0f, 0);
	rtglEnd();
	if (rtgl_use_lighting) {
		rtglEnable(GL_LIGHTING);
	}
	rtglEnable(GL_TEXTURE_2D);

	patch_t* weapon = (patch_t*) W_CacheLumpNum(shapenum, PU_CACHE, Cvt_patch_t,1);

	if(weapon->width - weapon->leftoffset <= 128 && weapon->height - weapon->topoffset <= 128) {
		size = 128;
	}
	else if(weapon->width - weapon->leftoffset <= 256 && weapon->height - weapon->topoffset <= 256) {
		size = 256; /* not used? */
	}
	else {
		size = 512;
	}

	VGL_Bind_Texture (shapenum, RTGL_TEXTURE_PATCH | RTGL_FILTER_RGBA | size);
	VGL_Draw2DTexture(160 + xoff - weapon->origsize/2, 200 + y - weapon->origsize, size, size, 1.0f, 1.0f);
}


/*
=======================
=
= DrawUnScaledSprite
=
=======================
*/

void DrawUnScaledSprite (const int x, const int y, const int shapenum, const int shade) {
	//FIXME use shade?
	//FIXME different sizes?

	patch_t* p = (patch_t*) W_CacheLumpNum(shapenum,PU_CACHE, Cvt_patch_t, 1);
	VGL_Bind_Texture(shapenum, RTGL_TEXTURE_PATCH | RTGL_FILTER_RGBA | 256);

	VGL_Draw2DTexture(	x - p->origsize/2,
				y + p->topoffset,
				p->width - p->leftoffset, p->height,
				((float) p->width - p->leftoffset) / 256.0f,
				((float) p->height - p->topoffset ) / 256.0f);
}


/*
=======================
=
= DrawScreenSprite
=
=======================
*/

void DrawScreenSprite (const int x, const int y, const int shapenum) {
	whereami=37;
	ScaleWeapon (x-160, y-200, shapenum);
}


/*
=======================
=
= DrawPositionedScaledSprite
=
=======================
*/

void DrawPositionedScaledSprite (const int x, const int y, const int shapenum, const int height, int type) {
	assert(height != 0);

	/* transpatch_t/patch_t workaround */
	if ( type > 128) {
		type -= 128;

		assert (type == 64 || type == 128);

		patch_t *p = (patch_t*) W_CacheLumpNum(shapenum,PU_CACHE, Cvt_patch_t, 1);
		VGL_Bind_Texture(shapenum, RTGL_TEXTURE_PATCH | RTGL_FILTER_RGBA | RTGL_GENERATE_MIPMAPS | type);
		VGL_Draw2DTexture(	x + ((p->leftoffset - p->width/2) * height) / (p->width - p->leftoffset),
					y - height/2,
					height, height,
					((float) p->width - p->leftoffset) / ((float) type),
					((float) p->height - p->topoffset) / ((float) type)
				);
	}
	else {
		transpatch_t *tp = (transpatch_t*) W_CacheLumpNum(shapenum,PU_CACHE, Cvt_transpatch_t, 1);

		assert (type == 64 || type == 128);

		VGL_Bind_Texture(shapenum, RTGL_TEXTURE_TRANSPATCH | RTGL_FILTER_RGBA | RTGL_GENERATE_MIPMAPS | type);
		VGL_Draw2DTexture(x-height/2, y-height/2, height, height, ((float) tp->width) / ((float) type), ((float) tp->height) / ((float) type));
	}
}

/*
=================
=
= DrawScreenSizedSprite
=
=================
*/

void DrawScreenSizedSprite (const int lump) {
	/* probably only works for gasmask */


	VGL_Bind_Texture ( lump, RTGL_TEXTURE_PATCH | 512);	//real size is 320x200

	/* scale for non 4/3 resolutions */
	assert(rtgl_screen_width != 0 && rtgl_screen_height != 0);
	const int ar_correction = 120.0f * ( (float) rtgl_screen_width/ (float) rtgl_screen_height - rtgl_hud_aspectratio);

	VGL_Draw2DTexture(-ar_correction,-60,320+2*ar_correction,260, 319.0f/512.0f, 259.0f / 512.0f);
}

//******************************************************************************
//
// DrawNormalSprite
//
//******************************************************************************

void DrawNormalSprite (const int x, const int y, const int shapenum) {
	whereami = 41;

	patch_t* p = (patch_t*) W_CacheLumpNum ( shapenum, PU_CACHE, Cvt_patch_t, 1);
//	printf("rt_scale.c: DrawNormalSprite:\n Width:%d\n Height:%d\n Origsize:%d\n leftoffset:%d\n topoffset:%d\n",
//			p->width,
//			p->height,
//			p->origsize,
//			p->leftoffset,
//			p->topoffset);
/*
   if (((x-p->leftoffset)<0) || ((x-p->leftoffset+p->width)>320))
#      Error ("DrawNormalSprite: x is out of range x=%d\n",x-p->leftoffset+p->width);
   if (((y-p->topoffset)<0) || ((y-p->topoffset+p->height)>200))
      Error ("DrawNormalSprite: y is out of range y=%d\n",y-p->topoffset+p->height);
*/

	VGL_Bind_Texture( shapenum , RTGL_TEXTURE_PATCH | 512);
//	VGL_Draw2DTexture(x - p->leftoffset, y - p->topoffset, p->width, p->height, ((float) (p->width-p->leftoffset)) / 512.0f, ((float) (p->height-p->topoffset)) / 512.0f);
	assert (p->width > 0);
	assert (p->origsize > 0);
	float scale = ((float) p->width) / (float) p->origsize;
	VGL_Draw2DTexture( ((float) x)*scale - (float) p->leftoffset, y - p->topoffset, p->origsize, p->height, ((float) (p->width-p->leftoffset)) / 512.0f, ((float) (p->height-p->topoffset)) / 512.0f);
}
