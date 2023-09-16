#include "rt_gl_sdl_video.h"

/* screen resolution */
extern unsigned int rtgl_screen_width;
extern unsigned int rtgl_screen_height;
extern float rtgl_hud_aspectratio;

/* texture formats */
extern GLint	rtgl_rgb;
extern GLint	rtgl_rgba;
extern GLint	rtgl_luminance_alpha;

/* texture compression */
extern int	rtgl_has_compression_rgb;
extern int	rtgl_has_compression_rgba;
extern int	rtgl_has_compression_luminance_alpha;
extern int	rtgl_use_color_table;
extern int	rtgl_use_lighting;
extern int	rtgl_use_multisampling;
extern int	rtgl_use_fog;
extern int	rtgl_use_mipmaps;

/* texture filtering */
extern GLfloat rtgl_max_anisotropy;

extern int	rtgl_has_multisample;
extern int	rtgl_use_multisample;

/* rotation effects */
extern int	rtgl_has_texture_rectangle;


/* shader support */
extern int	rtgl_has_shader;
extern int	rtgl_use_shader;
