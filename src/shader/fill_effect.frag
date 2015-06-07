#ifndef GL_ES
#define lowp
#define mediump
#endif

uniform lowp float u_opacity;
uniform lowp vec4 u_color;
uniform lowp vec4 u_tone;

const lowp vec3 lumaF = vec3(.299, .587, .114);

void main() {
  lowp vec4 result = u_color;

  if (u_tone != vec4(0.0, 0.0, 0.0, 0.0)) {
    lowp float luma = dot(result.rgb, lumaF);
    result.rgb = mix(result.rgb, vec3(luma), u_tone.w);
    result.rgb += u_tone.rgb;
  }

  if (u_opacity != 1.0) {
    result.a *= u_opacity;
  }

  gl_FragColor = result;
}
