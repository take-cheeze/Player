#ifndef GL_ES
#define lowp
#define mediump
#endif

varying mediump vec2 v_tex_coord;
uniform lowp sampler2D u_texture;

void main() {
  gl_FragColor = texture2D(u_texture, v_tex_coord);
}
