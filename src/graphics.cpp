/*
 * This file is part of EasyRPG Player.
 *
 * EasyRPG Player is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * EasyRPG Player is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with EasyRPG Player. If not, see <http://www.gnu.org/licenses/>.
 */

// Headers
#include <algorithm>
#include <sstream>
#include <vector>
#include <map>
#include <iostream>

#include "graphics.h"
#include "bitmap.h"
#include "cache.h"
#include "baseui.h"
#include "drawable.h"
#include "util_macro.h"
#include "player.h"
#include "output.h"

// Drawables
#include "tilemap_layer.h"
#include "window.h"
#include "background.h"
#include "battle_animation.h"
#include "message_overlay.h"
#include "plane.h"
#include "screen.h"
#include "sprite.h"
#include "sprite_timer.h"
#include "weather.h"

#include "main_data.h"
#include "game_system.h"
#include "game_party.h"
#include "game_temp.h"

#include "text.h"

#include <boost/optional.hpp>
#include <boost/utility/string_ref.hpp>
#include <boost/unordered_map.hpp>

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp> // for printing

#ifdef __APPLE__
#  include "TargetConditionals.h"
#  if TARGET_OS_IPHONE && TARGET_IPHONE_SIMULATOR
#    include <OpenGLES/ES2/gl.h>
#  else
#    include <OpenGL/gl3.h>
#  endif
#else
#include <GLES2/gl2.h>
#endif

using boost::optional;

namespace Graphics {
	bool fps_on_screen;

	void UpdateTitle();
	void DrawFrame();
	void DrawOverlay();

	int fps;
	int framerate = DEFAULT_FPS;

	void UpdateTransition();

	BitmapRef frozen_screen, black_screen;
	BitmapRef transition_from, transition_to;
	bool frozen;
	TransitionType transition_type;
	int transition_duration, transition_frame;
	bool screen_erased;

	uint32_t next_fps_time;

	inline bool SortDrawableList(const Drawable* first, const Drawable* second) {
		if (first->GetZ() < second->GetZ()) return true;
		return false;
	}

	struct State {
		State() : zlist_dirty(false) {}
		std::vector<Drawable*> drawable_list;
		bool zlist_dirty;
		void sort() {
			if (not zlist_dirty) { return; }
			std::sort(drawable_list.begin(), drawable_list.end(), SortDrawableList);
			zlist_dirty = false;
		}
	};

void check_gl_error() {
	GLenum const err = glGetError();
	char const* err_str = NULL;
	switch (err) {
	case GL_INVALID_ENUM:
		err_str = "invalid enum"; break;
	case GL_INVALID_FRAMEBUFFER_OPERATION:
		err_str = "invalid framebuffer operation"; break;
	case GL_INVALID_OPERATION:
		err_str = "invalid operation"; break;
	case GL_INVALID_VALUE:
		err_str = "invalid value"; break;
	case GL_OUT_OF_MEMORY:
		err_str = "out of memory"; break;
	default:
		err_str = "unknown error"; break;
	case GL_NO_ERROR: break;
	}
	if (err_str) {
		assert(false);
		Output::Error("OpenGL error: %s", err_str);
	}
}

struct Shader {
	Shader(GLenum t, char const* src) : type(t), source_(src) {}

	~Shader() {
		if (shader_) { released_.push_back(shader_.get()); }
	}

	GLuint get_handle() {
		if (not shader_) {
			shader_ = glCreateShader(type);
			GLchar const* src_list[] = { source_.get().data() };
			GLint const src_len_list[] = { static_cast<GLint>(source_.get().size()) };
			glShaderSource(shader_.get(), 1, src_list, src_len_list);
			glCompileShader(shader_.get());

			GLint info_len;
			glGetShaderiv(shader_.get(), GL_INFO_LOG_LENGTH, &info_len);
			std::vector<GLchar> info(info_len + 1);
			GLsizei info_result_len;
			glGetShaderInfoLog(shader_.get(), info.size(), &info_result_len, &info.front());

			GLint compile_stat;
			glGetShaderiv(shader_.get(), GL_COMPILE_STATUS, &compile_stat);
			if (compile_stat == GL_FALSE) {
				Output::Error("Shader compile error: %s\nSource:\n%s",
					      &info.front(), source_.get().c_str());
			} else if (info_len > 0) {
				Output::Debug("Shader compile succeeded: %s", &info.front());
			}

			source_ = boost::none;
		}
		return shader_.get();
	}

	static void clear_released() {
		std::for_each(released_.begin(), released_.end(), glDeleteShader);
		released_.clear();
	}

	GLenum const type;
    private:
	static std::vector<GLuint> released_;
	optional<GLuint> shader_;
	optional<std::string> source_;
};

struct Program {
	Program(EASYRPG_SHARED_PTR<Shader> vert, EASYRPG_SHARED_PTR<Shader> frag)
			: vertex_(vert), fragment_(frag)
	{
		assert(vert->type == GL_VERTEX_SHADER);
		assert(frag->type == GL_FRAGMENT_SHADER);
	}

	~Program() {
		if (program_) { released_.push_back(program_.get()); }
	}

	void use() {
		glUseProgram(get_handle());
	}

	GLint uniform_location(GLchar const* name) {
		loc_cache_type::const_iterator const i = uni_loc_.find(name);
		if (i != uni_loc_.end()) { return i->second; }

		GLint const res = glGetUniformLocation(get_handle(), name);
		if (res == -1) { Output::Error("Invalid uniform name: %s", name); }
		uni_loc_[name] = res;

		return res;
	}
	GLint attrib_location(GLchar const* name) {
		loc_cache_type::const_iterator const i = attr_loc_.find(name);
		if (i != attr_loc_.end()) { return i->second; }

		GLint const res = glGetAttribLocation(get_handle(), name);
		if (res == -1) { Output::Error("Invalid vertex attribute name: %s", name); }
		glEnableVertexAttribArray(res);
		attr_loc_[name] = res;

		return res;
	}

	GLuint get_handle() {
		if (not program_) {
			program_ = glCreateProgram();
			glAttachShader(program_.get(), vertex_->get_handle());
			glAttachShader(program_.get(), fragment_->get_handle());
			glLinkProgram(program_.get());
			GLint link_stat;
			glGetProgramiv(program_.get(), GL_LINK_STATUS, &link_stat);
			if (link_stat == GL_FALSE) {
				GLint info_len;
				glGetProgramiv(program_.get(), GL_INFO_LOG_LENGTH, &info_len);
				std::vector<GLchar> info(info_len + 1);
				GLsizei info_result_len;
				glGetProgramInfoLog(program_.get(), info.size(), &info_result_len, &info.front());
				Output::Error("Program link error: %s", &info.front());
			}
		}
		return program_.get();
	}

	static void clear_released() {
		std::for_each(released_.begin(), released_.end(), glDeleteProgram);
		released_.clear();
	}

	bool validate() {
		glValidateProgram(get_handle());
		GLint validate_stat;
		glGetProgramiv(program_.get(), GL_VALIDATE_STATUS, &validate_stat);
		if (validate_stat == GL_FALSE) {
			GLint info_len;
			glGetProgramiv(program_.get(), GL_INFO_LOG_LENGTH, &info_len);
			std::vector<GLchar> info(info_len + 1);
			GLsizei info_result_len;
			glGetProgramInfoLog(program_.get(), info.size(), &info_result_len, &info.front());
			Output::Debug("Program validate error: %s", &info.front());
		}
		return validate_stat != GL_FALSE;
	}

