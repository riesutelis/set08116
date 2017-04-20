#version 440 core

// Model transformation matrix
uniform mat4 M;
// Transformation matrix
uniform mat4 MVP;
// Normal matrix
uniform mat3 N;
// The light transformation matrix
uniform mat4 lMVP;

// Incoming position
layout (location = 0) in vec3 position;
// Incoming normal
layout (location = 2) in vec3 normal;
// Incoming binormal
layout (location = 3) in vec3 binormal;
// Incoming tangent
layout (location = 4) in vec3 tangent;
// Incoming texture coordinate
layout (location = 10) in vec2 tex_coord_in;

// Outgoing position
layout (location = 0) out vec3 vertex_position;
// Outgoing transformed normal
layout (location = 1) out vec3 transformed_normal;
// Outgoing texture coordinate
layout (location = 2) out vec2 tex_coord_out;
// Outgoing binormal
layout (location = 3) out vec3 binormal_out;
// Outgoing tangent
layout (location = 4) out vec3 tangent_out;
// Outgoing position in lightspace
layout (location = 5) out vec4 light_space_pos;

void main()
{
  const mat4 lightbias = mat4(
    vec4(0.5f, 0.0f, 0.0f, 0.0f),
    vec4(0.0f, 0.5f, 0.0f, 0.0f),
    vec4(0.0f, 0.0f, 0.5f, 0.0f),
    vec4(0.5f, 0.5f, 0.5f, 1.0f)
  );
  gl_Position = MVP * vec4(position, 1.0);
  
  vertex_position = (M * vec4(position, 1.0)).xyz;
  transformed_normal = N * normal;
  tex_coord_out = tex_coord_in;

  light_space_pos = lightbias * lMVP * vec4(position, 1.0);
  tangent_out = tangent;
  binormal_out = binormal;
}
