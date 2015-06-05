#ifdef GL_ES
#version 100
#endif

attribute vec2 a_position;
varying vec2 v_tex_coord;

void main() {
  v_tex_coord = (a_position + 1.0) * 0.5;
  gl_Position = vec4(a_position, 0.0, 1.0);
}
