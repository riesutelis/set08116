// Calculates the shadow factor of a vertex
float calculate_shadow(in sampler2DShadow shadow_map, in vec4 light_space_pos)
{

  float depthdiff = textureProj(shadow_map, light_space_pos);
  if (depthdiff != 1.0)
  {
    depthdiff = 0.5;
  }

  return (depthdiff);
}