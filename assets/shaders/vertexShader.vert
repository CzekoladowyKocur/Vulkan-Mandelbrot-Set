#version 450 core
const vec2 g_TextureCoordinates[4] = vec2[] (
	vec2(1.0, 1.0),
	vec2(1.0, 0.0),
	vec2(0.0, 0.0),
	vec2(0.0, 1.0)
);

layout(location = 0) in vec3 a_Position;
layout(location = 0) out vec2 v_TextureCoordinates;
void main()
{
	gl_Position = vec4(a_Position, 1.0);
	v_TextureCoordinates = g_TextureCoordinates[gl_VertexIndex];
}