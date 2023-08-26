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
// RT_BUILD.C

#include "../rt_def.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef DOS
#include <dos.h>
#include <conio.h>
#endif

#include "../rt_build.h"
#include "../_rt_buil.h"
#include "../rt_scale.h"
#include "../rt_menu.h"
#include "../isr.h"
#include "../rt_util.h"
#include "../lumpy.h"
#include "../z_zone.h"
#include "../w_wad.h"
#include "../rt_view.h"
#include "../rt_vid.h"

#include "../rt_sound.h"
#include "../modexlib.h"
#include "../rt_str.h"

#include <assert.h>
#include "../opengl/rt_gl.h"

byte * intensitytable;

// LOCAL VARIABLES

static char menutitles[2][40];
static int alternatemenubuf=0;
static int titleshade=16;
static int titleshadedir=1;
static int titleyoffset=0;
static char titlestring[40]="\0";
static int readytoflip;
static boolean MenuBufStarted=false;
static boolean BackgroundDrawn=false;

static int StringShade=16;

extern void (*USL_MeasureString)(const char *, int *, int *, font_t *);

static char strbuf[MaxString];

//******************************************************************************
//
// ClearMenuBuf
//
//******************************************************************************

void ClearMenuBuf ( void ) {
	/* reset screen */
	VL_DrawPostPic( W_GetNumForName( "trilogo" ) );
	/* 288 x 158 */
	VGL_Bind_Texture (W_GetNumForName("plane"), RTGL_TEXTURE_MENUPLANE | 512);
	VGL_Draw2DTexture(16, 34, 288, 158, 288.0f / 512.0f, 158.0f/512.0f);
}

//******************************************************************************
//
// ShutdownMenuBuf
//
//******************************************************************************

void ShutdownMenuBuf ( void ) {
	if (MenuBufStarted==false) return;
	MenuBufStarted=false;

	if (loadedgame==false) SetViewSize(viewsize);
}

//******************************************************************************
//
// SetupMenuBuf
//
//******************************************************************************

void SetupMenuBuf ( void ) {
	if (MenuBufStarted==true) return;
	MenuBufStarted=true;

	// No top offsets like in game

	centery=100;
	centeryfrac=centery<<16;

	strcpy(titlestring,menutitles[0]);

	screenofs=0;
	viewwidth=320;
	viewheight=200;

	ClearMenuBuf();
	BackgroundDrawn=true;
}


//******************************************************************************
//
// PositionMenuBuf
//
//******************************************************************************

void PositionMenuBuf(const int angle, const int distance, const boolean drawbackground ) {
	font_t * oldfont;
	int width,height;


	if (MenuBufStarted==false)
		Error("Called PositionMenuBuf without menubuf started\n");
	CalcTics();

	if (BackgroundDrawn==false) {
		ClearMenuBuf();
		BackgroundDrawn=true;
	}

	/* redraw upper screen part */
	VGL_Bind_Texture (W_GetNumForName("trilogo"), RTGL_TEXTURE_LPIC | 512);
	VGL_Draw2DTexture(0, 0, 320, 34, 320.0f / 512.0f, 34.0f/512.0f);

	oldfont=CurrentFont;
	CurrentFont = (font_t *)W_CacheLumpName ("newfnt1", PU_CACHE, Cvt_font_t, 1);
	US_MeasureStr (&width, &height, "%s", titlestring);
	US_ClippedPrint ((320-width)>>1, MENUTITLEY-titleyoffset, titlestring);
	CurrentFont=oldfont;
	titleshade+=titleshadedir;
	if (abs(titleshade-16)>6)
		titleshadedir=-titleshadedir;

	FlipPage();
}

//******************************************************************************
//
// RefreshMenuBuf
//
//******************************************************************************

void RefreshMenuBuf( const int time ) {
	int i;

	if (MenuBufStarted==false)
		Error("Called RefreshMenuBuf without menubuf started\n");

	if (readytoflip) return;

	for (i=0;i<=time;i+=tics) {
		PositionMenuBuf (0,NORMALVIEW,false);
	}
}

//******************************************************************************
//
// SetAlternateMenuBuf
//
//******************************************************************************

void SetAlternateMenuBuf ( void ) {
	if (MenuBufStarted==false)
		Error("Called SetAlternateMenuBuf without menubuf started\n");

	readytoflip=1;
}

//******************************************************************************
//
// SetMenuTitle
//
//******************************************************************************

