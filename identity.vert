#version 330 // https://en.wikipedia.org/wiki/OpenGL_Shading_Language#Versions

/* I prepend a_ to things that come from arrays.
 * Variables that come from arrays are called Attribs */
layout (location = 0) in vec3 a_position;


/* I prepend v_ to values that travel from one shader to another,
 * these values are known as Varyings. */
// This varying gives the coordinates inside the quad to the fragment shader,
//  which uses them to draw the Julia set.
out vec2 v_quad_position;

void main()
{
  // We want our quad to cover the full screen.
  // As the quad coordinates are already in the range -1,1
  //  we do not need to transform them, just to cast the vec3 to vec4.
  // Remember the 1 in the last coordinate!
  gl_Position = vec4(a_position,1.f);
  // Also, we write the coordinates as a parameter for the fragment shader.
  v_quad_position = a_position.xy;
}
