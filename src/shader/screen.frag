#ifdef GL_ES
#version 100
precision mediump float;
#endif

varying vec2 v_tex_coord;
uniform sampler2D u_texture;

void main() {
  gl_FragColor = texture2D(u_texture, v_tex_coord);
}
