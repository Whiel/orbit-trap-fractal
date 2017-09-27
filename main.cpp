#include "shader.hpp"
#include "lodepng.h"

#include <GL/glew.h>
#include <GL/gl.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include <future>
#include <list>
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

using namespace std::string_literals;

// Attrib indices
static const int POSITION_INDEX = 0;

static const float quad_positions[] = {
   1.0f, 1.0f, 0.0f,
  -1.0f,-1.0f, 0.0f,
   1.0f,-1.0f, 0.0f,
  
  -1.0f,-1.0f, 0.0f,
   1.0f, 1.0f, 0.0f,
  -1.0f, 1.0f, 0.0f,
};

struct image_t
{
  unsigned int width, height;
  std::vector<unsigned char> data;
};

struct fb_t
{
  glm::ivec2 size;
  GLuint fbo;
};

struct options_t
{
  bool good;
  bool use_mouse_coords;
  image_t image;
};

struct input_t
{
  glm::vec2 mouse_position;
  bool mouse_down;
  bool snap_next_frame;
  glm::ivec2 window_size;
  float aspect_ratio;
};

struct model_t
{
  model_t()
  {
    vertices = 0;
    vertex_array = 0;
    vertex_buffer = 0;
  }

  GLuint vertex_array;
  GLuint vertex_buffer;
  int vertices; // Number of vertices in the array
};

// To the action!

static model_t create_quad_model()
{
  model_t quad;
  
  glGenBuffers(1,&quad.vertex_buffer);
  glGenVertexArrays(1,&quad.vertex_array);
  
  glBindVertexArray(quad.vertex_array);

  glBindBuffer(GL_ARRAY_BUFFER, quad.vertex_buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(quad_positions),
               quad_positions, GL_STATIC_DRAW);
  glVertexAttribPointer(POSITION_INDEX, 3, GL_FLOAT, GL_FALSE, 0, NULL);
  glEnableVertexAttribArray(POSITION_INDEX);
  
  // Count the number of vertices.
  quad.vertices = sizeof(quad_positions) / (sizeof(float)*3);
  
  glBindVertexArray(0);

  return quad;
}

static void render_model(const model_t & m)
{
  if(m.vertex_array and m.vertex_buffer and m.vertices){
    glBindVertexArray(m.vertex_array);
    glDrawArrays(GL_TRIANGLES,0,m.vertices);
    glBindVertexArray(0);
  }else{
    std::cerr << "Attempt to render invalid model" << std::endl;
  }
}

// Renders a frame, whatever that means.
// Remember buffer swapping is done by the caller when needed
static void render(float aspect_ratio, GLuint texture, glm::vec2 c, float t)
{
  // Bad practice, but easy prototyping:
  static model_t quad = create_quad_model();
  static GLuint  program = shaders::build_program("./identity.vert","./julia.frag");
  static GLint   tex_loc = glGetUniformLocation(program, "u_tex");
  static GLint   c_loc = glGetUniformLocation(program, "u_c");
  static GLint   ratio_loc = glGetUniformLocation(program, "u_aspect_ratio");
  static GLint   time_loc = glGetUniformLocation(program, "u_time");

  // No need to clear color,
  // the whole screen is painted and no blending is done by the hardware.
  glClear(GL_DEPTH_BUFFER_BIT); 

  glUseProgram(program);
  
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D,texture);
  glUniform1i(tex_loc, 0);
  
  glUniform2f(c_loc, c.x, c.y);
  glUniform1f(ratio_loc, aspect_ratio);
  glUniform1f(time_loc, t);
      
  render_model(quad);
}


// Resize the viewport and update aspect ratio when resizing window.
static void size_callback(GLFWwindow* window,int,int)
{
  auto & input = *((input_t*)glfwGetWindowUserPointer(window));
  int width,height;
  glfwGetFramebufferSize(window, &width, &height);
  glViewport(0, 0, width, height);
  input.window_size = glm::ivec2(width,height);
  input.aspect_ratio = float(width)/height;
}

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
  auto & input = *((input_t*)glfwGetWindowUserPointer(window));
  // This is a bit convoluted
  // The position of the mouse is scaled and offset so that 0,0 is the center
  //  of the screen, and -1 and 1 correspond to the corners.
  input.mouse_position = {(xpos - input.window_size.x*0.5)/input.window_size.x,
                          -(ypos - input.window_size.y*0.5)/input.window_size.y};
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
  auto & input = *((input_t*)glfwGetWindowUserPointer(window));
  // The gotcha is when they press and then launch your program :v
  input.mouse_down = (action != GLFW_RELEASE);  
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  auto & input = *((input_t*)glfwGetWindowUserPointer(window));
  if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
    input.snap_next_frame = true;
  if (key == GLFW_KEY_Q && action == GLFW_PRESS)
    glfwSetWindowShouldClose(window, GLFW_TRUE);
  
}

