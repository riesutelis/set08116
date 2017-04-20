#version 440 core

in VertexData
{
	vec4 colour;
};

layout(location = 0) out vec4 colour_out;

void main()
{
	colour_out = colour;
}