#pragma once

#include <GL/glew.h>
#include <GL/gl.h>

#include <vector>
#include <sstream>
#include <exception>
#include <fstream>
#include <string>

namespace shaders
{

  /* Reads a text file into an string (in an unnecesarilly efficient way).*/
  std::string read_text_file(const std::string & file)
  {
    std::string buffer;
    std::ifstream is(file.c_str());
    if(is.good()){
      is.seekg(0, std::ios::end);   
      buffer.reserve(is.tellg());
      is.seekg(0, std::ios::beg);
 
      buffer.assign((std::istreambuf_iterator<char>(is)),
                    std::istreambuf_iterator<char>());
 
      return buffer;
    }
    throw std::runtime_error("Cannot read file \"" + file + "\"");
  }

  /* returns true if shader failed to compile, 
   * writes in error the compilation log */
  static bool shader_error(GLuint shader, std::ostream & error){
    GLint compiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if(!compiled){
      GLint error_length = 0;
      glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &error_length);
      std::vector<GLchar> error_buffer(error_length,0);
      glGetShaderInfoLog(shader, error_length, &error_length, error_buffer.data());
      error << std::string(error_buffer.begin(),error_buffer.end());
      return true;
    }
    return false;
  }

  /* returns true if program failed to link, 
   * writes in error the compilation log */
  static bool program_error(GLuint program, std::ostream & error){
    GLint linked = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if(!linked){
      GLint error_length = 0;
      glGetProgramiv(program, GL_INFO_LOG_LENGTH, &error_length);
      std::vector<GLchar> error_buffer(error_length,0);
      glGetProgramInfoLog(program, error_length, &error_length, error_buffer.data());
      error << std::string(error_buffer.begin(),error_buffer.end());
      return true;
    }
    return false;
  }

  /* Links a program from a list of shaders. Throws an exception in case of 
   * error.*/
  GLuint link_program(const std::vector<GLuint> & shaders)
  {
    typedef std::vector<GLuint>::const_iterator shaders_it;

    GLuint program = 0;
    std::stringstream error;

    program = glCreateProgram();
    for(shaders_it it = shaders.begin(); it != shaders.end(); ++it)
      glAttachShader(program,*it);

    //set_feedback(program,feedback_varyings,interlaced);
    glLinkProgram(program);

    for(shaders_it it = shaders.begin(); it != shaders.end(); ++it)
      glDetachShader(program,*it);

    error << "Error linking program: " << std::endl;
    if(program_error(program,error)){    
      glDeleteProgram(program);
      throw std::runtime_error(error.str());
    }

    return program;
  }

  /* Compiles a shader. If there is a compilation error throws an exception
   * with the details of the error as the .what() field*/
  GLuint compile_shader(const std::string & file, GLenum shader_type)
  {
    std::stringstream error;
    std::string code = read_text_file(file);
    const char * c_code = code.c_str();
    GLuint shader = glCreateShader(shader_type);

    glShaderSource(shader, 1, &c_code, 0);
    glCompileShader(shader);

    error << "Error compiling shader file: " << file << std::endl;
    if(shader_error(shader,error)){
      glDeleteShader(shader);
      throw std::runtime_error(error.str());
    }

    return shader;
  }

  /* This is the one you are probably going to use.
   * It always expects the first file to be a vertex shader and
   * the second a fragment shader.
   * I the code I've sent, in the c++14 section, the function
   *  selects automatically the type based on the extension of the file, check
   *  it out if you want to implement it. */
  GLuint build_program(const std::string & vertex_shader_file,
                       const std::string & fragment_shader_file)
  {
    std::vector<GLuint> shaders(2);
    shaders[0] = compile_shader(vertex_shader_file, GL_VERTEX_SHADER);
    shaders[1] = compile_shader(fragment_shader_file, GL_FRAGMENT_SHADER);
    return link_program(shaders);
  }

}
