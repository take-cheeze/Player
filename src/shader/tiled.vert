attribute vec2 a_position;
attribute vec2 a_tex_coord;

uniform mat4 u_proj_mat;
uniform mat4 u_model_mat;
uniform mat4 u_tex_mat;

varying vec2 v_tex_coord;

void main() {
  v_tex_coord = (u_tex_mat * vec4(a_tex_coord, 0.0, 1.0)).xy;
  gl_Position = u_proj_mat * u_model_mat * vec4(a_position, 0.0, 1.0);
}