    private:
	static std::vector<GLuint> released_;
	optional<GLuint> program_;
	EASYRPG_SHARED_PTR<Shader> vertex_, fragment_;
	typedef boost::unordered_map<std::string, GLint> loc_cache_type;
	loc_cache_type uni_loc_, attr_loc_;
};

std::vector<GLuint> released_buffer_;
template<class T>
struct Buffer {
	Buffer(GLenum t, GLenum u) : dirty_(true), target(t), usage(u) {}

	~Buffer() {
		if (buffer_) { released_buffer_.push_back(buffer_.get()); }
	}

	std::vector<T>& modify() {
		dirty_ = true;
		return data_;
	}

	GLuint get_handle() {
		if (not buffer_) {
			GLuint buf;
			glGenBuffers(1, &buf);
			buffer_ = buf;
		}
		return buffer_.get();
	}

	void bind() {
		glBindBuffer(target, get_handle());
		if (dirty_) {
			glBufferData(target, sizeof(T) * data_.size(), &data_.front(), usage);
			dirty_ = false;
		}
	}

    private:
	optional<GLuint> buffer_;
	std::vector<T> data_;
	bool dirty_;

    public:
	GLenum target, usage;
};

struct Texture2D {
	~Texture2D() {
		if (texture_) { released_.push_back(texture_.get()); }
	}

	GLuint get_handle() {
		if (not texture_) {
			GLuint tex;
			glGenTextures(1, &tex);
			texture_ = tex;
			glBindTexture(GL_TEXTURE_2D, tex);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		}
		return texture_.get();
	}

	void bind() {
		glBindTexture(GL_TEXTURE_2D, get_handle());
	}

	void sync(GLenum format, GLenum type, GLsizei w, GLsizei h, GLvoid const* data) {
		bind();
		glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, type, data);
		width_ = w;
		height_ = h;
	}

	glm::vec2 scale() const {
		return glm::vec2(1.f / width_.get(), 1.f / height_.get());
	}

	static void clear_released() {
		glDeleteTextures(released_.size(), &released_.front());
		released_.clear();
	}

	GLsizei width() const { return width_.get(); }
	GLsizei height() const { return height_.get(); }

    private:
	static std::vector<GLuint> released_;
	optional<GLuint> texture_;
	optional<GLsizei> width_, height_;
};

int real_fps;

EASYRPG_SHARED_PTR<State> state;
std::vector<EASYRPG_SHARED_PTR<State> > stack;
EASYRPG_SHARED_PTR<State> global_state;

Color backcolor_(0, 0, 0, 255);
optional<GLuint> render_fbo;
Texture2D screen_texture;
Rect const screen_target_rect(0, 0, SCREEN_TARGET_WIDTH, SCREEN_TARGET_HEIGHT);
glm::mat4 const screen_target_proj_mat = glm::ortho<float>(0, SCREEN_TARGET_WIDTH, SCREEN_TARGET_HEIGHT, 0);
Buffer<GLfloat> screen_fbo_buffer(GL_ARRAY_BUFFER, GL_STATIC_DRAW);

// Shaders
EASYRPG_SHARED_PTR<Program> screen_fbo_program = EASYRPG_MAKE_SHARED<Program>(
    EASYRPG_MAKE_SHARED<Shader>(GL_VERTEX_SHADER,
#include "./shader/screen.vert.inc"
				),
    EASYRPG_MAKE_SHARED<Shader>(GL_FRAGMENT_SHADER,
#include "./shader/screen.frag.inc"
				));

EASYRPG_SHARED_PTR<Program> tiled_program = EASYRPG_MAKE_SHARED<Program>(
    EASYRPG_MAKE_SHARED<Shader>(
	GL_VERTEX_SHADER,
#include "./shader/tiled.vert.inc"
				),
    EASYRPG_MAKE_SHARED<Shader>(
	GL_FRAGMENT_SHADER,
#include "./shader/tiled.frag.inc"
				));

EASYRPG_SHARED_PTR<Program> fill_effect_program = EASYRPG_MAKE_SHARED<Program>(
    EASYRPG_MAKE_SHARED<Shader>(
	GL_VERTEX_SHADER,
#include "./shader/fill_effect.vert.inc"
				),
    EASYRPG_MAKE_SHARED<Shader>(
	GL_FRAGMENT_SHADER,
#include "./shader/fill_effect.frag.inc"
				));

EASYRPG_SHARED_PTR<Program> sprite_program = EASYRPG_MAKE_SHARED<Program>(
     EASYRPG_MAKE_SHARED<Shader>(GL_VERTEX_SHADER,
#include "./shader/default.vert.inc"
				 ),
     EASYRPG_MAKE_SHARED<Shader>(GL_FRAGMENT_SHADER,
#include "./shader/default.frag.inc"
				 ));

GLuint get_render_fbo() {
	if (not render_fbo) {
		GLuint fbo;
		glGenFramebuffers(1, &fbo);
		render_fbo = fbo;

		screen_texture.sync(
		    GL_RGBA, GL_UNSIGNED_BYTE,
		    screen_target_rect.width, screen_target_rect.height, NULL);

		glBindFramebuffer(GL_FRAMEBUFFER, render_fbo.get());
		glFramebufferTexture2D(
		    GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
		    screen_texture.get_handle(), 0);

		std::vector<GLfloat>& buf = screen_fbo_buffer.modify();
		buf.resize(2 * 4);
		buf[2 * 0 + 0] = -1.f; buf[2 * 0 + 1] = -1.f;
		buf[2 * 1 + 0] =  1.f; buf[2 * 1 + 1] = -1.f;
		buf[2 * 2 + 0] =  1.f; buf[2 * 2 + 1] =  1.f;
		buf[2 * 3 + 0] = -1.f; buf[2 * 3 + 1] =  1.f;
	}
	return render_fbo.get();
}

struct TextureRef {
	EASYRPG_WEAK_PTR<Bitmap> bitmap;
	EASYRPG_SHARED_PTR<Texture2D> texture;
};
typedef boost::unordered_map<Bitmap*, TextureRef> texture_map_type;
texture_map_type tex_map_;

Texture2D& get_texture(BitmapRef const& bmp) {
	texture_map_type::iterator i = tex_map_.find(bmp.get());
	if (i == tex_map_.end()) {
		TextureRef r = { bmp, EASYRPG_MAKE_SHARED<Texture2D>() };
		tex_map_.insert(texture_map_type::value_type(bmp.get(), r));
		r.texture->sync(GL_RGBA, GL_UNSIGNED_BYTE,
				bmp->GetWidth(), bmp->GetHeight(), bmp->GetData());
		return *r.texture;
	}

	assert(not i->second.bitmap.expired());
	assert(i->second.texture);

	if (i->second.bitmap.lock()->GetDirty()) {
		i->second.texture->sync(
		    GL_RGBA, GL_UNSIGNED_BYTE,
		    bmp->GetWidth(), bmp->GetHeight(), bmp->GetData());
	}
	return *i->second.texture;
}

