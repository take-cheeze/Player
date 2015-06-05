attribute vec2 a_position;

uniform mat4 u_proj_mat;
uniform mat4 u_model_mat;

void main() {
  gl_Position = u_proj_mat * u_model_mat * vec4(a_position, 0.0, 1.0);
}
