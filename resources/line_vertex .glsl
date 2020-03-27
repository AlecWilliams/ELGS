#version 330 core
layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec2 vertTex;


out vec3 vertex_pos;

out vec2 vertex_tex;
void main()
{

	vec4 tpos =  vec4(vertPos, 1.0);
	vertex_pos = tpos.xyz;
	gl_Position = tpos;
	vertex_tex = vertTex;
}