void prepare_rendering() {
	DisplayUi->MakeGLContextCurrent();

	static bool print_info = false;
	if (not print_info) {
		print_info = true;
		Output::Debug("OpenGL Vendor: %s", glGetString(GL_VENDOR));
		Output::Debug("OpenGL Renderer: %s", glGetString(GL_RENDERER));
		Output::Debug("OpenGL Version: %s", glGetString(GL_VERSION));
		Output::Debug("GLSL Version: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));
		// Output::Debug("OpenGL Extensions: %s", glGetString(GL_EXTENSIONS));
	}

	static bool set_fixed_uniforms = false;
	if (not set_fixed_uniforms) {
		set_fixed_uniforms = true;

		fill_effect_program->use();
		glUniformMatrix4fv(fill_effect_program->uniform_location("u_proj_mat"),
				   1, GL_FALSE, glm::value_ptr(screen_target_proj_mat));

		tiled_program->use();
		glUniformMatrix4fv(tiled_program->uniform_location("u_proj_mat"),
				   1, GL_FALSE, glm::value_ptr(screen_target_proj_mat));
		glUniform1i(tiled_program->uniform_location("u_texture"), 0);

		sprite_program->use();
		glUniformMatrix4fv(sprite_program->uniform_location("u_proj_mat"),
				   1, GL_FALSE, glm::value_ptr(screen_target_proj_mat));
		glUniform1i(sprite_program->uniform_location("u_texture"), 0);

		screen_fbo_program->use();
		glUniform1i(screen_fbo_program->uniform_location("u_texture"), 0);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, get_render_fbo());
	assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

	glViewport(screen_target_rect.x, screen_target_rect.y,
		   screen_target_rect.width, screen_target_rect.height);

	glClearColor(backcolor_.red  / 255.f, backcolor_.green / 255.f,
		     backcolor_.blue / 255.f, backcolor_.alpha / 255.f);
	glClear(GL_COLOR_BUFFER_BIT);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_drawables() {
	std::vector<Drawable*>::iterator i;

	state->sort();
	global_state->sort();

	for (i = state->drawable_list.begin(); i != state->drawable_list.end(); ++i) {
		if ((*i)->GetVisible()) (*i)->Draw();
	}

	for (i = global_state->drawable_list.begin(); i != global_state->drawable_list.end(); ++i) {
		if ((*i)->GetVisible()) (*i)->Draw();
	}
}

std::vector<GLuint> Shader::released_;
std::vector<GLuint> Program::released_;
std::vector<GLuint> Texture2D::released_;
void clear_gl_objects() {
	for (texture_map_type::const_iterator i = tex_map_.cbegin(); i != tex_map_.cend(); ) {
		if (i->second.bitmap.expired()) { i = tex_map_.erase(i); }
		else { ++i; }
	}

	Shader::clear_released();
	Program::clear_released();
	glDeleteBuffers(released_buffer_.size(), &released_buffer_.front());
	released_buffer_.clear();
	Texture2D::clear_released();
	// glReleaseShaderCompiler();
}

struct RenderEffects {
	RenderEffects()
			: opacity(255), tone(0, 0, 0, 0), color(0, 0, 0, 0)
			, ox(0), oy(0), flip_x(false), flip_y(false)
			, zoom_x(1.0), zoom_y(1.0), angle(0.0)
			, waver_phase(0.0), waver_depth(0)
			, bush_opacity(255), bush_depth(0)
	{}

	uint8_t opacity;
	Tone tone;
	Color color;
	int ox, oy;
	bool flip_x, flip_y;
	double zoom_x, zoom_y;
	double angle;
	double waver_phase;
	int waver_depth;
	uint8_t bush_opacity;
	int bush_depth;

	bool tone_enabled() const {
		return not (tone.red == tone.green == tone.blue == tone.gray == 0);
	}
	bool color_enabled() const { return color.alpha > 0; }
	bool opacity_enabled() const { return opacity != 255; }
	bool bush_enabled() const { return bush_opacity != 255 and bush_depth > 0; }

	glm::mat4 model_matrix() const {
		glm::mat4 m;
		m = glm::rotate(m, float(glm::radians(angle)), glm::vec3(0.f, 0.f, 1.f));
		m = glm::translate(m, glm::vec3(m * glm::vec4(-ox * zoom_x, -oy * zoom_y, 0.f, 1.f)));
		m = glm::scale(m, glm::vec3(zoom_x, zoom_y, 1.f));
		return m;
	}
};

RenderEffects const default_effect;

void render_texture(
    Rect const& dst_rect, BitmapRef const& bmp, Rect const& src_rect,
    optional<RenderEffects const&> eff_ = boost::none)
{
	RenderEffects const& eff = eff_? eff_.get() : default_effect;
	Program& prog = *sprite_program;

	prog.use();

	Texture2D& tex = get_texture(bmp);
	tex.bind();
	glm::mat4 model_mat = eff.model_matrix();
        model_mat = glm::translate(model_mat, glm::vec3(dst_rect.x, dst_rect.y, 0));

	glUniform1f(prog.uniform_location("u_opacity"), eff.opacity / 255.f);
	glUniform1f(prog.uniform_location("u_bush_opacity"), eff.bush_opacity / 255.f);
	glUniform1f(prog.uniform_location("u_bush_depth"),
		    GLfloat(src_rect.y + src_rect.height - eff.bush_depth) / tex.height());
	glUniform4f(prog.uniform_location("u_color"),
		    eff.color.red / 255.f, eff.color.green / 255.f, eff.color.blue / 255.f, eff.color.alpha / 255.f);
	glUniform4f(prog.uniform_location("u_tone"),
		    eff.tone.red / 255.f, eff.tone.green / 255.f, eff.tone.blue / 255.f, eff.tone.gray / 255.f);
	glUniform2fv(prog.uniform_location("u_tex_scale"), 1, glm::value_ptr(tex.scale()));
	glUniformMatrix4fv(prog.uniform_location("u_model_mat"), 1, GL_FALSE, glm::value_ptr(model_mat));

	if (eff.waver_depth == 0) {
		EASYRPG_ARRAY<GLshort, 2 * 4> dst_coord, src_coord;
		// set base coord
		for (int i = 0; i < 4; ++i) {
			src_coord[2 * i + 0] = src_rect.x;
			src_coord[2 * i + 1] = src_rect.y;
			dst_coord[2 * i + 0] = 0;
			dst_coord[2 * i + 1] = 0;
		}
		// set size of rectangle
		dst_coord[2 * 1 + 0] += dst_rect.width;
		dst_coord[2 * 2 + 0] += dst_rect.width;
		dst_coord[2 * 2 + 1] += dst_rect.height;
		dst_coord[2 * 3 + 1] += dst_rect.height;
		src_coord[2 * 1 + 0] += src_rect.width;
		src_coord[2 * 2 + 0] += src_rect.width;
		src_coord[2 * 2 + 1] += src_rect.height;
		src_coord[2 * 3 + 1] += src_rect.height;

		using std::swap;
		if (eff.flip_x) {
			swap(src_coord[2 * 0 + 0], src_coord[2 * 1 + 0]);
			swap(src_coord[2 * 3 + 0], src_coord[2 * 2 + 0]);
		}
		if (eff.flip_y) {
			swap(src_coord[2 * 0 + 1], src_coord[2 * 3 + 1]);
			swap(src_coord[2 * 1 + 1], src_coord[2 * 2 + 1]);
		}

		glVertexAttribPointer(prog.attrib_location("a_position"), 2,
				      GL_SHORT, GL_FALSE, 0, dst_coord.data());
		glVertexAttribPointer(prog.attrib_location("a_tex_coord"), 2,
				      GL_SHORT, GL_FALSE, 0, src_coord.data());

		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	} else {
		std::vector<GLshort>
				dst_coords(2 * (src_rect.height + 1)),
				src_coords(2 * (src_rect.height + 1));
		for (int i = 0; i < (src_rect.height + 1); ++i) {
			int const shift = 2 * eff.waver_depth * sin((eff.waver_phase + i * 11.2) * 3.14159 / 180);
			dst_coords[2 * i + 0] = shift;
			dst_coords[2 * i + 1] = i;
			dst_coords[2 * (i + src_rect.height + 1) + 0] = dst_rect.width + shift;
			dst_coords[2 * (i + src_rect.height + 1) + 1] = i;
			// TODO: flip
			src_coords[2 * i + 0] = src_rect.x;
			src_coords[2 * i + 1] = src_rect.y + i;
			src_coords[2 * (i + src_rect.height + 1) + 0] = src_rect.x + src_rect.width;
			src_coords[2 * (i + src_rect.height + 1) + 1] = src_rect.y + i;
		}

		glVertexAttribPointer(prog.attrib_location("a_position"), 2,
				      GL_SHORT, GL_FALSE, 0, &dst_coords.front());
		glVertexAttribPointer(prog.attrib_location("a_tex_coord"), 2,
				      GL_SHORT, GL_FALSE, 0, &src_coords.front());

		glDrawArrays(GL_TRIANGLE_FAN, 0, src_rect.height + 1);
	}
	assert(prog.validate());
}

void render_texture(
    int x, int y, BitmapRef const& bmp, Rect const& src_rect, optional<RenderEffects const&> eff = boost::none) {
	render_texture(Rect(x, y, src_rect.width, src_rect.height), bmp, src_rect, eff);
}

void tiled_render_texture(
    Rect const& dst_rect, BitmapRef const& bmp, Rect const& src_rect,
    optional<RenderEffects const&> eff_ = boost::none)
{
	RenderEffects const& eff = eff_? eff_.get() : default_effect;
	Program& prog = *tiled_program;

	prog.use();

	Texture2D& tex = get_texture(bmp);
	tex.bind();
	glm::vec2 tex_scale = tex.scale();
	glm::mat4 model_mat = eff.model_matrix();
        model_mat = glm::translate(model_mat, glm::vec3(dst_rect.x, dst_rect.y, 0.f));
	model_mat = glm::translate(model_mat, glm::vec3(eff.ox, eff.oy, 0.f));

	glUniform2fv(prog.uniform_location("u_tex_base_coord"), 1,
		     glm::value_ptr(glm::vec2(src_rect.x, src_rect.y) * tex_scale));
	glUniform2f(prog.uniform_location("u_tex_range"),
		    GLfloat(src_rect.width) / tex.width(),
		    GLfloat(src_rect.height) / tex.height());
	glUniform1f(prog.uniform_location("u_opacity"), eff.opacity / 255.f);
	glUniform4f(prog.uniform_location("u_color"),
		    eff.color.red / 255.f, eff.color.green / 255.f, eff.color.blue / 255.f, eff.color.alpha / 255.f);
	glUniform4f(prog.uniform_location("u_tone"),
		    eff.tone.red / 255.f, eff.tone.green / 255.f, eff.tone.blue / 255.f, eff.tone.gray / 255.f);
	glUniform2fv(prog.uniform_location("u_tex_scale"), 1, glm::value_ptr(tex_scale));
	glUniformMatrix4fv(prog.uniform_location("u_model_mat"), 1, GL_FALSE, glm::value_ptr(model_mat));

	EASYRPG_ARRAY<GLshort, 2 * 4> dst_coord, src_coord;
	// set base coord
	for (int i = 0; i < 4; ++i) {
		dst_coord[2 * i + 0] = 0;
		dst_coord[2 * i + 1] = 0;
		src_coord[2 * i + 0] = eff.ox;
		src_coord[2 * i + 1] = eff.oy + dst_rect.height % src_rect.height;
	}
	// set size of rectangle
	dst_coord[2 * 1 + 0] += dst_rect.width;
	dst_coord[2 * 2 + 0] += dst_rect.width;
	dst_coord[2 * 2 + 1] += dst_rect.height;
	dst_coord[2 * 3 + 1] += dst_rect.height;
	src_coord[2 * 1 + 0] += dst_rect.width;
	src_coord[2 * 2 + 0] += dst_rect.width;
	src_coord[2 * 2 + 1] += dst_rect.height;
	src_coord[2 * 3 + 1] += dst_rect.height;

	glVertexAttribPointer(prog.attrib_location("a_position"), 2,
			      GL_SHORT, GL_FALSE, 0, dst_coord.data());
	glVertexAttribPointer(prog.attrib_location("a_tex_coord"), 2,
			      GL_SHORT, GL_FALSE, 0, src_coord.data());

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	assert(prog.validate());
}

void fill_effect(Rect const& rect, RenderEffects const& eff) {
	glm::mat4 model_mat;
	model_mat = glm::translate(model_mat, glm::vec3(rect.x, rect.y, 0.f));

	Program& prog = *fill_effect_program;

	prog.use();

	glUniform1f(prog.uniform_location("u_opacity"), eff.opacity / 255.f);
	glUniform4f(prog.uniform_location("u_color"),
		    eff.color.red / 255.f, eff.color.green / 255.f, eff.color.blue / 255.f, eff.color.alpha / 255.f);
	glUniformMatrix4fv(prog.uniform_location("u_model_mat"), 1, GL_FALSE, glm::value_ptr(model_mat));


	EASYRPG_ARRAY<GLshort, 2 * 4> coord;
	// set base coord
	for (int i = 0; i < 4; ++i) {
		coord[2 * i + 0] = 0;
		coord[2 * i + 1] = 0;
	}
	// set size of rectangle
	coord[2 * 1 + 0] += rect.width;
	coord[2 * 2 + 0] += rect.width;
	coord[2 * 2 + 1] += rect.height;
	coord[2 * 3 + 1] += rect.height;
	// transform left top coordinate to left bottom coordinate
	for (int i = 0; i < 4; ++i) {
		coord[2 * i + 1] = rect.height - coord[2 * i + 1];
	}

	glVertexAttribPointer(prog.attrib_location("a_position"), 2,
			      GL_SHORT, GL_FALSE, 0, coord.data());

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	assert(prog.validate());
}

}

unsigned Graphics::GetWidth() { return screen_target_rect.width; }
unsigned Graphics::GetHeight() { return screen_target_rect.height; }

using Graphics::render_texture;
using Graphics::tiled_render_texture;
using Graphics::fill_effect;
using Graphics::RenderEffects;

unsigned SecondToFrame(float const second) {
	return(second * Graphics::framerate);
}

void Graphics::Init() {
	fps_on_screen = false;
	fps = 0;
	screen_erased = false;

	black_screen = Bitmap::Create(SCREEN_TARGET_WIDTH, SCREEN_TARGET_HEIGHT, Color(0,0,0,255));

	state.reset(new State());
	global_state.reset(new State());

	next_fps_time = 0;
}

void Graphics::Quit() {
	state->drawable_list.clear();
	global_state->drawable_list.clear();

	frozen_screen.reset();
	black_screen.reset();

	tex_map_.clear();

	Cache::Clear();
}

void Graphics::Update(bool time_left) {
	if (next_fps_time == 0) {
		next_fps_time = DisplayUi->GetTicks() + 1000;
	}

	// Calculate fps
	uint32_t current_time = DisplayUi->GetTicks();
	if (current_time >= next_fps_time) {
		// 1 sec over
		next_fps_time += 1000;
		real_fps = fps;
		fps = 0;

		next_fps_time = current_time + 1000;

		UpdateTitle();
	}

	// Render next frame
	if (time_left) {
		fps++;

		DrawFrame();
	}
}

void Graphics::UpdateTitle() {
	if (DisplayUi->IsFullscreen()) return;

	std::stringstream title;
	title << Player::game_title;

	if (!fps_on_screen) {
		title << " - FPS " << real_fps;
	}

	DisplayUi->SetTitle(title.str());
}

void Graphics::DrawFrame() {
	prepare_rendering();

	if (IsTransitionPending()) {
		UpdateTransition();

		std::vector<Drawable*>::iterator i;
		for (i = global_state->drawable_list.begin(); i != global_state->drawable_list.end(); ++i) {
			if ((*i)->GetVisible()) (*i)->Draw();
		}

		DrawOverlay();

		DisplayUi->UpdateDisplay();
		return;
	}

	if (screen_erased) {
		return;
	}

	process_drawables();

	DrawOverlay();

	// bind default FBO and draw render result
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
	glDisable(GL_BLEND);
	glViewport(0, 0, DisplayUi->GetWidth(), DisplayUi->GetHeight());

	screen_fbo_program->use();
	screen_texture.bind();
	screen_fbo_buffer.bind();
	glVertexAttribPointer(screen_fbo_program->attrib_location("a_position"), 2, GL_FLOAT, GL_FALSE, 0, 0);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	assert(screen_fbo_program->validate());

	DisplayUi->UpdateDisplay();

	// unbind buffers
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	clear_gl_objects();
	check_gl_error();
}

void Graphics::DrawOverlay() {
	if (Graphics::fps_on_screen) {
		static BitmapRef const bmp = Bitmap::Create(12 * 10, 12);
		bmp->Clear();
		std::ostringstream text;
		text << "FPS: " << real_fps;
		Text::Draw(*bmp, 0,0, Color(255, 255, 255, 255), text.str());
		render_texture(2, 2, bmp, bmp->GetRect());
	}
}

BitmapRef Graphics::SnapToBitmap() {
	prepare_rendering();
	process_drawables();

	glFlush();

	uint32_t *result = reinterpret_cast<uint32_t*>(
	    std::malloc(sizeof(uint32_t) * screen_target_rect.width * screen_target_rect.height));
	glReadPixels(0, 0, screen_target_rect.width, screen_target_rect.height, GL_RGBA, GL_UNSIGNED_BYTE, result);

	// flip vertical
	for (int y = 0; y < (screen_target_rect.height / 2); ++y) {
		for (int x = 0; x < screen_target_rect.width; ++x) {
			using std::swap;
			swap(result[screen_target_rect.width * y + x],
			     result[screen_target_rect.width * (screen_target_rect.height - y - 1) + x]);
		}
	}

	return EASYRPG_MAKE_SHARED<Bitmap>(result, screen_target_rect.width, screen_target_rect.height, 0);
}

void Graphics::Freeze() {
	frozen_screen = SnapToBitmap();
}

void Graphics::Transition(TransitionType type, int duration, bool erase) {
	if (type != TransitionNone) {
		transition_type = type;
		transition_frame = 0;
		transition_duration = type == TransitionErase ? 1 : duration;

		Freeze();

		if (erase) {
			transition_from = frozen_screen;
			transition_to = black_screen;
		} else {
			transition_from = screen_erased? black_screen : frozen_screen;
			transition_to = frozen_screen;
		}
	}

	screen_erased = erase;
}

bool Graphics::IsTransitionPending() {
	return (transition_duration - transition_frame) > 0;
}

void Graphics::UpdateTransition() {
	int const w = SCREEN_TARGET_WIDTH, h = SCREEN_TARGET_HEIGHT;
	float const progress = float(transition_frame++) / transition_duration;

	// Fallback to FadeIn/Out for not implemented transition types:
	// (Remove from here when implemented below)
	switch (transition_type) {
	case TransitionRandomBlocks:
	case TransitionRandomBlocksUp:
	case TransitionRandomBlocksDown:
	case TransitionZoomIn:
	case TransitionZoomOut:
	case TransitionMosaicIn:
	case TransitionMosaicOut:
	case TransitionWaveIn:
	case TransitionWaveOut:
		transition_type = TransitionFadeIn;
		break;
	default:
		break;
	}

	switch (transition_type) {
	case TransitionFadeIn:
	case TransitionFadeOut: {
		RenderEffects eff;
		eff.opacity = 255 * progress;
		render_texture(0, 0, transition_from, transition_from->GetRect());
		render_texture(0, 0, transition_to, transition_to->GetRect(), eff);
	} break;
	case TransitionRandomBlocks:
		break;
	case TransitionRandomBlocksUp:
		break;
	case TransitionRandomBlocksDown:
		break;
	case TransitionBlindOpen: {
		RenderEffects eff;
		eff.opacity = 255 * progress;
		for (int i = 0; i < h / 8; i++) {
			render_texture(0, i * 8, transition_from, Rect(0, i * 8, w, 8 - 8 * progress));
			render_texture(0, i * 8 + 8 - 8 * progress, transition_to, Rect(0, i * 8 + 8 - 8 * progress, w, 8 * progress), eff);
		}
	} break;
	case TransitionBlindClose:
		for (int i = 0; i < h / 8; i++) {
			render_texture(0, i * 8 + 8 * progress, transition_from, Rect(0, i * 8 + 8 * progress, w, 8 - 8 * progress));
			render_texture(0, i * 8, transition_to, Rect(0, i * 8, w, 8 * progress));
		}
		break;
	case TransitionVerticalStripesIn:
	case TransitionVerticalStripesOut:
		for (int i = 0; i < h / 6 + 1 - h / 6 * progress; i++) {
			render_texture(0, i * 6 + 3, transition_from, Rect(0, i * 6 + 3, w, 3));
			render_texture(0, h - i * 6, transition_from, Rect(0, h - i * 6, w, 3));
		}
		for (int i = 0; i < h / 6 * progress; i++) {
			render_texture(0, i * 6, transition_to, Rect(0, i * 6, w, 3));
			render_texture(0, h - 3 - i * 6, transition_to, Rect(0, h - 3 - i * 6, w, 3));
		}
		break;
	case TransitionHorizontalStripesIn:
	case TransitionHorizontalStripesOut:
		for (int i = 0; i < w / 8 + 1 - w / 8 * progress; i++) {
			render_texture(i * 8 + 4, 0, transition_from, Rect(i * 8 + 4, 0, 4, h));
			render_texture(w - i * 8, 0, transition_from, Rect(w - i * 8, 0, 4, h));
		}
		for (int i = 0; i < w / 8 * progress; i++) {
			render_texture(i * 8, 0, transition_to, Rect(i * 8, 0, 4, h));
			render_texture(w - 4 - i * 8, 0, transition_to, Rect(w - 4 - i * 8, 0, 4, h));
		}
		break;
	case TransitionBorderToCenterIn:
	case TransitionBorderToCenterOut:
		render_texture(0,  0, transition_to, transition_to->GetRect());
		render_texture((w / 2) * progress, (h / 2) * progress, transition_from, Rect((w / 2) * progress, (h / 2) * progress, w - w * progress, h - h * progress));
		break;
	case TransitionCenterToBorderIn:
	case TransitionCenterToBorderOut:
		render_texture(0,  0, transition_from, transition_from->GetRect());
		render_texture(w / 2 - (w / 2) * progress, h / 2 - (h / 2) * progress, transition_to, Rect(w / 2 - (w / 2) * progress, h / 2 - (h / 2) * progress, w * progress, h * progress));
		break;
	case TransitionScrollUpIn:
	case TransitionScrollUpOut:
		render_texture(0,  -h * progress, transition_from, transition_from->GetRect());
		render_texture(0,  h - h * progress, transition_to, transition_to->GetRect());
		break;
	case TransitionScrollDownIn:
	case TransitionScrollDownOut:
		render_texture(0,  h * progress, transition_from, transition_from->GetRect());
		render_texture(0,  -h + h * progress, transition_to, transition_to->GetRect());
		break;
	case TransitionScrollLeftIn:
	case TransitionScrollLeftOut:
		render_texture(-w * progress,  0, transition_from, transition_from->GetRect());
		render_texture(w - w * progress,  0, transition_to, transition_to->GetRect());
		break;
	case TransitionScrollRightIn:
	case TransitionScrollRightOut:
		render_texture(w * progress,  0, transition_from, transition_from->GetRect());
		render_texture(-w + w * progress,  0, transition_to, transition_to->GetRect());
		break;
	case TransitionVerticalCombine:
		render_texture(0, (h / 2) * progress, transition_from, Rect(0, (h / 2) * progress, w, h - h * progress));
		render_texture(0, -h / 2 + (h / 2) * progress, transition_to, Rect(0, 0, w, h / 2));
		render_texture(0, h - (h / 2) * progress, transition_to, Rect(0, h / 2, w, h / 2));
		break;
	case TransitionVerticalDivision:
		render_texture(0, -(h / 2) * progress, transition_from, Rect(0, 0, w, h / 2));
		render_texture(0, h / 2 + (h / 2) * progress, transition_from, Rect(0, h / 2, w, h / 2));
		render_texture(0, h / 2 - (h / 2) * progress, transition_to, Rect(0, h / 2 - (h / 2) * progress, w, h * progress));
		break;
	case TransitionHorizontalCombine:
		render_texture((w / 2) * progress, 0, transition_from, Rect((w / 2) * progress, 0, w - w * progress, h));
		render_texture(- w / 2 + (w / 2) * progress, 0, transition_to, Rect(0, 0, w / 2, h));
		render_texture(w - (w / 2) * progress, 0, transition_to, Rect(w / 2, 0, w / 2, h));
		break;
	case TransitionHorizontalDivision:
		render_texture(-(w / 2) * progress, 0, transition_from, Rect(0, 0, w / 2, h));
		render_texture(w / 2 + (w / 2) * progress, 0, transition_from, Rect(w / 2, 0, w / 2, h));
		render_texture(w / 2 - (w / 2) * progress, 0, transition_to, Rect(w / 2 - (w / 2) * progress, 0, w * progress, h));
		break;
	case TransitionCrossCombine:
		render_texture((w / 2) * progress, 0, transition_from, Rect((w / 2) * progress, 0, w - w * progress, (h / 2) * progress));
		render_texture((w / 2) * progress, h - (h / 2) * progress, transition_from, Rect((w / 2) * progress, h - (h / 2) * progress, w - w * progress, (h / 2) * progress));
		render_texture(0, (h / 2) * progress, transition_from, Rect(0, (h / 2) * progress, w, h - h * progress));
		render_texture(- w / 2 + (w / 2) * progress, -h / 2 + (h / 2) * progress, transition_to, Rect(0, 0, w / 2, h / 2));
		render_texture(w - (w / 2) * progress, -h / 2 + (h / 2) * progress, transition_to, Rect(w / 2, 0, w / 2, h / 2));
		render_texture(w - (w / 2) * progress, h - (h / 2) * progress, transition_to, Rect(w / 2, h / 2, w / 2, h / 2));
		render_texture(- w / 2 + (w / 2) * progress, h - (h / 2) * progress, transition_to, Rect(0, h / 2, w / 2, h / 2));
		break;
	case TransitionCrossDivision:
		render_texture(-(w / 2) * progress, -(h / 2) * progress, transition_from, Rect(0, 0, w / 2, h / 2));
		render_texture(w / 2 + (w / 2) * progress, -(h / 2) * progress, transition_from, Rect(w / 2, 0, w / 2, h / 2));
		render_texture(w / 2 + (w / 2) * progress, h / 2 + (h / 2) * progress, transition_from, Rect(w / 2, h / 2, w / 2, h / 2));
		render_texture(-(w / 2) * progress, h / 2 + (h / 2) * progress, transition_from, Rect(0, h / 2, w / 2, h / 2));
		render_texture(w / 2 - (w / 2) * progress, 0, transition_to, Rect(w / 2 - (w / 2) * progress, 0, w * progress, h / 2 - (h / 2) * progress));
		render_texture(w / 2 - (w / 2) * progress, h / 2 + (h / 2) * progress, transition_to, Rect(w / 2 - (w / 2) * progress, h / 2 + (h / 2) * progress, w * progress, h / 2 + (h / 2) * progress));
		render_texture(0, h / 2 - (h / 2) * progress, transition_to, Rect(0, h / 2 - (h / 2) * progress, w, h * progress));
		break;
	case TransitionZoomIn:
		break;
	case TransitionZoomOut:
		break;
	case TransitionMosaicIn:
		break;
	case TransitionMosaicOut:
		break;
	case TransitionWaveIn:
		break;
	case TransitionWaveOut:
		break;
	default:
		break;
	}
}

void Graphics::FrameReset() {
	next_fps_time = (uint32_t)DisplayUi->GetTicks() + 1000;
	fps = 0;
}

void Graphics::Push() {
	stack.push_back(state);
	state.reset(new State());
}

void Graphics::Pop() {
	if (stack.size() > 0) {
		state = stack.back();
		stack.pop_back();
	}
}

int Graphics::GetDefaultFps() {
	return DEFAULT_FPS;
}

void Graphics::SetBackcolor(Color const& c) {
	backcolor_ = c;
}

Drawable::Drawable(DrawableType t, int z, bool g) : type(t), z(z), global(g), visible(true) {
	Graphics::State& s = global? *Graphics::global_state : *Graphics::state;
	s.drawable_list.push_back(this);
	s.zlist_dirty = true;
}

Drawable::~Drawable() {
	EASYRPG_SHARED_PTR<Graphics::State> const& ptr = global? Graphics::global_state : Graphics::state;
	if (not ptr) { return; }

	std::vector<Drawable*>& list = ptr->drawable_list;
	std::vector<Drawable*>::const_iterator it = std::find(list.begin(), list.end(), this);
	if (it != list.end()) { list.erase(it); }
}

int Drawable::GetZ() const { return z; }
DrawableType Drawable::GetType() const { return type; }
void Drawable::SetZ(int n_z) {
	if (n_z == z) { return; }
	z = n_z;
	(global? Graphics::global_state : Graphics::state)->zlist_dirty = true;
}

bool Drawable::GetVisible() const { return visible; }
void Drawable::SetVisible(bool v) { visible = v; }

void TilemapLayer::prepare_draw() {
	Graphics::Program& prog = *Graphics::sprite_program;
	prog.use();

	Graphics::Texture2D& tex = Graphics::get_texture(chipset);
	tex.bind();

	glUniform1f(prog.uniform_location("u_opacity"), 1.f);
	glUniform1f(prog.uniform_location("u_bush_opacity"), 1.f);
	glUniform1f(prog.uniform_location("u_bush_depth"), 0.f);
	glUniform4f(prog.uniform_location("u_color"), 0.f, 0.f, 0.f, 0.f);
	glUniform4f(prog.uniform_location("u_tone"), 0.f, 0.f, 0.f, 0.f);
	glUniform2fv(prog.uniform_location("u_tex_scale"), 1, glm::value_ptr(tex.scale()));
	glUniformMatrix4fv(prog.uniform_location("u_model_mat"), 1, GL_FALSE, glm::value_ptr(glm::mat4()));

	a_position_idx = prog.attrib_location("a_position");
	a_tex_coord_idx = prog.attrib_location("a_tex_coord");
}

void TilemapLayer::DrawTile(int x, int y, int row, int col) {
	for (int i = 0; i < 4; ++i) {
		dst_coord[2 * i + 0] = x;
		dst_coord[2 * i + 1] = y;
		src_coord[2 * i + 0] = TILE_SIZE * col;
		src_coord[2 * i + 1] = TILE_SIZE * row;
	}
	dst_coord[2 * 1 + 0] += TILE_SIZE;
	dst_coord[2 * 2 + 0] += TILE_SIZE;
	dst_coord[2 * 2 + 1] += TILE_SIZE;
	dst_coord[2 * 3 + 1] += TILE_SIZE;
	src_coord[2 * 1 + 0] += TILE_SIZE;
	src_coord[2 * 2 + 0] += TILE_SIZE;
	src_coord[2 * 2 + 1] += TILE_SIZE;
	src_coord[2 * 3 + 1] += TILE_SIZE;

	glVertexAttribPointer(a_position_idx, 2, GL_SHORT, GL_FALSE, 0, dst_coord.data());
	glVertexAttribPointer(a_tex_coord_idx, 2, GL_SHORT, GL_FALSE, 0, src_coord.data());
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void TilemapLayer::DrawTile(int x, int y, TilemapLayer::subtile_coords const& coords) {
	for (size_t i = 0; i < coords.size(); ++i) {
		if (coords[i][0] == SKIP_SUBTILE) { continue; }

		for (int j = 0; j < 4; ++j) {
			dst_coord[2 * j + 0] = x + subtile_base[i][0];
			dst_coord[2 * j + 1] = y + subtile_base[i][1];
			src_coord[2 * j + 0] = coords[i][0];
			src_coord[2 * j + 1] = coords[i][1];
		}
		dst_coord[2 * 1 + 0] += TILE_SIZE / 2;
		dst_coord[2 * 2 + 0] += TILE_SIZE / 2;
		dst_coord[2 * 2 + 1] += TILE_SIZE / 2;
		dst_coord[2 * 3 + 1] += TILE_SIZE / 2;
		src_coord[2 * 1 + 0] += TILE_SIZE / 2;
		src_coord[2 * 2 + 0] += TILE_SIZE / 2;
		src_coord[2 * 2 + 1] += TILE_SIZE / 2;
		src_coord[2 * 3 + 1] += TILE_SIZE / 2;

		glVertexAttribPointer(a_position_idx, 2, GL_SHORT, GL_FALSE, 0, dst_coord.data());
		glVertexAttribPointer(a_tex_coord_idx, 2, GL_SHORT, GL_FALSE, 0, src_coord.data());
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	}
}

void Background::Draw() {
	RenderEffects eff;

	if (bg_bitmap) {
		eff.ox = -Scale(bg_x); eff.oy = -Scale(bg_y);
		tiled_render_texture(Graphics::screen_target_rect, bg_bitmap, bg_bitmap->GetRect(), eff);
	}

	if (fg_bitmap) {
		eff.ox = -Scale(fg_x); eff.oy = -Scale(fg_y);
		tiled_render_texture(Graphics::screen_target_rect, fg_bitmap, fg_bitmap->GetRect(), eff);
	}
}

void BattleAnimation::Draw() {
	if (!screen) {
		// Initialization failed
		return;
	}

	if (frame >= (int) animation->frames.size())
		return;

	const RPG::AnimationFrame& anim_frame = animation->frames[frame];

	std::vector<RPG::AnimationCellData>::const_iterator it;
	for (it = anim_frame.cells.begin(); it != anim_frame.cells.end(); ++it) {
		const RPG::AnimationCellData& cell = *it;
		int sx = cell.cell_id % 5;
		int sy = cell.cell_id / 5;
		int size = large ? 128 : 96;
		Rect src_rect(sx * size, sy * size, size, size);
		double zoom = cell.zoom / 100.0;
		RenderEffects eff;
		eff.opacity = 255 * (100 - cell.transparency) / 100;
		eff.tone.Set(cell.tone_red, cell.tone_green, cell.tone_blue, cell.tone_gray);
		Rect dst_rect(x + cell.x - size / 2 * zoom, y + cell.y - size / 2 * zoom,
			      size * zoom, size * zoom);
		render_texture(dst_rect, screen, src_rect, eff);
	}
}

void MessageOverlay::Draw() {
	std::deque<MessageOverlayItem>::iterator it;

	++counter;
	if (counter > 150) {
		counter = 0;
		if (!messages.empty()) {
			for (it = messages.begin(); it != messages.end(); ++it) {
				if (!it->hidden) {
					it->hidden = true;
					break;
				}
			}
			dirty = true;
		}
	} else {
		if (!messages.empty()) {
			render_texture(ox, oy, bitmap, bitmap->GetRect());
		}
	}

	if (!dirty) return;

	bitmap->Clear();

	int i = 0;

	for (it = messages.begin(); it != messages.end(); ++it) {
		if (!it->hidden || show_all) {
			bitmap->Blit(0, i * text_height, *black, black->GetRect(), 128);
			Text::Draw(*bitmap, 2, i * text_height, it->color, it->text);
			++i;
		}
	}

	render_texture(ox, oy, bitmap, bitmap->GetRect());

	dirty = false;
}

void Plane::Draw() {
	if (!bitmap) return;

	RenderEffects eff;
	eff.ox = -ox; eff.oy = -oy;
	tiled_render_texture(Graphics::screen_target_rect, bitmap, bitmap->GetRect());
}

void Screen::Draw() {
	Tone const& tone = Main_Data::game_screen->GetTone();

	if (tone != default_tone) {
		/* TODO: screen tone
		RenderEffects eff;
		eff.tone = tone;
		fill_effect(Graphics::screen_target_rect, eff);
		*/
	}

	int flash_time_left;
	int flash_current_level;
	Color flash_color = Main_Data::game_screen->GetFlash(flash_current_level, flash_time_left);

	if (flash_time_left > 0) {
		RenderEffects eff;
		eff.color = flash_color;
		eff.opacity = flash_current_level;
		fill_effect(Graphics::screen_target_rect, eff);
	}
}

// Draw
void Sprite::Draw() {
	if (not bitmap) { return; }

	RenderEffects eff;
	eff.ox = ox;
	eff.oy = oy;
	eff.tone = tone_effect;
	eff.color = blend_color_effect;
	if (flash_color.alpha != 0) {
		eff.color.red = std::min(255, eff.color.red * eff.color.alpha + flash_color.red * (255 - eff.color.alpha));
		eff.color.green = std::min(255, eff.color.green * eff.color.alpha + flash_color.green * (255 - eff.color.alpha));
		eff.color.blue = std::min(255, eff.color.blue * eff.color.alpha + flash_color.blue * (255 - eff.color.alpha));
		eff.color.alpha = eff.color.alpha / 255.f * flash_color.alpha / 255.f;
	}
	eff.flip_x = flipx_effect;
	eff.flip_y = flipy_effect;
	eff.zoom_x = zoom_x_effect;
	eff.zoom_y = zoom_y_effect;
	eff.angle = angle_effect;
	eff.waver_phase = waver_effect_phase;
	eff.waver_depth = waver_effect_depth;
	eff.opacity = opacity_top_effect;
	if (bush_effect > 0) {
		eff.bush_opacity = opacity_bottom_effect;
		eff.bush_depth = bush_effect;
	}
	render_texture(Rect(x, y, src_rect.width, src_rect.height), bitmap, src_rect, eff);
}

void Weather::Draw() {
	static const int snowflake_visible = 150;

	switch (Main_Data::game_screen->GetWeatherType()) {
	case Game_Screen::Weather_None:
		break;

	case Game_Screen::Weather_Rain: {
		Rect const rect = rain_bitmap->GetRect();
		RenderEffects eff;
		eff.opacity = 96;

		const std::vector<Game_Screen::Snowflake>& snowflakes = Main_Data::game_screen->GetSnowflakes();

		std::vector<Game_Screen::Snowflake>::const_iterator it;
		for (it = snowflakes.begin(); it != snowflakes.end(); ++it) {
			if (it->life > snowflake_visible)
				continue;
			render_texture(it->x - it->y / 2, it->y, rain_bitmap, rect, eff);
		}
	} break;

	case Game_Screen::Weather_Snow: {
		static const int wobble[2][18] = {
			{-1,-1, 0, 1, 0, 1, 1, 0,-1,-1, 0, 1, 0, 1, 1, 0,-1, 0},
			{-1,-1, 0, 0, 1, 1, 0,-1,-1, 0, 1, 0, 1, 1, 0,-1, 0, 0}
		};

		Rect const rect = snow_bitmap->GetRect();
		RenderEffects eff;
		eff.opacity = 192;

		const std::vector<Game_Screen::Snowflake>& snowflakes = Main_Data::game_screen->GetSnowflakes();

		std::vector<Game_Screen::Snowflake>::const_iterator it;
		for (it = snowflakes.begin(); it != snowflakes.end(); ++it) {
			if (it->life > snowflake_visible)
				continue;
			int x = it->x - it->y/2;
			int y = it->y;
			int i = (y / 2) % 18;
			x += wobble[0][i];
			y += wobble[1][i];
			render_texture(x, y, snow_bitmap, rect, eff);
		}
	} break;

	case Game_Screen::Weather_Fog: {
		static const int opacities[3] = {128, 160, 192};
		RenderEffects eff;
		eff.color = Color(128, 128, 128, opacities[Main_Data::game_screen->GetWeatherStrength()]);
		fill_effect(Graphics::screen_target_rect, eff);
	} break;

	case Game_Screen::Weather_Sandstorm: {
		static const int opacities[3] = {128, 160, 192};
		RenderEffects eff;
		eff.color = Color(192, 160, 128, opacities[Main_Data::game_screen->GetWeatherStrength()]);
		fill_effect(Graphics::screen_target_rect, eff);

		// TODO
	} break;
	}
}

void Window::Draw() {
	int ianimation_count = animation_count;

	if (windowskin) {
		// background
		if (width > 4 && height > 4 && (back_opacity * opacity / 255 > 0)) {
			RenderEffects effect;
			effect.opacity = back_opacity * opacity / 255;
			Rect dst_rect(x, y, width, height);
			if (animation_frames > 0) {
				dst_rect.y += height / 2 - ianimation_count;
				dst_rect.height = ianimation_count * 2;
			}
			Rect src_rect(0, 0, 16, 16);
			if (stretch) {
				src_rect.width = src_rect.height = 32;
				render_texture(dst_rect, windowskin, src_rect, effect);
			} else {
				tiled_render_texture(dst_rect, windowskin, src_rect, effect);
			}
		}

		// frame
		if (width > 0 && height > 0 && opacity > 0 and (animation_frames == 0 or ianimation_count > 0)) {
			int dst_x = x, dst_y = y;
			size_t dst_w = width, dst_h = height;
			if (animation_frames > 0) {
				dst_y += height / 2 - ianimation_count;
				dst_h = ianimation_count * 2;
			}
			int const frame_h = std::min<int>(8, dst_h / 2);
			RenderEffects eff;

			// Upper left corner
			render_texture(dst_x, dst_y, windowskin, Rect(32, 0, 8, 8));
			// Upper right corner
			render_texture(dst_x + dst_w - 8, dst_y, windowskin, Rect(32 + 32 - 8, 0, 8, 8));
			// Lower left corner
			render_texture(dst_x, dst_y + dst_h - 8, windowskin, Rect(32, 32 - 8, 8, 8));
			// Lower right corner
			render_texture(dst_x + dst_w - 8, dst_y + dst_h - 8, windowskin,
				       Rect(32 + 32 - 8, 32 - 8, 8, 8));

			eff.ox = 8; eff.oy = 0;
			// border up
			tiled_render_texture(Rect(dst_x + 8, dst_y, dst_w - 16, frame_h), windowskin,
					     Rect(32 + 8, 0, 16, frame_h), eff);
			// border down
			tiled_render_texture(Rect(dst_x + 8, dst_y + dst_h - frame_h, dst_w - 16, frame_h), windowskin,
					     Rect(32 + 8, 32 - frame_h, 16, frame_h), eff);

			if (animation_frames == 0 or ianimation_count > 8) {
				eff.ox = 0; eff.oy = 8;
				// border left
				tiled_render_texture(Rect(dst_x, dst_y + 8, 8, dst_h - 16), windowskin,
						     Rect(32, 8, 8, 16), eff);
				// border right
				tiled_render_texture(Rect(dst_x + dst_w - 8, dst_y + 8, 8, dst_h - 16), windowskin,
						     Rect(32 + 32 - 8, 8, 8, 16), eff);
			}
		}

		// cursor
		if (width > 16 && height > 16 && cursor_rect.width > 4 && cursor_rect.height > 4 && animation_frames == 0) {
			int const dst_x = x + cursor_rect.x + border_x,
				  dst_y = y + cursor_rect.y + border_y;
			int const base_src_x = cursor_frame <= 10? 64 : 96;
			int const cw = cursor_rect.width, ch = cursor_rect.height;

			RenderEffects eff;

			eff.ox = 8; eff.oy = 0;
			// Border Up
			tiled_render_texture(Rect(dst_x + 8, dst_y, cw - 16, 8), windowskin,
					     Rect(base_src_x + 8, 0, 16, 8), eff);
			// Border Down
			tiled_render_texture(Rect(dst_x + 8, dst_y + ch - 8, cw - 16, 8), windowskin,
					     Rect(base_src_x + 8, 32 - 8, 16, 8), eff);

			eff.ox = 0; eff.oy = 8;
			// Border Left
			tiled_render_texture(Rect(dst_x, dst_y + 8, 8, ch - 16), windowskin,
					     Rect(base_src_x, 8, 8, 16), eff);
			// Border Right
			tiled_render_texture(Rect(dst_x + cw - 8, dst_y + 8, 8, ch - 16), windowskin,
					     Rect(base_src_x + 32 - 8, 8, 8, 16), eff);

			// Upper left corner
			render_texture(dst_x, dst_y, windowskin, Rect(base_src_x, 0, 8, 8));
			// Upper right corner
			render_texture(dst_x + cw - 8, dst_y, windowskin, Rect(base_src_x + 32 - 8, 0, 8, 8));
			// Lower left corner
			render_texture(dst_x, dst_y + ch - 8, windowskin, Rect(base_src_x, 32 - 8, 8, 8));
			// Lower right corner
			render_texture(dst_x + cw - 8, dst_y + ch - 8, windowskin,
				       Rect(base_src_x + 32 - 8, 32 - 8, 8, 8));

			eff.ox = 8; eff.oy = 8;
			// Background
			tiled_render_texture(Rect(dst_x + 8, dst_y + 8, cw - 16, ch - 16), windowskin,
					     Rect(base_src_x + 8, 8, 16, 16), eff);
		}
	}

	if (contents) {
		if (width > 2 * border_x && height > 2 * border_y &&
			-ox < width - 2 * border_x && -oy < height - 2 * border_y &&
			contents_opacity > 0 && animation_frames == 0) {
			Rect const src_rect(-min(-ox, 0), -min(-oy, 0),
					    min(width - 2 * border_x, width - 2 * border_x + ox),
					    min(height - 2 * border_y, height - 2 * border_y + oy));

			RenderEffects eff;
			eff.opacity = contents_opacity;
			render_texture(max(x + border_x, x + border_x - ox),
				       max(y + border_y, y + border_y - oy),
				       contents, src_rect, eff);
		}
	}

	if (pause && pause_frame > 16 && animation_frames <= 0) {
		render_texture(x + width / 2 - 8, y + height - 8, windowskin, Rect(40, 16, 16, 8));
	}

	if (up_arrow) {
		render_texture(x + width / 2 - 8, y, windowskin, Rect(40, 8, 16, 8));
	}

	if (down_arrow) {
		render_texture(x + width / 2 - 8, y + height - 8, windowskin, Rect(40, 16, 16, 8));
	}

	if (animation_frames > 0) {
		// Open/Close Animation
		animation_frames -= 1;
		animation_count += animation_increment;
		if (closing && animation_frames <= 0) {
			SetVisible(false);
			closing = false;
		}
	}
}
