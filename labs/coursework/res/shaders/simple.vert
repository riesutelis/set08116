#version 440 core

// Model transformation matrix
uniform mat4 M;
// Transformation matrix
uniform mat4 MVP;

// Incoming position
layout (location = 0) in vec3 position;
// Incoming texture coordinate
layout (location = 10) in vec2 tex_coord_in;

// Outgoing position
layout (location = 0) out vec3 vertex_position;
// Outgoing texture coordinate
layout (location = 2) out vec2 tex_coord_out;

void main()
{
  gl_Position = MVP * vec4(position, 1.0);
  
  vertex_position = (M * vec4(position, 1.0)).xyz;
  tex_coord_out = tex_coord_in;
}
