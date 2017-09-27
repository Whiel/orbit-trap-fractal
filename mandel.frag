#version 330
#extension GL_ARB_gpu_shader_fp64 : enable
uniform sampler2D u_tex;
uniform vec2 u_c;
uniform float u_aspect_ratio;
uniform float u_time;

// This comes from the vertex shader.
// Contains the coordinates (in the range -1,1) of the fragment we are drawing.
in vec2 v_quad_position;

layout(location = 0) out vec4 o_color;

// blend front-to-back
vec4 under(vec4 dst, vec4 src){
  vec3 rgb = dst.a*src.a*src.rgb + dst.rgb;
  float a = (1-src.a)*dst.a;
  return vec4(rgb,a);
}

void main()
{
  const float iter = 150.0;
  const float threshold = 4.0;

  
  vec2 upscale = normalize(vec2(u_aspect_ratio,1)) * 3;
  //vec2 c = v_quad_position * upscale / u_time;// / 20 + vec2(1,0);
  dvec2 c = v_quad_position / pow(2,u_time) + vec2(0.28693186889504513, 0.014286693904085048);
  dvec2 z = vec2(0.0,0.0); //vec2(phi-2,phi-1);

  // f(z) = z^2 + c
  // Compute z_i = f(z_(i-1))
  // Until the norm of z grows over a threshold
  vec4 color = vec4(0,0,0,1);
  
  int i;
  for(i = 0; i<iter && dot(z,z) < threshold; ++i){
    dvec2 p = z*z;
    z.y = 2*z.x*z.y + c.y;
    z.x = p.x - p.y + c.x;
    p = 0.5 + (0.25 * z);
    color = under(color,texture(u_tex,vec2(p)));
  }
  o_color = color;
  o_color.a = 1 - o_color.a;
}


