#version 450 core

layout(location = 0) in vec2 v_TextureCoordinates;
layout(location = 0) out vec4 Color;
void main()
{
	vec2 c, z;
	float scale = 2.5;

	c.x = v_TextureCoordinates.x - 0.5;
	c.y = v_TextureCoordinates.y; - 0.5;

	int i;
	const int iter = 2000;
	z = c;

	for (i=0; i < iter; i++) 
	{
		float x = (z.x * z.x - z.y * z.y) + c.x;
		float y = (z.y * z.x + z.x * z.y) + c.y;

		if((x * x + y * y) > 4.0) 
			break;

		z.x = x;
		z.y = y;						
	}

	const float value = i == iter ? 0 : float(i) / iter;
	Color = vec4(value, 0, 0, 1.0);
}