void SetMenuTitle ( const char * menutitle ) {
	if (MenuBufStarted==false)
		Error("Called SetMenuTitle without menubuf started\n");

	strcpy(menutitles[alternatemenubuf],menutitle);

	if (readytoflip==0)
		strcpy(titlestring,menutitle);
}

//******************************************************************************
//
// DrawMenuBufPicture
//
//******************************************************************************

void DrawMenuBufPicture (int x, int y, const byte * pic, int w, int h) {
	VGL_Bind_DefaultTexture(pic, w, h);
	VGL_Draw2DTexture(x+16, y+34, w, h, ((float) w) / 256.0f, ((float) h) / 256.0f);
	STUB_FUNCTION;
}

//******************************************************************************
//
// DrawMenuBufItem
//
//******************************************************************************

void DrawMenuBufItem (const int x, const int y, const int shapenum) {
	patch_t *p;
	int width, height;
	//FIXME CRASHES malformed patches ?
	//FIXME USE 64?

	p = (patch_t*) W_CacheLumpNum ( shapenum, PU_CACHE, Cvt_patch_t, 1);

	width = p->width - p->leftoffset;
	height = p->height - p->topoffset;

	if (width <= 64 && height <= 64) {
		VGL_Bind_Texture(shapenum, RTGL_TEXTURE_PATCH | 64);
		//x,y relative to menuplane
		VGL_Draw2DTexture(x+16, y+34, 64, 64, 1, 1);
	} else if (width <= 128 && height <= 128) {
		VGL_Bind_Texture(shapenum, RTGL_TEXTURE_PATCH | 128);
		//x,y relative to menuplane
		VGL_Draw2DTexture(x+16, y+34, 128, 128, 1, 1);
	} else if (width <= 256 && height <= 256) {
		VGL_Bind_Texture(shapenum, RTGL_TEXTURE_PATCH | 256);
		//x,y relative to menuplane
		VGL_Draw2DTexture(x+16, y+34, 256, 256, 1, 1);
	} else
		puts("RT_GL: MenuBufItem too large");
}

//******************************************************************************
//
// DrawIMenuBufItem
//
//******************************************************************************

void DrawIMenuBufItem (const int x, const int y, const int shapenum, const int color) {
	extern rtgl_pal rtgl_palette[256];
	patch_t *p = (patch_t*) W_CacheLumpNum (shapenum, PU_CACHE, Cvt_pic_t, 1);

	assert (p->width - p->leftoffset<= 256);
	assert (p->width + p->leftoffset<= 256);

	VGL_Bind_Texture(shapenum, RTGL_TEXTURE_IMENU_ITEM | 256);

	rtglColor3f(	rtgl_palette[color].r / 255.0f,
			rtgl_palette[color].g / 255.0f,
			rtgl_palette[color].b / 255.0f );

	//x,y relative to menuplane
	VGL_Draw2DTexture(x+16, y+34, p->width*4, p->height, (float) p->width*4.0f / 256.0f, (float) p->height / 256.0f);

	rtglColor3f(1,1,1);
}

//******************************************************************************
//
// EraseMenuBufRegion
//
//******************************************************************************

void EraseMenuBufRegion (int x, int y, int width, int height) {
	if (MenuBufStarted==false)
		Error("Called EraseMenuBufRegion without menubuf started\n");

//	288 x 158
	VGL_Bind_Texture (W_GetNumForName("plane"), RTGL_TEXTURE_MENUPLANE | 512);

	//fix pos and acc
	rtglPushMatrix();
	rtglTranslatef(16,-34, 0);
	x -= 1;
	y -= 1;
	width += 2;
	height += 2;

	//sane range
	if (x+width > 288)
		width -= x+width-288;
	if (y+height > 158)
		height -= y+height-158;

	//rtglDisable(GL_TEXTURE_2D);
	float s = 512.0f;

	GLfloat coords[20] = {	((float) y) / s, ((float) x) / s, x, -y, 0,
				((float) height+y)/s, ((float) x) / s, x, -y - height, 0,
				((float) height+y)/s, ((float) width + x) / s, x+width,  -y - height, 0,
				((float) y) / s, ( (float) width + x) / s, x+width, -y, 0
	};

	rtglInterleavedArrays(GL_T2F_V3F, 0, coords);
	rtglDrawArrays(GL_QUADS, 0, 4);
	rtglPopMatrix();
}


//******************************************************************************
//
// DrawTMenuBufPic
//
//******************************************************************************

