#!/usr/bin/env ruby

# usage> ruby shader_tool.rb "input" "output" "vertex or fragment shader for checking"

raise "invalid arguments: #{ARGV}" if ARGV.length != 3

shader_src = IO.read ARGV[0]

begin
  require 'rubygems'
  require 'opengl_es'
  require 'glut'

  OpenGL.load_dll
  GLUT.load_dll

  include OpenGL
  include GLUT

  glutInit [1].pack('I'), [""].pack('p')
  glutInitDisplayMode GLUT_DOUBLE | GLUT_RGBA
  glutCreateWindow 'test'
  glutHideWindow

  def compile_shader name, src
    ret = glCreateShader name.end_with?('.frag') ? GL_FRAGMENT_SHADER : GL_VERTEX_SHADER
    glShaderSource ret, 1, [src].pack('p'), [src.size].pack('I')
    glCompileShader ret

    info_len = '    '
    glGetShaderiv ret, GL_INFO_LOG_LENGTH, info_len
    info_len = info_len.unpack('L')[0]
    log = ' ' * info_len
    glGetShaderInfoLog ret, info_len, nil, log
    print "Shader compile log: #{log}\n" if log.size > 0

    compile_result = '    '
    glGetShaderiv ret, GL_COMPILE_STATUS, compile_result
    raise "Shader compile failed: #{name}" unless compile_result.unpack('L')[0] != 0

    ret
  end

  prog = glCreateProgram
  glAttachShader prog, compile_shader(ARGV[0], shader_src)
  glAttachShader prog, compile_shader(ARGV[2], IO.read(ARGV[2]))
  glLinkProgram prog
  info_len = '    '
  glGetProgramiv prog, GL_INFO_LOG_LENGTH, info_len
  info_len = info_len.unpack('L')[0]
  log = ' ' * info_len
  glGetProgramInfoLog prog, info_len, nil, log
  print "Shader link log: #{log}\n" if log.size > 0
  link_result = '    '
  glGetProgramiv prog, GL_LINK_STATUS, link_result
  raise "Shader compile failed: #{ARGV[0]}" unless link_result.unpack('L')[0] != 0
rescue LoadError => e
  print "Skipping shader compile test: #{e}\n"
end

f = File.open ARGV[1], 'w'
shader_src.each_line do |l|
  f.write "\"#{l.chomp}\\n\"\n"
end
