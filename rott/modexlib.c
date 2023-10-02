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

#include <stdarg.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include <stdlib.h>
#include <sys/stat.h>
#include "modexlib.h"
#include "rt_util.h"
#include "rt_net.h" // for GamePaused
#include "myprint.h"

#ifdef RT_OPENGL
#include <assert.h>
#include "opengl/rt_gl_init.h"
#include "opengl/rt_gl.h"
#endif

static void StretchMemPicture ();
// GLOBAL VARIABLES

boolean StretchScreen=0;//bn�++
extern boolean iG_aimCross;
extern boolean sdl_fullscreen;
extern int iG_X_center;
extern int iG_Y_center;
byte  *iG_buf_center;
  
int    linewidth;
//int    ylookup[MAXSCREENHEIGHT];
int    ylookup[600];//just set to max res
byte  *page1start;
byte  *page2start;
byte  *page3start;
int    screensize;
byte  *bufferofs;
byte  *displayofs;
boolean graphicsmode=false;
byte  *bufofsTopLimit;
byte  *bufofsBottomLimit;

void DrawCenterAim ();

#include "SDL.h"

#ifndef STUB_FUNCTION

/* rt_def.h isn't included, so I just put this here... */
#ifndef STUB_FUNCTION
#define STUB_FUNCTION fprintf(stderr,"STUB: %s at " __FILE__ ", line %d, thread %d\n",__FUNCTION__,__LINE__,getpid())
#endif

#endif

/*
====================
=
= GraphicsMode
=
====================
*/
#ifndef RT_OPENGL
static SDL_Surface *sdl_surface = NULL;
static SDL_Surface *unstretch_sdl_surface = NULL;
#endif

static SDL_Window *screen;
#ifndef RT_OPENGL
static SDL_Renderer *renderer;
static SDL_Surface *argbbuffer;
static SDL_Texture *texture;
static SDL_Rect blit_rect = {0};

SDL_Surface *VL_GetVideoSurface (void)
{
	return sdl_surface;
}
#endif

int VL_SaveBMP (const char *file)
{
#ifdef RT_OPENGL
STUB_FUNCTION;
#else
    return SDL_SaveBMP(sdl_surface, file);
#endif
}

void SetShowCursor(int show)
{
	SDL_SetRelativeMouseMode(!show);
	SDL_GetRelativeMouseState(NULL, NULL);
}

#ifdef RT_OPENGL
void set_rt_gl_has_multisample() {
        /* check for multisample support */
        for (rtgl_has_multisample = 16; rtgl_has_multisample > 1; rtgl_has_multisample /= 2) {
                if (SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1) == -1) {
                    printf("failed to set SDL_GL_MULTISAMPLEBUFFERS\n");
                    rtgl_has_multisample = 0;
                    return;
                }

                if (SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, rtgl_has_multisample) != -1) {
                    printf("Set SDL_GL_MULTISAMPLESAMPLES to %d\n", rtgl_has_multisample);
                    return;
                }
        }
}
#endif

