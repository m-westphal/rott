#include <string.h>
#include "rt_gl_shader.h"
#include "rt_gl_sdl_video.h"

#include <stdio.h>

static const GLchar* vertex_shader =
"varying vec3 light_dir[2];"
"varying vec3 x;"
"varying vec3 y;"
"varying vec3 norm;"
"varying vec3 diffuse[2];"
"varying float fog_factor;"
""
"void normal_surface(out vec3 x, out vec3 y) {"
"	if(gl_Normal.y == 0.0) {"
"		y = vec3(0.0,1.0,0.0);"
"	if(gl_Normal.x == 0.0)"
"			x = vec3(-1.0 * gl_Normal.z,0.0,0.0);"
"		else"
"			x = vec3(0.0,0.0,gl_Normal.x);"
"	}"
"	else {"
"		x = vec3(0.0,0.0,-1.0);"
"		y = vec3(gl_Normal.y,0.0,0.0);"
"	}"
"}"
""
"float specular(in vec3 N, in vec3 V) {"
"	float val = max(0.0, dot(N, normalize(-V)));"
"	return pow(val, gl_FrontMaterial.shininess);"
"}"
""
"void main(void) {"
"	float dist[2];"
""
"	gl_TexCoord[0] = gl_MultiTexCoord0;"
"	vec4 V = gl_ModelViewMatrix * gl_Vertex;"
"	vec3 N = normalize(gl_NormalMatrix * gl_Normal);"
"	light_dir[0] = gl_LightSource[0].position.xyz - V.xyz;"
"	light_dir[1] = gl_LightSource[1].position.xyz - V.xyz;"
"	dist[0] = length(light_dir[0]);"
"	dist[1] = length(light_dir[1]);"
"	light_dir[0] = normalize(light_dir[0]);"
"	light_dir[1] = normalize(light_dir[1]);"
"	float att1 = 1.0 / (gl_LightSource[0].constantAttenuation + "
"			    dist[0] * dist[0] * gl_LightSource[0].quadraticAttenuation);"
"	float att2 = 1.0 / (dist[1] * dist[1] * gl_LightSource[1].quadraticAttenuation);"
"	gl_FrontColor = gl_FrontLightModelProduct.sceneColor + gl_FrontMaterial.ambient * ("
"			att1 * gl_LightSource[0].ambient + "
"			att2 * gl_LightSource[1].ambient)"
"			 + gl_FrontMaterial.specular * specular(N, V.xyz) * ("
"			att1 * gl_LightSource[0].specular + "
"			att2 * gl_LightSource[1].specular);"
"	diffuse[0] = att1 * vec3(gl_LightSource[0].diffuse) * vec3(gl_FrontMaterial.diffuse);"
"	diffuse[1] = att2 * vec3(gl_LightSource[1].diffuse) * vec3(gl_FrontMaterial.diffuse);"
"	norm = gl_Normal;"
"	normal_surface(x, y);"
"	gl_Position = ftransform();"
""
"	gl_FogFragCoord = length(V);"
"	fog_factor = clamp( (gl_Fog.end - gl_FogFragCoord) * gl_Fog.scale, 0.0, 1.0);"
"}";


// NEEDED?:
//"	if(dot(N, L) > 0.0) {"
//"	}"

static const GLchar* fragment_shader =
"uniform sampler2D tex0;"
"varying vec3 light_dir[2];"
"varying vec3 x;"
"varying vec3 y;"
"varying vec3 norm;"
"varying vec3 diffuse[2];"
"varying float fog_factor;"
""
"vec3 tex_normal() {"
"	const float strength = 4.0;"
"	vec4 l = vec4(texture2D(tex0, gl_TexCoord[0].st + vec2(        0, -1.0/64.0)));"
"	vec4 r = vec4(texture2D(tex0, gl_TexCoord[0].st + vec2(        0, +1.0/64.0)));"
"	vec4 u = vec4(texture2D(tex0, gl_TexCoord[0].st + vec2(-1.0/64.0,         0)));"
"	vec4 d = vec4(texture2D(tex0, gl_TexCoord[0].st + vec2(+1.0/64.0,         0)));"
"	return normalize( gl_NormalMatrix * (strength * (length(r) - length(l)) * x + strength * (length(d) - length(u)) * y + norm) );"
"}"
""
"void main(void) {"
"	vec3 d_color[2];"
"	vec3 t_normal = tex_normal();"
"	d_color[0] = dot(t_normal, light_dir[0]) * diffuse[0];"
"	d_color[1] = dot(t_normal, light_dir[1]) * diffuse[1];"
"	vec4 t_color = vec4(texture2D(tex0, gl_TexCoord[0].st));"
"	vec4 f_color = (vec4(d_color[0] + d_color[1], 0.0) + gl_Color) * t_color;"
"	gl_FragColor = mix(gl_Fog.color, f_color, fog_factor);"
"}";

GLuint rtgl_shader_program;

static GLuint _compile_shader(const GLenum type, const GLchar* shader) {
	GLint ok;
	GLuint uid_shader = rtglCreateShader(type);

	if(uid_shader == 0)
		return 0;

	rtglShaderSource(uid_shader, (GLsizei) 1, &shader, NULL);

	puts("RT_GL: compile shader");
	rtglCompileShader(uid_shader);

	rtglGetShaderiv(uid_shader, GL_COMPILE_STATUS, &ok);
	if(ok == GL_FALSE) {
		puts("RT_GL: compile FAILED");
		GLchar log[1024*1024];
		rtglGetShaderInfoLog(uid_shader, 1024*1024, NULL, log);
		puts(log);
		return 0;
	}

	return uid_shader;
}

static GLuint _combine_shader(GLuint a, GLuint b) {
	GLuint uid_program = rtglCreateProgram();
	if(uid_program == 0)
		return 0;

	rtglAttachShader(uid_program, a);
	rtglAttachShader(uid_program, b);
	rtglDeleteShader(a);
	rtglDeleteShader(b);

	rtglLinkProgram(uid_program);

	GLint ok;
	rtglGetProgramiv(uid_program, GL_LINK_STATUS, &ok);
	if(ok == GL_FALSE)
		return 0;

	return uid_program;
}

int rtglUploadTextureShader() {
	printf("RT_GL: begin shader upload\n");

	GLuint fs = _compile_shader(GL_FRAGMENT_SHADER, fragment_shader);
	if(fs == 0)
		return 0;

	printf("RT_GL: fragment shader compiled\n");
	GLuint vs = _compile_shader(GL_VERTEX_SHADER, vertex_shader);
	if(vs == 0)
		return 0;

	printf("RT_GL: vertex shader compiled\n");
	GLuint uid_program = _combine_shader(fs, vs);
	if(uid_program == 0)
		return 0;

	printf("RT_GL: shader uploaded\n");

	rtgl_shader_program = uid_program;
	rtglActiveTexture(GL_TEXTURE0); // DEF VALUE!

	return 1;
}
