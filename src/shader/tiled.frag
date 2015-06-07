#ifndef GL_ES
#define lowp
#define mediump
#endif

varying mediump vec2 v_tex_coord;

uniform lowp sampler2D u_texture;
uniform mediump vec2 u_tex_base_coord;
uniform mediump vec2 u_tex_range;
uniform lowp float u_opacity;
uniform lowp vec4 u_tone;
uniform lowp vec4 u_color;

const lowp vec3 lumaF = vec3(.299, .587, .114);

void main() {
  lowp vec4 result = texture2D(u_texture, u_tex_base_coord + mod(v_tex_coord, u_tex_range));

  lowp float luma = dot(result.rgb, lumaF);
  result.rgb = mix(result.rgb, vec3(luma), u_tone.w);
  result.rgb += u_tone.rgb;

  result.a *= u_opacity;
  result.rgb = mix(result.rgb, u_color.rgb, u_color.a);

  gl_FragColor = result;
}
