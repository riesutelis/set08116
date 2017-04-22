#version 430 core

// Captured render
uniform sampler2D tex;
// Alpha map
uniform sampler2D alpha_map;

// Incoming texture coordinate
layout(location = 2) in vec2 tex_coord;

// Outgoing colour
layout(location = 0) out vec4 colour;

void main() {
  // *********************************
  // Sample textures
  vec4 tex1 = texture(tex, tex_coord);
  vec4 tex2 = texture(alpha_map, tex_coord);
  // Final colour is produce of these two colours
  //colour = tex1 * tex2;
  colour = mix(tex1, tex2, tex2.a);
  // Ensure alpha is 1
  colour.a = 1.0;
  // *********************************
}