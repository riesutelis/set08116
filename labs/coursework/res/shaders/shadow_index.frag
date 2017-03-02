// Calculates the shadow factor of a vertex
float calculate_shadow(in sampler2D shadow_map, in vec4 light_space_pos)
{
	// Dehomogenise coordinates
	vec3 proj_coords = light_space_pos.xyz / light_space_pos.w;
	// This is a bias calculation to convert to texture space
	vec2 shadow_tex_coords.x = 0.5 * proj_coords.x + 0.5;
	shadow_tex_coords.y = 0.5 * proj_coords.y + 0.5;
	// Check shadow coord is in range
	if (shadow_tex_coords.x < 0 || shadow_tex_coords.x > 1 || shadow_tex_coords.y < 0 || shadow_tex_coords.y > 1)
	{
		return 1.0;
	}
	// Bias calculation for z
	float z = 0.5 * proj_coords.z + 0.5;
  
	float depth = texture(shadow_map, shadow_tex_coords).x;
   
	// Check if depth is in range.  Add a slight epsilon for precision
	if (depth == 0.0)
	{
		return 1.0;
	}
	else if (depth < z + 0.001)
	{
		return 0.5;
	}
	else
	{
		return 1.0;
	}
}