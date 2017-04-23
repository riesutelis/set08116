#version 440 core

// This shader requires direction.frag, point.frag and spot.frag

// Directional light structure
#ifndef DIRECTIONAL_LIGHT
#define DIRECTIONAL_LIGHT
struct directional_light {
  vec4 ambient_intensity;
  vec4 light_colour;
  vec3 light_dir;
};
#endif

// Point light information
#ifndef POINT_LIGHT
#define POINT_LIGHT
struct point_light {
  vec4 light_colour;
  vec3 position;
  float constant;
  float linear;
  float quadratic;
};
#endif

// Spot light data
#ifndef SPOT_LIGHT
#define SPOT_LIGHT
struct spot_light {
  vec4 light_colour;
  vec3 position;
  vec3 direction;
  float constant;
  float linear;
  float quadratic;
  float power;
};
#endif

// A material structure
#ifndef MATERIAL
#define MATERIAL
struct material {
  vec4 emissive;
  vec4 diffuse_reflection;
  vec4 specular_reflection;
  float shininess;
};
#endif

// Forward declarations of used functions
vec4 calculate_directional(in directional_light light, in material mat, in vec3 normal, in vec3 view_dir,
                         in vec4 tex_colour);
vec4 calculate_point(in point_light point, in material mat, in vec3 position, in vec3 normal, in vec3 view_dir,
                     in vec4 tex_colour);
vec4 calculate_spot(in spot_light spot, in material mat, in vec3 position, in vec3 normal, in vec3 view_dir,
                    in vec4 tex_colour);
float calculate_shadow(in sampler2DShadow   shadow_map, in vec4 light_space_pos);

// Directional light information
uniform directional_light light;
// Point lights being used in the scene
uniform point_light points[10];
// Spot lights being used in the scene
uniform spot_light spots[10];
// Number of point lights used
uniform int pn;
// Number of spot lights used
uniform int sn;
// Material of the object being rendered
uniform material mat;
// Position of the eye
uniform vec3 eye_pos;
// Texture to sample from
uniform sampler2D tex;
// Texture to sample normals from
uniform sampler2D normal_map;
// 1 to map normals
uniform float map_norms;
// Shadow map to sample from
uniform sampler2DShadow shadow_map;
// Portal position
uniform vec3 portal_pos;
// Portal normal
uniform vec3 portal_normal;
// Portal normal
uniform vec3 other_portal_normal;
// Offset matrix
uniform mat4 offset;

// Incoming position
layout(location = 0) in vec3 position;
// Incoming normal
layout(location = 1) in vec3 normal;
// Incoming texture coordinate
layout(location = 2) in vec2 tex_coord;
// Incoming binormal
layout (location = 3) in vec3 binormal;
// Incoming tangent
layout (location = 4) in vec3 tangent;
// Incoming texture coordinate
layout(location = 5) in vec4 light_space_pos;

// Outgoing colour
layout(location = 0) out vec4 colour;

void main()
{
	if (dot(eye_pos - portal_pos, portal_normal) < 0)
	{
		if (dot(other_portal_normal, ((offset * vec4(portal_pos, 1.0)).xyz - position)) > 0)
			discard;
	}
	else
	{
		if (dot(other_portal_normal * -1, ((offset * vec4(portal_pos, 1.0)).xyz - position)) > 0)
			discard;
	}
		
	// Calculate shade factor
	float shade_factor = calculate_shadow(shadow_map, light_space_pos);

	vec3 view_dir = normalize(eye_pos - position);
	vec4 tex_colour = texture(tex, vec2(tex_coord.x, -tex_coord.y));

	vec3 new_normal;
	if (map_norms > 0.0)
	{
		vec3 norm_sample = texture(normal_map, tex_coord).xyz;
		new_normal = normalize(normal);
		vec3 new_tangent = normalize(tangent);
		vec3 new_binormal = normalize(binormal);

		// Transform components to range [0, 1]
		norm_sample = 2.0 * norm_sample - vec3(1.0, 1.0, 1.0);
		// Generate TBN matrix
		mat3 TBN = mat3(new_tangent, new_binormal, new_normal);
		// Return sampled normal transformed by TBN
		new_normal = normalize(TBN * norm_sample);
	}
	else
		new_normal = normal;


	colour += calculate_directional(light, mat, new_normal, view_dir, tex_colour);
    for (int i = 0; i < pn; i++)
	{
		// The only shadow map used at the moment is for spotlight 1
		if (i != 1)
			colour += calculate_point(points[i], mat, position, new_normal, view_dir, tex_colour);
		else if (shade_factor > 0.5f)
			colour += calculate_point(points[i], mat, position, new_normal, view_dir, tex_colour);
	}
    for (int i = 0; i < sn; i++)
		colour += calculate_spot(spots[i], mat, position, new_normal, view_dir, tex_colour);
	colour.a = 1.0;
}