void GraphicsMode ( void )
{
	uint32_t flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
	uint32_t pixel_format;

	unsigned int rmask, gmask, bmask, amask;
	int bpp;

#ifdef RT_OPENGL
	flags |= SDL_WINDOW_OPENGL;
#endif

	if (SDL_InitSubSystem (SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
	{
		Error ("Could not initialize SDL\n");
	}

	if (sdl_fullscreen)
	{
		flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
	}

#ifdef RT_OPENGL
	printf("rtgl_screen_* %d / %d\n", rtgl_screen_width, rtgl_screen_height);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	set_rt_gl_has_multisample();

	screen = SDL_CreateWindow(NULL,
	                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
	                          rtgl_screen_width, rtgl_screen_height,
	                          flags);
	SDL_SetWindowMinimumSize(screen, rtgl_screen_width, rtgl_screen_height);
#else
	screen = SDL_CreateWindow(NULL,
	                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
	                          iGLOBAL_SCREENWIDTH, iGLOBAL_SCREENHEIGHT,
	                          flags);
	SDL_SetWindowMinimumSize(screen, iGLOBAL_SCREENWIDTH, iGLOBAL_SCREENHEIGHT);
#endif

	SDL_SetWindowTitle(screen, PACKAGE_STRING);

#ifndef RT_OPENGL
	renderer = SDL_CreateRenderer(screen, -1, SDL_RENDERER_PRESENTVSYNC);
	if (!renderer)
	{
		renderer = SDL_CreateRenderer(screen, -1, SDL_RENDERER_SOFTWARE);
	}
	SDL_RenderSetLogicalSize(renderer, 640, 480);

	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);
	SDL_RenderPresent(renderer);

	sdl_surface = SDL_CreateRGBSurface(0,
	                                   iGLOBAL_SCREENWIDTH, iGLOBAL_SCREENHEIGHT, 8,
	                                   0, 0, 0, 0);
	SDL_FillRect(sdl_surface, NULL, 0);

	pixel_format = SDL_GetWindowPixelFormat(screen);
	SDL_PixelFormatEnumToMasks(pixel_format, &bpp,
	                           &rmask, &gmask, &bmask, &amask);
	argbbuffer = SDL_CreateRGBSurface(0,
	                                  iGLOBAL_SCREENWIDTH, iGLOBAL_SCREENHEIGHT, bpp,
	                                  rmask, gmask, bmask, amask);

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
	texture = SDL_CreateTexture(renderer,
	                            pixel_format,
	                            SDL_TEXTUREACCESS_STREAMING,
	                            iGLOBAL_SCREENWIDTH, iGLOBAL_SCREENHEIGHT);

	blit_rect.w = iGLOBAL_SCREENWIDTH;
	blit_rect.h = iGLOBAL_SCREENHEIGHT;
#else
	printf("RT_GL: OpenGL start\n");

	SDL_GL_CreateContext(screen);

	if (SDL_GL_LoadLibrary(NULL) != 0) {
		puts(SDL_GetError());
		Error("Failed to load GL library\n");
	}

	if( ! rtgl_SupportedCard() ) {
		puts(SDL_GetError());
		Error ("RT_GL: failed to initialize\n");
	}

        if(rtgl_has_multisample != 0)
                rtglEnable(GL_MULTISAMPLE_ARB);

        /* widescreen hud */
        rtgl_hud_aspectratio = (float) rtgl_screen_width / (float) rtgl_screen_height;
        if (rtgl_hud_aspectratio > 4.0f/3.0f)
                rtgl_hud_aspectratio = 4.0f/3.0f;

        rtglShadeModel( GL_SMOOTH );
        rtglClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
        rtglClearDepth( 128.0f );
        rtglEnable( GL_NORMALIZE );
        rtglEnable( GL_TEXTURE_2D );
        rtglEnable( GL_CULL_FACE );
        rtglBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        rtglDepthFunc( GL_LEQUAL );
        rtglAlphaFunc( GL_GREATER, 0.001f);
        rtglHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST );
        rtglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        rtglLightModeli( GL_LIGHT_MODEL_TWO_SIDE, 1);   //Backside = normal * -1 of frontside
        rtglViewport( 0, 0, (GLsizei)rtgl_screen_width, (GLsizei)rtgl_screen_height );
        VGL_InitHash();
        VGL_Setup2DMode ();
        rtglClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

        printf("RT_GL: OpenGL initialized\n");
#endif

	SetShowCursor(!sdl_fullscreen);
}

void ToggleFullScreen (void)
{
	unsigned int flags = 0;

	sdl_fullscreen = !sdl_fullscreen;

	if (sdl_fullscreen)
	{
		flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
	}

	SDL_SetWindowFullscreen(screen, flags);
	SetShowCursor(!sdl_fullscreen);
}

/*
====================
=
= SetTextMode
=
====================
*/
void SetTextMode ( void )
{
	if (SDL_WasInit(SDL_INIT_VIDEO) == SDL_INIT_VIDEO) {
#ifndef RT_OPENGL
		if (sdl_surface != NULL) {
			SDL_FreeSurface(sdl_surface);
	
			sdl_surface = NULL;
		}
#else
		VGL_DestroyHash();
#endif
		
		SDL_QuitSubSystem (SDL_INIT_VIDEO);
	}
}

/*
====================
=
= TurnOffTextCursor
=
====================
*/
void TurnOffTextCursor ( void )
{
}

/*
====================
=
= WaitVBL
=
====================
*/
void WaitVBL( void )
{
	SDL_Delay (16667/1000);
}

/*
=======================
=
= VL_SetVGAPlaneMode
=
=======================
*/