// Front-to-back blending done in shader wants RGB values to be
// alpha-premultiplied
void image_alpha_premultiply(image_t & image)
{
  for(size_t i = 0; i < image.data.size(); i+=4)
  {
    float alpha = image.data[i+3] / 255.f;
    for(size_t j = 0; j < 3;++j)
      image.data[i+j] = 255 * ((image.data[i+j] / 255.f) * alpha);
  }      
}

// Create a gl window, inits glfw, GLEW, creates the context, yada yada yada...
GLFWwindow * init_gl_window(std::string title, int width, int height)
{
  if (!glfwInit())
    return nullptr;
  
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
  
  GLFWwindow *window =
    glfwCreateWindow(width, height, title.c_str(), NULL, NULL);
  if (!window){
    std::cerr << "glfw: Failed to create the window." << std::endl;
    glfwTerminate();
    return nullptr;
  }  
  glfwMakeContextCurrent(window);
  
  glewExperimental=true; 
  GLenum err = glewInit();
  if (GLEW_OK != err){
    std::cerr << "glew error: " << glewGetErrorString(err) << std::endl;
    glfwTerminate();    
    return nullptr;
  }
  return window;
}


GLuint texture_from_image(const image_t & image)
{
  GLuint texture;
  glGenTextures(1,&texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width, image.height, 0,
               GL_RGBA, GL_UNSIGNED_BYTE, image.data.data());
  glBindTexture(GL_TEXTURE_2D, 0);
  return texture;
}

fb_t create_framebuffer(glm::ivec2 size)
{
  GLuint fbo;
  glGenFramebuffers(1, &fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  
  {
    GLuint color_rb;
    glGenRenderbuffers(1, &color_rb);
    glBindRenderbuffer(GL_RENDERBUFFER, color_rb);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA, size.x, size.y);

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                              GL_RENDERBUFFER, color_rb);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
  }
  {
    GLuint depth_rb;
    glGenRenderbuffers(1, &depth_rb);
    glBindRenderbuffer(GL_RENDERBUFFER, depth_rb);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, size.x, size.y);

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, depth_rb);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  return {size, fbo};
}

auto snap_framebuffer(const fb_t & fb, const std::string & dst)
{
  std::vector<unsigned char> buffer(fb.size.x * fb.size.y * 4, '\0');
  glBindFramebuffer(GL_FRAMEBUFFER, fb.fbo);
  glReadBuffer(GL_COLOR_ATTACHMENT0);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  std::cout << "Reading pixels" << std::endl;
  glReadPixels(0, 0, fb.size.x, fb.size.y, GL_RGBA, GL_UNSIGNED_BYTE, buffer.data());
  std::cout << "Saving image" << std::endl;
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  auto encoding = std::async([buffer{std::move(buffer)},fb,dst](){   
    auto error = lodepng::encode(dst, buffer.data(), fb.size.x, fb.size.y);
    if(error)
    {
      std::cerr << "Failed take snapshot: "
                << lodepng_error_text(error) << std::endl;
      return false;
    }
    std::cout << "Taken snapshot '" << dst << "' "
              << fb.size.x << "x" << fb.size.y << std::endl;
    return true;
  });
  return encoding;
}

// 
options_t parse_options(std::vector<std::string> args)
{
  options_t opts;
  opts.use_mouse_coords = false;
  opts.good = true;
  
  if (args.empty()){
    std::cerr << "Missing image parameter (png only)" << std::endl;
    opts.good = false;
  }

  {
    auto mouse_opt = std::find(begin(args),end(args),"-m"s);
    if (mouse_opt != end(args))
    {
      std::cout << "No time evolution, use mouse drag to set fractal parameters." << std::endl;
      opts.use_mouse_coords = true;
      args.erase(mouse_opt);
    }
  }
  
  {
    auto & image = opts.image;
    auto error = lodepng::decode(image.data,
                                 image.width, image.height,
                                 args.front());
    if(error)
    {
      std::cerr << "Failed to load file: "
                << lodepng_error_text(error) << std::endl;
      opts.good = false;
    }
    else{
      image_alpha_premultiply(image);
      std::cout << "Loaded '" << args.front() << "' "
                << image.width << "x" << image.height << std::endl;
    }
  }
  
  return opts;
}