void DrawTMenuBufPic (const int x, const int y, const int shapenum) {
	DrawMenuBufPic (x, y, shapenum);
}


//******************************************************************************
//
// DrawTMenuBufItem
//
//******************************************************************************

void DrawTMenuBufItem (const int x, const int y, const int shapenum) {
	DrawMenuBufItem (x, y, shapenum);
}

//******************************************************************************
//
// DrawColoredMenuBufItem
//
//******************************************************************************

void DrawColoredMenuBufItem (const int x, const int y, const int shapenum, const int color) {
	//FIXME untested
	rtglColor3f(	rtgl_palette [color].r,
			rtgl_palette [color].g,
			rtgl_palette [color].b
		 );
	DrawMenuBufItem (x, y, shapenum);
	rtglColor3f(1, 1, 1);
}

//******************************************************************************
//
// DrawMenuBufPic
//
//******************************************************************************

void DrawMenuBufPic (const int x, const int y, const int shapenum) {
	pic_t *p = (pic_t*) W_CacheLumpNum (shapenum, PU_CACHE, Cvt_pic_t, 1);

	//FIXME CRASHES malformed patches ?
	//FIXME USE 64?
	VGL_Bind_Texture(shapenum, RTGL_TEXTURE_PIC | 512);	//512x512!

	//x,y relative to menuplane
	VGL_Draw2DTexture(x+16, y+34, p->width*4, p->height, (float) p->width*4.0f / 512.0f, (float) p->height / 512.0f);
}




//******************************************************************************
//
// DrawTMenuBufBox
//
//******************************************************************************


void DrawTMenuBufBox ( int x, int y, int width, int height ) {
	   STUB_FUNCTION;
	   /*
   byte *buffer;
   int   xx;
   int   yy;
   int   pixel;

   if (MenuBufStarted==false)
      Error("Called DrawTMenuBufBox without menubuf started\n");

   shadingtable = colormap + ( 25 << 8 );

   if ( ( x < 0 ) || ( ( x + width ) >= TEXTUREW ) )
      Error ("DrawTMenuBar : x is out of range\n");
   if ( ( y < 0 ) || ( y + height ) >= TEXTUREHEIGHT )
      Error ("DrawTMenuBar : y is out of range\n");

   buffer = ( byte * )menubuf + ( x * TEXTUREHEIGHT ) + y;

   for ( xx = 0; xx < width; xx++ )
      {
      for ( yy = 0; yy < height; yy++ )
         {
         pixel = *( buffer + yy );
         pixel = *( shadingtable + pixel );
         *( buffer + yy ) = pixel;
         }

      buffer += TEXTUREHEIGHT;
      }*/
   }


//******************************************************************************
//
// DrawTMenuBufHLine
//
//******************************************************************************

void DrawTMenuBufHLine (const int x, const int y, const int width, const boolean up) {
	VGL_Bind_Texture (W_GetNumForName("plane"), RTGL_TEXTURE_MENUPLANE | 512);

	//FIXME use up for shading...
	//relative to menuplane
	VGL_Draw2DTexture(x+16, y+34, width, 1, ((float) width) / 512.0f, 1.0f/512.0f);
}

//******************************************************************************
//
// DrawTMenuBufVLine
//
//******************************************************************************

void DrawTMenuBufVLine (const int x, const int y, const int height, const boolean up) {
	VGL_Bind_Texture (W_GetNumForName("plane"), RTGL_TEXTURE_MENUPLANE | 512);

	//FIXME use up for shading...
	//relative to menuplane
	VGL_Draw2DTexture(x+16, y+34, 1, height, 1.0f / 512.0f, ((float) height)/512.0f);
}

//******************************************************************************
//******************************************************************************
//
// STRING ROUTINES
//
//******************************************************************************
//******************************************************************************


//******************************************************************************
//
// DrawMenuBufPropString ()
//
//******************************************************************************

void DrawMenuBufPropString (const int px, const int py, const char *string) {
	int ch;

	VW_DrawClippedString (px+16, py+34, string);

	while ((ch = (unsigned char)*string++)!=0)
		PrintX += CurrentFont->width[ch];
}


//******************************************************************************
//
// DrawMenuBufIString ()
//
//******************************************************************************