void VL_SetVGAPlaneMode ( void )
{
#ifdef RT_OPENGL
    GraphicsMode();
#else
   int i,offset;

    GraphicsMode();

//
// set up lookup tables
//
//bna--   linewidth = 320;
   linewidth = iGLOBAL_SCREENWIDTH;

   offset = 0;

   for (i=0;i<iGLOBAL_SCREENHEIGHT;i++)
      {
      ylookup[i]=offset;
      offset += linewidth;
      }

//    screensize=MAXSCREENHEIGHT*MAXSCREENWIDTH;
    screensize=iGLOBAL_SCREENHEIGHT*iGLOBAL_SCREENWIDTH;



    page1start=sdl_surface->pixels;
    page2start=sdl_surface->pixels;
    page3start=sdl_surface->pixels;
    displayofs = page1start;
    bufferofs = page2start;

	iG_X_center = iGLOBAL_SCREENWIDTH / 2;
	iG_Y_center = (iGLOBAL_SCREENHEIGHT / 2)+10 ;//+10 = move aim down a bit

	iG_buf_center = bufferofs + (screensize/2);//(iG_Y_center*iGLOBAL_SCREENWIDTH);//+iG_X_center;

	bufofsTopLimit =  bufferofs + screensize - iGLOBAL_SCREENWIDTH;
	bufofsBottomLimit = bufferofs + iGLOBAL_SCREENWIDTH;

    // start stretched
    EnableScreenStretch();
    XFlipPage ();
#endif
}

/*
=======================
=
= VL_CopyPlanarPage
=
=======================
*/
void VL_CopyPlanarPage ( byte * src, byte * dest )
{
#ifdef RT_OPENGL
	STUB_FUNCTION;
#else
      memcpy(dest,src,screensize);
#endif
}

/*
=======================
=
= VL_CopyPlanarPageToMemory
=
=======================
*/
void VL_CopyPlanarPageToMemory ( byte * src, byte * dest )
{
#ifdef RT_OPENGL
	STUB_FUNCTION;
#else
      memcpy(dest,src,screensize);
#endif
}

/*
=======================
=
= VL_CopyBufferToAll
=
=======================
*/
void VL_CopyBufferToAll ( byte *buffer )
{
}

/*
=======================
=
= VL_CopyDisplayToHidden
=
=======================
*/
void VL_CopyDisplayToHidden ( void )
{
   VL_CopyBufferToAll ( displayofs );
}

/*
=================
=
= VL_ClearBuffer
=
= Fill the entire video buffer with a given color
=
=================
*/

void VL_ClearBuffer (byte *buf, byte color)
{
#ifdef RT_OPENGL
	extern rtgl_pal rtgl_palette[256];
	rtglPushAttrib( GL_COLOR_BUFFER_BIT );
	rtglClearColor( (GLfloat) rtgl_palette[color].r / 255.0f,
			(GLfloat) rtgl_palette[color].g / 255.0f,
			(GLfloat) rtgl_palette[color].b / 255.0f, 0);
	rtglClear (GL_COLOR_BUFFER_BIT);
	rtglPopAttrib();
#else
  memset((byte *)buf,color,screensize);
#endif
}

/*
=================
=
= VL_ClearVideo
=
= Fill the entire video buffer with a given color
=
=================
*/

void VL_ClearVideo (byte color)
{
#ifdef RT_OPENGL
	VL_ClearBuffer(0, color);
#else
  memset (sdl_surface->pixels, color, iGLOBAL_SCREENWIDTH*iGLOBAL_SCREENHEIGHT);
#endif
}

/*
=================
=
= VL_DePlaneVGA
=
=================
*/

void VL_DePlaneVGA (void)
{
}


/* C version of rt_vh_a.asm */

void VH_UpdateScreen (void)
{ 	

#ifdef RT_OPENGL
	XFlipPage();
#else
	if (StretchScreen){//bna++
		StretchMemPicture ();
	}else{
		DrawCenterAim ();
	}
	SDL_LowerBlit(VL_GetVideoSurface(), &blit_rect, argbbuffer, &blit_rect);
	SDL_UpdateTexture(texture, NULL, argbbuffer->pixels, argbbuffer->pitch);
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);
#endif
}


/*
=================
=
= XFlipPage
=
=================
*/

void XFlipPage ( void )
{
#ifdef RT_OPENGL
	SDL_GL_SwapWindow(screen);
#else

 	if (StretchScreen){//bna++
		StretchMemPicture ();
	}else{
		DrawCenterAim ();
	}
   SDL_LowerBlit(sdl_surface, &blit_rect, argbbuffer, &blit_rect);
   SDL_UpdateTexture(texture, NULL, argbbuffer->pixels, argbbuffer->pitch);
   SDL_RenderClear(renderer);
   SDL_RenderCopy(renderer, texture, NULL, NULL);
   SDL_RenderPresent(renderer);
#endif
}

