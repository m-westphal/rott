#include <SDL2/SDL_opengl.h>

void texture_filter_rgba(GLubyte* texture, const unsigned int space);

void* texture_allocquad (size_t xy, size_t colorchans);
GLubyte* texture_render_pic (pic_t *texture, const unsigned int space);
GLubyte* texture_render_pic_trans (pic_t *texture, const unsigned int space);
GLubyte* texture_render_lpic (lpic_t *texture);
GLubyte* texture_render_patch (patch_t *texture, const unsigned int space);
GLubyte* texture_render_colored_patch (patch_t *texture, const unsigned int color);
GLubyte* texture_render_transpatch (transpatch_t *texture, const int, const int);
GLubyte* texture_render_sky (byte *texture);
GLubyte* texture_render_cfont (cfont_t * cfont, const int space);
GLubyte* texture_render_font (font_t * font, const int space);
GLubyte* texture_render_imenu_item (patch_t *texture, const int space);
GLubyte* texture_render_lbm (lbm_t *texture, const int space);
