#version 440

// Transformation matrix
uniform mat4 MVP;

// Incoming position
layout(location = 0) in vec3 position;

void main()
{
  gl_Position = MVP * vec4(position, 1.0);
}