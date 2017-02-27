#version 440

// A directional light structure
struct directional_light {
  vec4 ambient_intensity;
  vec4 light_colour;
  vec3 light_dir;
};

// A material structure
struct material {
  vec4 emissive;
  vec4 diffuse_reflection;
  vec4 specular_reflection;
  float shininess;
};

// Directional light for the scene
uniform directional_light light;
// Material of the object
uniform material mat;
// Position of the camera
uniform vec3 eye_pos;
// Texture
uniform sampler2D tex;

// Incoming position
layout(location = 0) in vec3 vertex_position;
// Incoming normal
layout(location = 1) in vec3 transformed_normal;
// Incoming texture coordinate
layout(location = 2) in vec2 tex_coord_out;

// Outgoing colour
layout(location = 0) out vec4 colour;

void main() {

  // *********************************
  // Calculate ambient component
  vec4 ambient = mat.diffuse_reflection * light.ambient_intensity;
  // Calculate diffuse component
  vec4 diffuse = max(dot(transformed_normal, light.light_dir), 0.0) * (mat.diffuse_reflection * light.light_colour);
  // Calculate view direction
  vec3 view_dir = normalize(eye_pos - vertex_position);
  // Calculate half vector
  vec3 h = normalize(view_dir + light.light_dir);
  // Calculate specular component
  vec4 specular = pow(max(dot(transformed_normal, h), 0.0), mat.shininess) * (mat.specular_reflection * light.light_colour);
  // Sample texture
  colour = texture(tex, tex_coord_out);
  // Calculate primary colour component

  // Calculate final colour - remember alpha
  colour = colour * (ambient + diffuse + mat.emissive) + specular;
  colour.a = 1.0f;
  // *********************************
}