void EnableScreenStretch(void)
{
#ifndef RT_OPENGL
   if (iGLOBAL_SCREENWIDTH <= 320 || StretchScreen) return;
   
   if (unstretch_sdl_surface == NULL)
   {
      /* should really be just 320x200, but there is code all over the
         places which crashes then */
      unstretch_sdl_surface = SDL_CreateRGBSurface(0,
         iGLOBAL_SCREENWIDTH, iGLOBAL_SCREENHEIGHT, 8, 0, 0, 0, 0);
   }
	
   displayofs = unstretch_sdl_surface->pixels +
	(displayofs - (byte *)sdl_surface->pixels);
   bufferofs  = unstretch_sdl_surface->pixels;
   page1start = unstretch_sdl_surface->pixels;
   page2start = unstretch_sdl_surface->pixels;
   page3start = unstretch_sdl_surface->pixels;
   StretchScreen = 1;	
#endif
}

void DisableScreenStretch(void)
{
#ifndef RT_OPENGL
   if (iGLOBAL_SCREENWIDTH <= 320 || !StretchScreen) return;
	
   displayofs = sdl_surface->pixels +
	(displayofs - (byte *)unstretch_sdl_surface->pixels);
   bufferofs  = sdl_surface->pixels;
   page1start = sdl_surface->pixels;
   page2start = sdl_surface->pixels;
   page3start = sdl_surface->pixels;
   StretchScreen = 0;
#endif
}


// bna section -------------------------------------------
static void StretchMemPicture ()
{
#ifndef RT_OPENGL
  SDL_Rect src;
  SDL_Rect dest;
	
  src.x = 0;
  src.y = 0;
  src.w = 320;
  src.h = 200;
  
  dest.x = 0;
  dest.y = 0;
  dest.w = iGLOBAL_SCREENWIDTH;
  dest.h = iGLOBAL_SCREENHEIGHT;
  SDL_SoftStretch(unstretch_sdl_surface, &src, sdl_surface, &dest);
#endif
}

// bna function added start
extern	boolean ingame;
int		iG_playerTilt;

void DrawCenterAim ()
{
	int x;

	int percenthealth = (locplayerstate->health * 10) / MaxHitpointsForCharacter(locplayerstate);
	int color = percenthealth < 3 ? egacolor[RED] : percenthealth < 4 ? egacolor[YELLOW] : egacolor[GREEN];

	if (iG_aimCross && !GamePaused){
		if (( ingame == true )&&(iGLOBAL_SCREENWIDTH>320)){
			  if ((iG_playerTilt <0 )||(iG_playerTilt >iGLOBAL_SCREENHEIGHT/2)){
					iG_playerTilt = -(2048 - iG_playerTilt);
			  }
			  if (iGLOBAL_SCREENWIDTH == 640){ x = iG_playerTilt;iG_playerTilt=x/2; }
			  iG_buf_center = bufferofs + ((iG_Y_center-iG_playerTilt)*iGLOBAL_SCREENWIDTH);//+iG_X_center;

			  for (x=iG_X_center-10;x<=iG_X_center-4;x++){
				  if ((iG_buf_center+x < bufofsTopLimit)&&(iG_buf_center+x > bufofsBottomLimit)){
					 *(iG_buf_center+x) = color;
				  }
			  }
			  for (x=iG_X_center+4;x<=iG_X_center+10;x++){
				  if ((iG_buf_center+x < bufofsTopLimit)&&(iG_buf_center+x > bufofsBottomLimit)){
					 *(iG_buf_center+x) = color;
				  }
			  }
			  for (x=10;x>=4;x--){
				  if (((iG_buf_center-(x*iGLOBAL_SCREENWIDTH)+iG_X_center) < bufofsTopLimit)&&((iG_buf_center-(x*iGLOBAL_SCREENWIDTH)+iG_X_center) > bufofsBottomLimit)){
					 *(iG_buf_center-(x*iGLOBAL_SCREENWIDTH)+iG_X_center) = color;
				  }
			  }
			  for (x=4;x<=10;x++){
				  if (((iG_buf_center+(x*iGLOBAL_SCREENWIDTH)+iG_X_center) < bufofsTopLimit)&&((iG_buf_center+(x*iGLOBAL_SCREENWIDTH)+iG_X_center) > bufofsBottomLimit)){
					 *(iG_buf_center+(x*iGLOBAL_SCREENWIDTH)+iG_X_center) = color;
				  }
			  }
		}
	}
}
// bna function added end




// bna section -------------------------------------------



