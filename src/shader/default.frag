#ifndef GL_ES
#define lowp
#define mediump
#endif

uniform lowp sampler2D u_texture;
uniform lowp vec4 u_tone;
uniform lowp vec4 u_color;
uniform lowp float u_opacity;
uniform lowp float u_bush_opacity;
uniform mediump float u_bush_depth;

varying mediump vec2 v_tex_coord;

const vec3 lumaF = vec3(.299, .587, .114);

void main() {
  lowp vec4 result = texture2D(u_texture, v_tex_coord);

  lowp float luma = dot(result.rgb, lumaF);
  result.rgb = mix(result.rgb, vec3(luma), u_tone.w);
  result.rgb += u_tone.rgb;

  result.a *= u_opacity;
  result.rgb = mix(result.rgb, u_color.rgb, u_color.a);

  lowp float under_bush = float(v_tex_coord.y <= u_bush_depth);
  result.a *= clamp(u_bush_opacity + under_bush, 0.0, 1.0);

  gl_FragColor = result;
}
