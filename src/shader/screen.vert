#ifndef GL_ES
#define lowp
#define mediump
#endif

attribute vec2 a_position;
varying mediump vec2 v_tex_coord;

void main() {
  v_tex_coord = (a_position + 1.0) * 0.5;
  gl_Position = vec4(a_position, 0.0, 1.0);
}
