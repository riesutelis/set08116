#version 440

// Texture uniform
uniform samplerCube cube;

// Incoming texture coordinate
layout(location = 0) in vec3 tex_coord;

// Outgoing colour
layout(location = 0) out vec4 colour;

void main()
{
	colour = texture(cube, tex_coord);
}