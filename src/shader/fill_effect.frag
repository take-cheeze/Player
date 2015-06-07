#ifndef GL_ES
#define lowp
#define mediump
#endif

uniform lowp float u_opacity;
uniform lowp vec4 u_color;
uniform lowp vec4 u_tone;

void main() {
  lowp vec4 result = u_color;
  result.a *= u_opacity;
  gl_FragColor = result;
}