// Ties an input_t structure to a window.
// The structure MUST OUTLIVE the window.
void prepare_window_input(GLFWwindow * window, input_t & input)
{
  glfwSetWindowUserPointer(window, &input);
  glfwSetCursorPosCallback(window, cursor_position_callback);
  glfwSetMouseButtonCallback(window, mouse_button_callback);
  glfwSetKeyCallback(window, key_callback);
  glfwSetWindowSizeCallback(window, size_callback);
}


void switch_to_framebuffer(const fb_t & fb)
{
  glBindFramebuffer(GL_FRAMEBUFFER, fb.fbo);
  glViewport(0,0,fb.size.x,fb.size.y);
  glDrawBuffer(GL_COLOR_ATTACHMENT0);
}

int main(int argc, char ** argv)
{
   // Size of snapshots
  constexpr glm::ivec2 CAPTURE_SIZE {4096*2,3112*2};
  // Window attributes
  constexpr int  INITIAL_WIDTH  = 800;
  constexpr int  INITIAL_HEIGHT = 600;
  constexpr auto TITLE          = "Fractrap";
  constexpr int  SWAP_INTERVAL  = 1; // frames per v-sync
  // Snapshot file name
  constexpr auto SNAP_FILE = "snap.png";
  // Parameters
  constexpr glm::vec2 INITIAL_C {0.687,0.312}; // Default Julia set C constant
  constexpr float TIME_STEP = 0.015; // Constant time advance per frame
  constexpr glm::vec2 MOUSE_C_SCALE {4.f,4.f};
  // Fixed
  constexpr float INITIAL_RATIO  = float(INITIAL_WIDTH)/INITIAL_HEIGHT;

  // For asynchronous snapshot encoding
  std::list<std::future<bool>> pending_encodings;
  
  input_t input
    {
      {0.f,0.f}, false,                   // mouse-pos, mouse-down
      false,                              // snap-next-frame
      {INITIAL_WIDTH,INITIAL_HEIGHT},     // window-size
      INITIAL_RATIO                       // aspect-ratio
    };  
  
  auto opts = parse_options({argv+1,argv+argc});  
  if (!opts.good)
    return 1;

  auto window = init_gl_window(TITLE, input.window_size.x, input.window_size.y);
  if (!window)
    return 2;  
  auto texture = texture_from_image(opts.image);       // source image
  auto capture_fb = create_framebuffer(CAPTURE_SIZE);  // for snapshots
  float time = 0.0f;
  glm::vec2 julia_c = INITIAL_C;

  glfwSwapInterval(SWAP_INTERVAL);
  prepare_window_input(window, input);
  glViewport(0,0,input.window_size.x, input.window_size.y);

  auto render_frame =
    [&](){
      render(input.aspect_ratio, texture, julia_c,time);
    };
  auto snapshot =
    [&](std::string output){
      std::cout << "Rendering snapshot" << std::endl;
      switch_to_framebuffer(capture_fb);
      render_frame();
      switch_to_framebuffer({input.window_size,0}); // default fb                    
      glFinish();
      pending_encodings.push_front(snap_framebuffer(capture_fb, output));
      std::cout << "c = " << julia_c.x
                              << " " << "+-"[julia_c.y < 0]
                << " i" << abs(julia_c.y) << std::endl;
    };

  // main loop
  while(not glfwWindowShouldClose(window)){
    
    pending_encodings.remove_if([](auto & f){
      return (f.wait_for(0s) == std::future_status::ready);
    });
    glfwPollEvents();
    
    julia_c = [&](){
                  if (!opts.use_mouse_coords)
                    return glm::vec2{0.75421*cos(0.92+time),0.75421*cos(1.98+time*0.9)};
                  if(input.mouse_down)
                    return MOUSE_C_SCALE*input.mouse_position;
                  return julia_c;
              }();    
    time += TIME_STEP;       
    
    if(input.snap_next_frame){
      snapshot(SNAP_FILE);
      input.snap_next_frame = false;
    }
    
    render_frame();
    glfwSwapBuffers(window);
  }

  // Termination
  glfwDestroyWindow(window);
  if(not pending_encodings.empty())
  {
    std::cout << pending_encodings.size()
              << " snapshots are still being encoded..." << std::endl;
    std::for_each(begin(pending_encodings),end(pending_encodings),
                  [](auto & f){
                    f.wait();
                  });
    std::cout << "Done" << std::endl;     
  }
  return 0;
}




