# version 440

// Texture uniform
uniform sampler2D tex;
// Hue shift uniform
uniform float hue_offset;
// Saturation uniform
uniform float saturation;
// Brightness uniform
uniform float brightness;

// Incoming position
layout(location = 0) in vec3 position;
// Incoming texture coordinate
layout(location = 2) in vec2 tex_coord;

// Outgoing colour
layout(location = 0) out vec4 colour;

void main()
{
	vec4 cl = texture(tex, tex_coord);
	float mx = max(cl.r, max(cl.g, cl.b));
	float mn = min(cl.r, min(cl.g, cl.b));
	float C = mx - mn;

	// Converting to a modified HSV model
	float H;
	if (mx == cl.r)
	{
		H = mod(((cl.g - cl.b) / C), 6.0);
	}
	else if (mx == cl.g)
	{
		H = (cl.b - cl.r) / C + 2.0;
	}
	else if (mx == cl.b)
	{
		H = (cl.r - cl.g) / C + 4.0;
	}
	else
	{
		H = 0.0;
	}
	H = H * 60;
	float Y = 0.299 * cl.r + 0.587 * cl.g + 0.114 * cl.b;
	float S = 0.0;
	if (Y != 0)
		S = C / Y;
	vec3 HCL = vec3(H, S, Y);

	// Modifying the colour values
	HCL.x = HCL.x + hue_offset;
	HCL.y = clamp(HCL.y + saturation, 0.0, 1.0);
	HCL.z = clamp(HCL.z + brightness, 0.0, 1.0);
	
	// Converting back to RGB
	H = HCL.x / 60;
	C = HCL.z * HCL.y;
	float X = C * (1 - abs(mod(H, 2.0) - 1));
	if (HCL.z == 0)
	{
		cl = vec4(0.0, 0.0, 0.0, 1.0);
	}
	else if (H < 1.0)
	{
		cl = vec4(C, X, 0.0, 1.0);
	}
	else if (H < 2.0)
	{
		cl = vec4(X, C, 0.0, 1.0);
	}
	else if (H < 3.0)
	{
		cl = vec4(0.0, C, X, 1.0);
	}
	else if (H < 4.0)
	{
		cl = vec4(0.0, X, C, 1.0);
	}
	else if (H < 5.0)
	{
		cl = vec4(X, 0.0, C, 1.0);
	}
	else if (H < 6.0)
	{
		cl = vec4(C, 0.0, X, 1.0);
	}

	mn = HCL.z - (0.3 * cl.r + 0.59 * cl.g + 0.11 * cl.b);
	colour = vec4(cl.r + mn, cl.g + mn, cl.b + mn, 1.0);
}