void DrawMenuBufIString (const int px, const int py, const char *string, const int color) {
   int   width, ht;
   int   ch;

	if (IFont ==  W_CacheLumpName("sifont",  PU_CACHE, Cvt_cfont_t, 1))
		VGL_Bind_Texture( W_GetNumForName ("sifont"), RTGL_TEXTURE_CFONT | 16);
	else if (IFont ==  W_CacheLumpName("lifont",  PU_CACHE, Cvt_cfont_t, 1))
		VGL_Bind_Texture( W_GetNumForName ("lifont"), RTGL_TEXTURE_CFONT | 16);
	else if (IFont ==  W_CacheLumpName("ifnt",  PU_CACHE, Cvt_cfont_t, 1))
		VGL_Bind_Texture( W_GetNumForName ("ifnt"), RTGL_TEXTURE_CFONT | 16);
	else if (IFont ==  W_CacheLumpName("itnyfont",  PU_CACHE, Cvt_cfont_t, 1))
		VGL_Bind_Texture( W_GetNumForName ("itnyfont"), RTGL_TEXTURE_CFONT | 16);

	if (MenuBufStarted==false)
		Error("Called DrawMenuBufPropString without menubuf started\n");

	assert(color >=0);
	assert(color <=255);

	ht = IFont->height;

	PrintX = px;
	PrintY = py;

	while ((ch = (unsigned char)*string++)!=0) {
		if ( ch == '\x9' ) { /* TAB */
			PrintX   += 4 * 5 - (PrintX-px) % ( 4 * 5 );
			continue;
		}

		ch -= 31;
		width = IFont->width[ ch ];

		float texx = ( (float) (ch % 16) * 16) / 256.0f;
		float texx_f = ( (float) (ch % 16) * 16 + width) / 256.0f;
		float texy = ( (float) (ch / 16) * 16) / 256.0f;
		float texy_f = ( (float) (ch / 16) * 16 + ht) / 256.0f;

		float r = ( (float) rtgl_palette[ intensitytable[ color ] ].r ) / 255.0f;
		float g = ( (float) rtgl_palette[ intensitytable[ color ] ].g ) / 255.0f;
		float b = ( (float) rtgl_palette[ intensitytable[ color ] ].b ) / 255.0f;

		rtglColor3f( r, g, b );

		GLfloat char_triangle_fan[20] = {	texx, texy_f, PrintX+16, -py - ht-34, 0,
							texx_f, texy_f, PrintX+16 + width, -py - ht-34, 0,
							texx_f, texy, PrintX+16 + width, -py-34, 0,
							texx, texy, PrintX+16, -py-34, 0
						};

		rtglInterleavedArrays( GL_T2F_V3F, 0, char_triangle_fan);
		rtglDrawArrays(GL_TRIANGLE_FAN, 0, 4);

		PrintX += width;
	}

	//reset color
	rtglColor3f(1, 1, 1);
}


//******************************************************************************
//
// DrawTMenuBufPropString ()
//
// Draws a string at px, py to bufferofs
//
//******************************************************************************

void DrawTMenuBufPropString (int px, int py, const char *string)
{
	STUB_FUNCTION;
	/*
   byte  pix;
   int   width,height,ht;
   byte  *source, *dest, *origdest;
   int   ch;


   if (MenuBufStarted==false)
      Error("Called DrawTMenuBufPropString without menubuf started\n");

   ht = CurrentFont->height;
   dest = origdest = (byte*)menubuf+(px*TEXTUREHEIGHT)+py;

   shadingtable=colormap+(StringShade<<8);
   while ((ch = (unsigned char)*string++)!=0)
   {
      ch -= 31;
      width = CurrentFont->width[ch];
      source = ((byte *)CurrentFont)+CurrentFont->charofs[ch];
      while (width--)
      {
         height = ht;
         while (height--)
         {
            pix = *source;
            if (pix)
               {
               pix = *dest;
               pix = *(shadingtable+pix);
               *dest = pix;
               }
            source++;
            dest ++;
         }

         PrintX++;
         origdest+=TEXTUREHEIGHT;
         dest = origdest;
      }
   }*/
}


//******************************************************************************
//
// MenuBufCPrintLine() - Prints a string centered on the current line and
//    advances to the next line. Newlines are not supported.
//
//******************************************************************************

void MenuBufCPrintLine (const char *s) {
	int w, h;

	USL_MeasureString (s, &w, &h, CurrentFont);

	if (w > WindowW)
		Error("MenuBufCPrintLine() - String exceeds width");

	PrintX = WindowX + ((WindowW - w) / 2);
	DrawMenuBufPropString (PrintX, PrintY, s);
	PrintY += h;
}

