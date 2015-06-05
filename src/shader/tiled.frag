varying vec2 v_tex_coord;

uniform sampler2D u_texture;
uniform vec2 u_tex_base_coord;
uniform vec2 u_tex_range;
uniform float u_opacity;
uniform vec4 u_tone;
uniform vec4 u_color;

const vec3 lumaF = vec3(.299, .587, .114);

void main() {
  vec4 result = texture2D(u_texture, u_tex_base_coord + mod(v_tex_coord, u_tex_range));

  float luma = dot(result.rgb, lumaF);
  result.rgb = mix(result.rgb, vec3(luma), u_tone.w);
  result.rgb += u_tone.rgb;

  result.a *= u_opacity;
  result.rgb = mix(result.rgb, u_color.rgb, u_color.a);

  gl_FragColor = result;
}
