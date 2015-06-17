#ifndef GL_ES
#define lowp
#define mediump
#endif

attribute vec2 a_position;
attribute vec2 a_tex_coord;

uniform mat4 u_proj_mat;
uniform mat4 u_model_mat;
uniform vec2 u_tex_scale;

varying mediump vec2 v_tex_coord;

void main() {
  v_tex_coord = a_tex_coord * u_tex_scale;
  gl_Position = u_proj_mat * u_model_mat * vec4(a_position, 0.0, 1.0);
}
