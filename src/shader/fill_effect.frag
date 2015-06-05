#ifdef GL_ES
#version 100
precision mediump float;
#endif

uniform float u_opacity;
uniform vec4 u_color;
uniform vec4 u_tone;

void main() {
  vec4 result = u_color;
  result.a *= u_opacity;
  gl_FragColor = result;
}