//******************************************************************************
//
// MenuBufCPrint() - Prints a string in the current window. Newlines are
//    supported.
//
//******************************************************************************

void MenuBufCPrint (const char *string) {
	char  c, *se, *s;

	/* !!! FIXME: this is lame. */
	if (strlen(string) >= sizeof (strbuf)) {
		fprintf(stderr, "buffer overflow!\n");
		return;
	}

	/* prevent writing to literal strings... ( MenubufCPrint("feh"); ) */
	strcpy(strbuf, string);
	s = strbuf;

	while (*s) {
		se = s;
		while ((c = *se) && (c != '\n'))
			se++;
		*se = '\0';

		MenuBufCPrintLine(s);

		s = se;
		if (c) {
			*se = c;
			s++;
		}
	}
}

//******************************************************************************
//
// MenuBufPrintLine() - Prints a string on the current line and
//    advances to the next line. Newlines are not supported.
//
//******************************************************************************

void MenuBufPrintLine (const char *s) {
	int w, h;

	USL_MeasureString (s, &w, &h, CurrentFont);

	if (w > WindowW)
		Error("MenuBufCPrintLine() - String exceeds width");

	PrintX = WindowX;
	DrawMenuBufPropString (PrintX, PrintY, s);
	PrintY += h;
}

//******************************************************************************
//
// MenuBufPrint() - Prints a string in the current window. Newlines are
//    supported.
//
//******************************************************************************

void MenuBufPrint (const char *string) {
	char  c, *se, *s;

	strcpy(strbuf, string);
	s = strbuf;
   
	while (*s) {
		se = s;
		while ((c = *se) && (c != '\n'))
			se++;
		*se = '\0';

		MenuBufPrintLine(s);

		s = se;
		if (c) {
			*se = c;
			s++;
		}
	}
}

//******************************************************************************
//
// MenuTBufPrintLine() - Prints a string on the current line and
//    advances to the next line. Newlines are not supported.
//
//******************************************************************************

void MenuTBufPrintLine (const char *s, const int shade) {
	int w, h;

	USL_MeasureString (s, &w, &h, CurrentFont);

	if (w > WindowW)
		Error("MenuBufCPrintLine() - String exceeds width");

	PrintX = WindowX;
	StringShade=shade;
	DrawTMenuBufPropString (PrintX, PrintY, s);
	PrintY += h;
}

//******************************************************************************
//
// FlipMenuBuf
//
//******************************************************************************

void FlipMenuBuf ( void )
{
   int i;
   int dh;
   int h;
   int y;
   int dy;
   int time;
   int flip;

   if (MenuBufStarted==false)
      Error("Called FlipMenuBuf without menubuf started\n");

   if (!readytoflip)
      Error("FlipMenuBuf called without SetAlternateMenuBuf beforehand");
   readytoflip=0;
   if (Menuflipspeed<=5)
      {
      strcpy(titlestring,menutitles[alternatemenubuf]);
      RefreshMenuBuf(0);
      }
   else
      {
      strcpy(titlestring,menutitles[alternatemenubuf^1]);
      time=Menuflipspeed-5;
      dh=(1024<<8)/time;
      h=0;
      dy=((MENUTITLEY*6)<<8)/time;
      y=0;
      flip=0;
      titleyoffset=0;
      for (i=0;i<time;i+=tics)
         {
         PositionMenuBuf(h>>8,NORMALVIEW,true);
         h+=dh*tics;
         y+=dy*tics;
         titleyoffset=y>>8;
         if ((h>=512<<8) && (flip==0))
            {
            MN_PlayMenuSnd (SD_MENUFLIP);
            h=1536<<8;
            dy=-dy;
            strcpy(titlestring,menutitles[alternatemenubuf]);
            flip=1;
            }
         }
      }
   titleyoffset=0;
}



//******************************************************************************
//
// RotatePlane
//
//******************************************************************************

void RotatePlane ( void )
{
   SetupMenuBuf();

   while (!(Keyboard[0x01]))
      {
      RefreshMenuBuf(100);
      SetAlternateMenuBuf();
      ClearMenuBuf();
      DrawMenuBufPic  (0,0,W_GetNumForName("newg11"));
      DrawMenuBufItem (0,0,W_GetNumForName("apogee"));
      FlipMenuBuf();
      EraseMenuBufRegion(30,30,30,30);
      RefreshMenuBuf(100);
      SetAlternateMenuBuf();
      ClearMenuBuf();
      FlipMenuBuf();
      }
   ShutdownMenuBuf();
}
