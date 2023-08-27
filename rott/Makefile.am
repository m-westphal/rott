AUTOMAKE_OPTIONS = subdir-objects
SUBDIRS = audiolib

rott@SUFFIX@_CFLAGS = @SDL_CFLAGS@ @SDL_mixer_CFLAGS@
if BUILD_RT_OPENGL
rott@SUFFIX@_CFLAGS += -DRT_OPENGL
endif

bin_PROGRAMS = rott@SUFFIX@
rott@SUFFIX@_SOURCES = \
	byteordr.c \
	cin_actr.c \
	cin_efct.c \
	cin_evnt.c \
	cin_glob.c \
	cin_main.c \
	cin_util.c \
	dosutil.c \
	dukemusc.c \
	isr.c \
	m_misc2.c \
	modexlib.c \
	rt_actor.c \
	rt_battl.c \
	rt_cfg.c \
	rt_com.c \
	rt_crc.c \
	rt_datadir.c \
	rt_debug.c \
	rt_dmand.c \
	rt_door.c \
	rt_draw.c \
	rt_err.c \
	rt_floor.c \
	rt_game.c \
	rt_in.c \
	rt_main.c \
	rt_map.c \
	rt_menu.c \
	rt_msg.c \
	rt_net.c \
	rt_playr.c \
	rt_rand.c \
	rt_sound.c \
	rt_sqrt.c \
	rt_stat.c \
	rt_state.c \
	rt_str.c \
	rt_ted.c \
	rt_util.c \
	rt_vid.c \
	rt_view.c \
	scriplib.c \
	watcom.c \
	winrott.c \
	w_wad.c \
	z_zone.c

rott@SUFFIX@_LDADD = audiolib/libaudiolib.a @SDL_LIBS@ @SDL_mixer_LIBS@

if BUILD_RT_OPENGL
rott@SUFFIX@_SOURCES += opengl/engine.c opengl/rt_build.c opengl/rt_gl.c opengl/rt_gl_hash.c opengl/rt_gl_init.c opengl/rt_gl_sdl_video_init.c opengl/rt_gl_shader.c opengl/rt_gl_texture.c opengl/rt_scale.c
rott@SUFFIX@_LDADD += -lm -lGLU
else
rott@SUFFIX@_SOURCES += engine.c rt_build.c rt_scale.c
endif
