// Directional light structure
#ifndef DIRECTIONAL_LIGHT
#define DIRECTIONAL_LIGHT
struct directional_light
{
	vec4 ambient_intensity;
	vec4 light_colour;
	vec3 light_dir;
};
#endif

// A material structure
#ifndef MATERIAL
#define MATERIAL
struct material
{
	vec4 emissive;
	vec4 diffuse_reflection;
	vec4 specular_reflection;
	float shininess;
};
#endif
// Calculates the directional light
vec4 calculate_directional(in directional_light light, in material mat, in vec3 normal, in vec3 view_dir, in vec4 tex_colour)
{
	vec4 ambient = light.ambient_intensity * mat.diffuse_reflection;
	vec4 diffuse = mat.diffuse_reflection * light.light_colour * max(dot(normal, -light.light_dir), 0.0);
	vec3 h = normalize(view_dir + -light.light_dir);
	vec4 specular = pow(max(dot(normal, h), 0.0), mat.shininess) * mat.specular_reflection * light.light_colour;
	vec4 colour = ((mat.emissive + ambient + diffuse) * tex_colour) + specular;
	return colour;
}